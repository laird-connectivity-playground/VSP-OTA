/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: mainwindow.cpp
**
** Notes:
**
** License: This program is free software: you can redistribute it and/or
**          modify it under the terms of the GNU General Public License as
**          published by the Free Software Foundation, version 3.
**
**          This program is distributed in the hope that it will be useful,
**          but WITHOUT ANY WARRANTY; without even the implied warranty of
**          MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**          GNU General Public License for more details.
**
**          You should have received a copy of the GNU General Public License
**          along with this program.  If not, see http://www.gnu.org/licenses/
**
*******************************************************************************/
#include "mainwindow.h"
#include "ui_mainwindow.h"

//Global string for toast messages (required for Android)
QString gstrToastString;

//=============================================================================
//=============================================================================
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    //Setup GUI
    ui->setupUi(this);

    //Load the images/animations
    LoadImages();

    //Add loading image to the status bar
    labelStatusBarLoader = new QLabel();
    ui->statusBar->findChild<QHBoxLayout *>()->insertWidget(0, labelStatusBarLoader);

    //Change image to standby
    unOldStatus = 255;
    SetLoadingStatus(STATUS_STANDBY);

    //Hide buttons for use when connected
    ui->btn_Disconnect->setVisible(false);
    ui->btn_Download->setVisible(false);
    ui->btn_ModuleInfo->setVisible(false);

    //Not in a mode
    nCurrentMode = MAIN_MODE_IDLE;
    ui->btn_Cancel->setEnabled(false);

    //Update window title with version
    this->setWindowTitle(this->windowTitle().append(", ").append(APP_VERSION));

    //Clear variables
    unWrittenBytes = 0;
    unRecDatSize = 0;
    blesvcVSPService = NULL;
    lecBLEController = NULL;
    unTotalSizeSent = 0;
    unTotalAppSize = 0;
    nSelectedFileType = 0;
    strChecksumString = NULL;
    chkChecksum = NULL;
    bVSPBlocked = false;
    bDisconnectActive = false;
    bIsConnected = false;
    bHasModem = false;

    //Setup BLE discovery agent
    ddaDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    connect(ddaDiscoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(FoundDevice(QBluetoothDeviceInfo)));
    connect(ddaDiscoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)), this, SLOT(BTError(QBluetoothDeviceDiscoveryAgent::Error)));
    connect(ddaDiscoveryAgent, SIGNAL(finished()), this, SLOT(BTFinishedScan()));
    ddaDiscoveryAgent->setLowEnergyDiscoveryTimeout(TIMEOUT_BLE_SCAN);

    //Setup device scan dialog
    dlgScanDialog = new ScanSelection(this);
    connect(dlgScanDialog, SIGNAL(DeviceSelected(QBluetoothDeviceInfo)), this, SLOT(ConnectToDevice(QBluetoothDeviceInfo)));
    connect(dlgScanDialog, SIGNAL(WindowClosed()), this, SLOT(CancelScan()));
    connect(dlgScanDialog, SIGNAL(ScanningFinished(quint8)), this, SLOT(SetLoadingStatus(quint8)));

    //Setup file location selection dialogue
    dlgFileTypeDialog = new FileTypeSelection(this);
    connect(dlgFileTypeDialog, SIGNAL(DisplayMessage(QString,bool)), this, SLOT(ExternalToastMessage(QString,bool)));
    connect(dlgFileTypeDialog, SIGNAL(FileTypeChanged(qint8,QString)), this, SLOT(FileTypeChanged(qint8,QString)));

    //Setup disconnect clean up timer
    tmrDisconnectCleanUpTimer = new QTimer();
    tmrDisconnectCleanUpTimer->setSingleShot(true);
    tmrDisconnectCleanUpTimer->setInterval(10);
    connect(tmrDisconnectCleanUpTimer, SIGNAL(timeout()), this, SLOT(ClearVar()));

    //Setup display update timer
    tmrDisplayUpdateTimer = new QTimer();
    tmrDisplayUpdateTimer->setSingleShot(true);
    tmrDisplayUpdateTimer->setInterval(80);
    connect(tmrDisplayUpdateTimer, SIGNAL(timeout()), this, SLOT(UpdateDisplay()));

    //Setup response timeout timer
    tmrResponseTimeoutTimer = new QTimer();
    tmrResponseTimeoutTimer->setInterval(TIMEOUT_TIMER_INTERVAL);
    tmrResponseTimeoutTimer->setSingleShot(true);
    connect(tmrResponseTimeoutTimer, SIGNAL(timeout()), this, SLOT(TimeoutTimerElapsed()));

    UpdateTxRx();

    //Connect the downloader signals
    dwnDownloaderHandle = new Downloader();
    connect(dwnDownloaderHandle, SIGNAL(XCompileComplete(bool,qint16,QByteArray)), this, SLOT(ProcessFileData(bool,qint16,QByteArray)));
    connect(dwnDownloaderHandle, SIGNAL(FileDownloaded(bool,qint16,QByteArray)), this, SLOT(FileDownloaded(bool,qint16,QByteArray)));
    connect(dwnDownloaderHandle, SIGNAL(FirmwareResponse(bool,qint16,QString)), this, SLOT(FirmwareVersionCheck(bool,qint16,QString)));
    connect(dwnDownloaderHandle, SIGNAL(StatusChanged(quint8)), this, SLOT(DownloaderStatusChanged(quint8)));

    //Load the settings
    stgSettingsHandle = new SettingsStorage(this);
    qint8 nSettingsCode = stgSettingsHandle->LoadSettings();
    if (nSettingsCode == SETTINGS_LOAD_NONE)
    {
        //Set default settings
        stgSettingsHandle->DefaultSettings();
    }
    else if (nSettingsCode == SETTINGS_LOAD_ERROR)
    {
        //Settings opening failed
        QMessageBox::critical(this, "Configuration Write Failed", "Failed to write default configuration, please ensure you grant write access or errors and undesired operation may occur.", QMessageBox::Ok, QMessageBox::NoButton);
    }

    //Android: Check for bluetooth permissions
#ifdef Q_OS_ANDROID
    if (QtAndroid::androidSdkVersion() >= 23)
    {
        if (QtAndroid::checkPermission("android.permission.BLUETOOTH") == QtAndroid::PermissionResult::Denied || QtAndroid::checkPermission("android.permission.READ_EXTERNAL_STORAGE") == QtAndroid::PermissionResult::Denied || QtAndroid::checkPermission("android.permission.ACCESS_COARSE_LOCATION") == QtAndroid::PermissionResult::Denied)
        {
            //Bluetooth permission denied
            gstrToastString = "Bluetooth permission (coarse location) and external file read permissions are required, please grant access.";
            ToastMessage(true);
            QtAndroid::PermissionResultMap PermissionResult = QtAndroid::requestPermissionsSync(QStringList() << "android.permission.BLUETOOTH" << "android.permission.READ_EXTERNAL_STORAGE" << "android.permission.ACCESS_COARSE_LOCATION");
            if (PermissionResult.find("android.permission.BLUETOOTH").value() == QtAndroid::PermissionResult::Denied || PermissionResult.find("android.permission.READ_EXTERNAL_STORAGE").value() == QtAndroid::PermissionResult::Denied)
            {
                //User's request for Bluetooth/read external permission has been denied
                gstrToastString = "Bluetooth (coarse location)/external file access permission request denied, application is non-functional.";
                ui->edit_Display->clear();
                ui->edit_Display->appendPlainText(gstrToastString);
                ToastMessage(true);
                ui->btn_Clear->setEnabled(false);
                ui->btn_Disconnect->setEnabled(false);
                ui->btn_Download->setEnabled(false);
                ui->btn_ModuleInfo->setEnabled(false);
                ui->btn_Scan->setEnabled(false);
                ui->btn_SelectFile->setEnabled(false);
                ui->btn_Settings->setEnabled(false);
            }
        }
    }
#endif

    //Setup regular expression objects
    rxpDevName.setPattern("\t0\t([a-zA-Z0-9\\-_]{3,20})\r");
    rxpDevName.setPatternOptions(QRegularExpression::MultilineOption);
    rxpXCompiler.setPattern("\t13\t([a-zA-Z0-9]{4}) ([a-zA-Z0-9]{4})");
    rxpXCompiler.setPatternOptions(QRegularExpression::MultilineOption);
    rxpFreeSpace.setPattern("\t6\t([0-9]+),([0-9]+),([0-9]+)\r");
    rxpFreeSpace.setPatternOptions(QRegularExpression::MultilineOption);
    rxpFirmware.setPattern("\t3\t([0-9a-zA-Z\\-\\_\\.]+)\r");
    rxpFirmware.setPatternOptions(QRegularExpression::MultilineOption);
    rxpErrorCode.setPattern("\n01\t([a-fA-f0-9]+)\r");
    rxpErrorCode.setPatternOptions(QRegularExpression::MultilineOption);
    rxpCRC.setPattern("\n10\t49452\t([0-9A-Fa-f]{4})\r\n");
    rxpCRC.setPatternOptions(QRegularExpression::MultilineOption);

    //Calculate space required for loading image
    QFontMetrics fmFontMet(ui->statusBar->font());
    nStatusBarSpaces = ceilf((32.0 / (float)fmFontMet.width(" ")));

    //Show application version
    ui->statusBar->showMessage(QString(" ").repeated(nStatusBarSpaces).append("Laird OTA VSP, v").append(APP_VERSION));

    //Check if device supports 2M PHY
    bIs2MPhySupported = false;
#ifdef Q_OS_ANDROID
    QAndroidJniObject adapter = QAndroidJniObject::callStaticObjectMethod("android/bluetooth/BluetoothAdapter", "getDefaultAdapter", "()Landroid/bluetooth/BluetoothAdapter;");
    if (!adapter.isValid())
    {
        //Bluetooth not detected on device
        QMessageBox::warning(this, "Bluetooth Not Detected", "Bluetooth support was not detected on your phone, please check you have granted permission and that your device supports Bluetooth v4.0 or later, otherwise this application will not function.");
    }
    else
    {
        //Check if 2M PHY is supported
        if (QtAndroid::androidSdkVersion() >= 26)
        {
            bIs2MPhySupported = adapter.callMethod<jboolean>("isLe2MPhySupported", "()Z");
            if (bIs2MPhySupported == true)
            {
                //Show warning
                QMessageBox::warning(this, "Bluetooth 2M PHY", "Your device supports Bluetooth Low Energy 2M PHY - please note that OTA/VSP to BL652 on firmware v28.7.3.0 will not function.");
            }
        }
    }
#endif

    //Setup start-up timer
    tmrStartupTimer = new QTimer();
    tmrStartupTimer->setInterval(20);
    tmrStartupTimer->setSingleShot(true);
    connect(tmrStartupTimer, SIGNAL(timeout()), this, SLOT(StartupTimerElapsed()));
    tmrStartupTimer->start();
}

//=============================================================================
//=============================================================================
MainWindow::~MainWindow(
    )
{
    if (blesvcVSPService != NULL)
    {
        //Clean up VSP service
        disconnect(this, SLOT(VSPServiceCharacteristicChanged(QLowEnergyCharacteristic,QByteArray)));
        disconnect(this, SLOT(VSPServiceCharacteristicWritten(QLowEnergyCharacteristic,QByteArray)));
        disconnect(this, SLOT(VSPServiceDescriptorWritten(QLowEnergyDescriptor,QByteArray)));
        disconnect(this, SLOT(VSPServiceError(QLowEnergyService::ServiceError)));
        disconnect(this, SLOT(VSPServiceStateChanged(QLowEnergyService::ServiceState)));
        delete blesvcVSPService;
        blesvcVSPService = NULL;
    }

    if (lecBLEController != NULL)
    {
        //Clean up BLE controller
        disconnect(this, SLOT(BLEConnected()));
        disconnect(this, SLOT(BLEDisconnected()));
        disconnect(this, SLOT(BLEDiscoveryFinished()));
        disconnect(this, SLOT(BLEError(QLowEnergyController::Error)));
        disconnect(this, SLOT(BLEDiscovered(QBluetoothUuid)));
        disconnect(this, SLOT(BLEStateChanged(QLowEnergyController::ControllerState)));
        lecBLEController->deleteLater();
    }

    if (stgSettingsHandle != NULL)
    {
        //Clean up settings
        delete stgSettingsHandle;
    }

    if (dlgFileTypeDialog != NULL)
    {
        //Clean up file type dialog
        disconnect(dlgFileTypeDialog, SIGNAL(DisplayMessage(QString,bool)));
        disconnect(dlgFileTypeDialog, SIGNAL(FileTypeChanged(qint8,QString)));
        delete dlgFileTypeDialog;
    }

    //Clean up timers
    disconnect(this, SLOT(ClearVar()));
    disconnect(this, SLOT(UpdateDisplay()));
    disconnect(this, SLOT(TimeoutTimerElapsed()));
    delete tmrDisconnectCleanUpTimer;
    delete tmrDisplayUpdateTimer;
    delete tmrResponseTimeoutTimer;

    disconnect(this, SLOT(ProcessFileData(bool,qint16,QByteArray)));

    if (dwnDownloaderHandle != NULL)
    {
        //Clean up downloader object
        disconnect(dwnDownloaderHandle, SIGNAL(XCompileComplete(QByteArray*)));
        disconnect(dwnDownloaderHandle, SIGNAL(FileDownloaded(bool,qint16,QByteArray)));
        disconnect(dwnDownloaderHandle, SIGNAL(FirmwareResponse(bool,qint16,QString)));
        disconnect(dwnDownloaderHandle, SIGNAL(StatusChanged(quint8)));
        delete dwnDownloaderHandle;
    }

    disconnect(this, SLOT(FoundDevice(QBluetoothDeviceInfo)));
    disconnect(this, SLOT(BTError(QBluetoothDeviceDiscoveryAgent::Error)));
    disconnect(this, SLOT(BTFinishedScan()));
    disconnect(this, SLOT(ConnectToDevice(QBluetoothDeviceInfo)));
    delete dlgScanDialog;
    delete ddaDiscoveryAgent;

    if (strChecksumString != NULL)
    {
        delete strChecksumString;
        strChecksumString = NULL;
    }

    if (chkChecksum != NULL)
    {
        delete chkChecksum;
        chkChecksum = NULL;
    }

#ifdef Q_OS_ANDROID
    if (afdFileDialog != NULL)
    {
        disconnect(this, SLOT(AndroidOpen(QString,QByteArray)));
        delete afdFileDialog;
        afdFileDialog = NULL;
    }
#endif

    if (dlgSettingsView != NULL)
    {
        disconnect(this, SLOT(SettingsUpdated(QString,QString,QString,QString,QString,bool,
#ifdef Q_OS_ANDROID
            bool,
#endif
            quint8,bool,bool,quint8,bool,quint8,bool,bool,bool,bool)));
        delete dlgSettingsView;
        dlgSettingsView = NULL;
    }

    ui->statusBar->removeWidget(labelStatusBarLoader);
    delete labelStatusBarLoader;
    movieLoadingAnimation->stop();
    delete movieLoadingAnimation;
    delete pixmapStandbyPicture;

    delete ui;
}

//=============================================================================
//=============================================================================
void
MainWindow::BTError(
    QBluetoothDeviceDiscoveryAgent::Error nErrorCode
    )
{
    //Bluetooth error occured
    if (nErrorCode != QBluetoothDeviceDiscoveryAgent::NoError)
    {
        if (nErrorCode == QBluetoothDeviceDiscoveryAgent::PoweredOffError)
        {
            //Bluetooth radio is powered off
            gstrToastString = "Bluetooth radio is powered off, please power it on.";
        }
        else if (nErrorCode == QBluetoothDeviceDiscoveryAgent::InputOutputError)
        {
            //Error reading from/writing to Bluetooth adapter
            gstrToastString = "Error reading from/writing to Bluetooth adapter.";
        }
        else if (nErrorCode == QBluetoothDeviceDiscoveryAgent::InvalidBluetoothAdapterError)
        {
            //Bluetooth device mismatch
            gstrToastString = "Invalid Bluetooth adapter, check your system settings.";
        }
        else if (nErrorCode == QBluetoothDeviceDiscoveryAgent::UnsupportedPlatformError)
        {
            //Platform does not support Bluetooth searching
            gstrToastString = "The platform you are on does not support the required Bluetooth functionality.";
        }
        else if (nErrorCode == QBluetoothDeviceDiscoveryAgent::UnsupportedDiscoveryMethod)
        {
            //Discovery type not supported on device
            gstrToastString = "The platform you are on does not support the required Bluetooth functionality.";
        }
        else if (nErrorCode == QBluetoothDeviceDiscoveryAgent::UnknownError)
        {
            //Unknown error occured
            gstrToastString = "An unknown Bluetooth error has occured.";
        }

        //Show message
        ToastMessage(false);
    }
#ifdef ENABLE_DEBUG
    qDebug() << "Error: " << nErrorCode;
#endif
}

//=============================================================================
//=============================================================================
void
MainWindow::BTFinishedScan(
    )
{
    //Bluetooth scan finished
    dlgScanDialog->SetStatus(STATUS_STANDBY);
    SetLoadingStatus(STATUS_STANDBY);
#ifdef ENABLE_DEBUG
    qDebug() << "Finished";
#endif
}

//=============================================================================
//=============================================================================
void
MainWindow::FoundDevice(
    QBluetoothDeviceInfo bdiDeviceInfo
    )
{
    //Bluetooth device detected
    if (bdiDeviceInfo.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
    {
        if (stgSettingsHandle->GetBool(SETTINGS_KEY_RESTRICTUUID) == true)
        {
            //Service filtering, check if VSP service is present
#ifdef ENABLE_DEBUG
            qDebug() << "Detected device: ";
#ifdef Q_OS_MAC
            qDebug() << bdiDeviceInfo.deviceUuid() << bdiDeviceInfo.name() << bdiDeviceInfo.rssi();
#else
            qDebug() << bdiDeviceInfo.address() << bdiDeviceInfo.name() << bdiDeviceInfo.rssi();
#endif
#endif
            QList<QBluetoothUuid> Services = bdiDeviceInfo.serviceUuids();
            int i = 0;
            while (i < Services.count())
            {
                if (Services.at(i).toString() == QString("{").append(stgSettingsHandle->GetString(SETTINGS_KEY_UUID)).append("}"))
                {
                    //Found VSP service
                    dlgScanDialog->ScanSelection::AddDevice(bdiDeviceInfo);
                    break;
                }
               ++i;
            }
        }
        else
        {
            //No filtering, add device
#ifdef ENABLE_DEBUG
            qDebug() << "Detected device: ";
#ifdef Q_OS_MAC
            qDebug() << bdiDeviceInfo.deviceUuid() << bdiDeviceInfo.name() << bdiDeviceInfo.rssi();
#else
            qDebug() << bdiDeviceInfo.address() << bdiDeviceInfo.name() << bdiDeviceInfo.rssi();
#endif
#endif
            dlgScanDialog->ScanSelection::AddDevice(bdiDeviceInfo);
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::BLEConnected(
    )
{
    //Connected to Bluetooth device
#ifdef ENABLE_DEBUG
    qDebug() << "Connected";
#endif
    nCurrentMode = MAIN_MODE_DISCOVERING;
    lecBLEController->discoverServices();
    ui->btn_Scan->setVisible(false);
    ui->btn_Scan->setEnabled(false);
    ui->btn_Disconnect->setEnabled(true);
    ui->btn_Disconnect->setVisible(true);
    ui->btn_Download->setEnabled(true);
    ui->btn_Download->setVisible(true);
    ui->btn_ModuleInfo->setEnabled(true);
    ui->btn_ModuleInfo->setVisible(true);

    //Now connected
    bIsConnected = true;
}

//=============================================================================
//=============================================================================
void
MainWindow::BLEDisconnected(
    )
{
    //Disconnected from Bluetooth device
#ifdef ENABLE_DEBUG
    qDebug() << "Disconnected";
#endif
    if (nCurrentMode != MAIN_MODE_IDLE)
    {
        //Cancel any current operations
        baOutputBuffer.clear();
        balOutputBufferList.clear();
        baVersionResponse.clear();
        baFileData.clear();
        if (tmrResponseTimeoutTimer->isActive())
        {
            //Stop timeout timer
            tmrResponseTimeoutTimer->stop();
        }

        if (nCurrentMode == MAIN_MODE_ONLINE_DOWNLOAD || nCurrentMode == MAIN_MODE_XCOMPILING)
        {
            //Pending web request, cancel it
            dwnDownloaderHandle->CancelRequest();
        }

        //Disable cancel button
        ui->btn_Cancel->setEnabled(false);

        //Back to idle mode
        nCurrentMode = MAIN_MODE_IDLE;
        ui->edit_DownloadName->setReadOnly(false);
    }

    //Clean up
    if (chkChecksum != NULL)
    {
        delete chkChecksum;
        chkChecksum = NULL;
    }

    if (strChecksumString != NULL)
    {
        delete strChecksumString;
        strChecksumString = NULL;
    }

    disconnect(this, SLOT(BLEConnected()));
    disconnect(this, SLOT(BLEDisconnected()));
    disconnect(this, SLOT(BLEDiscoveryFinished()));
    disconnect(this, SLOT(BLEError(QLowEnergyController::Error)));
    disconnect(this, SLOT(BLEDiscovered(QBluetoothUuid)));
    disconnect(this, SLOT(BLEStateChanged(QLowEnergyController::ControllerState)));

    if (blesvcVSPService != NULL)
    {
        disconnect(this, SLOT(VSPServiceCharacteristicChanged(QLowEnergyCharacteristic,QByteArray)));
        disconnect(this, SLOT(VSPServiceCharacteristicWritten(QLowEnergyCharacteristic,QByteArray)));
        disconnect(this, SLOT(VSPServiceDescriptorWritten(QLowEnergyDescriptor,QByteArray)));
        disconnect(this, SLOT(VSPServiceError(QLowEnergyService::ServiceError)));
        disconnect(this, SLOT(VSPServiceStateChanged(QLowEnergyService::ServiceState)));
        delete blesvcVSPService;
        blesvcVSPService = NULL;
    }

    if (lecBLEController != NULL)
    {
        tmrDisconnectCleanUpTimer->start();
    }

    //Change button states to enable scan button
    ui->btn_Disconnect->setVisible(false);
    ui->btn_Disconnect->setEnabled(false);
    ui->btn_Download->setVisible(false);
    ui->btn_Download->setEnabled(false);
    ui->btn_ModuleInfo->setVisible(false);
    ui->btn_ModuleInfo->setEnabled(false);
    ui->btn_Scan->setEnabled(true);
    ui->btn_Scan->setVisible(true);

    //Display message
    gstrToastString = "Device disconnected.";
    ToastMessage(false);

    //No longer busy
    SetLoadingStatus(STATUS_STANDBY);

    //Update state
    bDisconnectActive = false;
    bIsConnected = false;
}

//=============================================================================
//=============================================================================
void
MainWindow::BLEDiscovered(
    QBluetoothUuid
    )
{
    //
}

//=============================================================================
//=============================================================================
void
MainWindow::BLEDiscoveryFinished(
    )
{
    //Bluetooth discovery complete
#ifdef ENABLE_DEBUG
    qDebug() << "Discovery finished, " << QBluetoothUuid(stgSettingsHandle->GetString(SETTINGS_KEY_UUID));
#endif
    blesvcVSPService = lecBLEController->createServiceObject(QBluetoothUuid(stgSettingsHandle->GetString(SETTINGS_KEY_UUID)));
#ifdef ENABLE_DEBUG
    qDebug() << "Is Null: " << (blesvcVSPService == NULL ? "yes" : "no");
    qDebug() << lecBLEController->services();
    qDebug() << "Is Null: " << blesvcVSPService;
//    qDebug() << lecBLEController->children();
////    qDebug() << blesvcVSPService->characteristics();
//    qDebug() << blesvcVSPService->children();
#endif

    //Set status to idle
    nCurrentMode = MAIN_MODE_IDLE;

    //Disable cancel button and enable download name edit
    ui->btn_Cancel->setEnabled(false);
    ui->edit_DownloadName->setReadOnly(false);

    if (!blesvcVSPService)
    {
        //Service not found, disconnect
        if (lecBLEController != NULL && bDisconnectActive == false)
        {
            bDisconnectActive = true;
            lecBLEController->disconnectFromDevice();
        }
        ui->btn_Disconnect->setVisible(false);
        ui->btn_Disconnect->setEnabled(false);
        ui->btn_Download->setVisible(false);
        ui->btn_Download->setEnabled(false);
        ui->btn_ModuleInfo->setVisible(false);
        ui->btn_ModuleInfo->setEnabled(false);
        ui->btn_Scan->setEnabled(true);
        ui->btn_Scan->setVisible(true);
        SetLoadingStatus(STATUS_STANDBY);

        //Set error
        gstrToastString = "VSP service not found, disconnecting...";
        ToastMessage(false);
        ClearVar();

        return;
    }

    connect(blesvcVSPService, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)), this, SLOT(VSPServiceCharacteristicChanged(QLowEnergyCharacteristic,QByteArray)));
    connect(blesvcVSPService, SIGNAL(characteristicWritten(QLowEnergyCharacteristic,QByteArray)), this, SLOT(VSPServiceCharacteristicWritten(QLowEnergyCharacteristic,QByteArray)));
    connect(blesvcVSPService, SIGNAL(descriptorWritten(QLowEnergyDescriptor,QByteArray)), this, SLOT(VSPServiceDescriptorWritten(QLowEnergyDescriptor,QByteArray)));
    connect(blesvcVSPService, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(VSPServiceError(QLowEnergyService::ServiceError)));
    connect(blesvcVSPService, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(VSPServiceStateChanged(QLowEnergyService::ServiceState)));

    blesvcVSPService->discoverDetails();
}

//=============================================================================
//=============================================================================
void
MainWindow::BLEError(
    QLowEnergyController::Error nErrorCode
    )
{
    //Bluetooth Low Energy error
#ifdef ENABLE_DEBUG
    qDebug() << "BLE Error: " << nErrorCode;
#endif
    if (nErrorCode != QLowEnergyController::NoError)
    {
        if (nErrorCode == QLowEnergyController::UnknownError)
        {
            //Unknown error
            if (lecBLEController != NULL && bDisconnectActive == false)
            {
                bDisconnectActive = true;
                lecBLEController->disconnectFromDevice();
            }
            ui->btn_Disconnect->setVisible(false);
            ui->btn_Disconnect->setEnabled(false);
            ui->btn_Download->setVisible(false);
            ui->btn_Download->setEnabled(false);
            ui->btn_ModuleInfo->setVisible(false);
            ui->btn_ModuleInfo->setEnabled(false);
            ui->btn_Scan->setEnabled(true);
            ui->btn_Scan->setVisible(true);
            SetLoadingStatus(STATUS_STANDBY);

            //Set error
            gstrToastString = "Unknown Bluetooth Error.";
            ClearVar();
        }
        else if (nErrorCode == QLowEnergyController::UnknownRemoteDeviceError)
        {
            //Remote device cannot be found
            SetLoadingStatus(STATUS_STANDBY);

            //Set error
            gstrToastString = "Remote Bluetooth device was not found.";
            ClearVar();
        }
        else if (nErrorCode == QLowEnergyController::NetworkError)
        {
            //Failed to read from/write to the remote device
            if (lecBLEController != NULL && bDisconnectActive == false)
            {
                bDisconnectActive = true;
                lecBLEController->disconnectFromDevice();
            }
            ui->btn_Disconnect->setVisible(false);
            ui->btn_Disconnect->setEnabled(false);
            ui->btn_Download->setVisible(false);
            ui->btn_Download->setEnabled(false);
            ui->btn_ModuleInfo->setVisible(false);
            ui->btn_ModuleInfo->setEnabled(false);
            ui->btn_Scan->setEnabled(true);
            ui->btn_Scan->setVisible(true);
            SetLoadingStatus(STATUS_STANDBY);

            //Set error
            gstrToastString = "Failed reading from/writing to remote device.";
            ClearVar();
        }
        else if (nErrorCode == QLowEnergyController::InvalidBluetoothAdapterError)
        {
            //Issue with Bluetooth adapter
            ui->btn_Disconnect->setVisible(false);
            ui->btn_Disconnect->setEnabled(false);
            ui->btn_Download->setVisible(false);
            ui->btn_Download->setEnabled(false);
            ui->btn_ModuleInfo->setVisible(false);
            ui->btn_ModuleInfo->setEnabled(false);
            ui->btn_Scan->setEnabled(true);
            ui->btn_Scan->setVisible(true);
            SetLoadingStatus(STATUS_STANDBY);

            //Set error
            gstrToastString = "Error with Bluetooth Adapter.";
            ClearVar();
        }
        else if (nErrorCode == QLowEnergyController::ConnectionError)
        {
            //Failed to connect to device
            ui->btn_Disconnect->setVisible(false);
            ui->btn_Disconnect->setEnabled(false);
            ui->btn_Download->setVisible(false);
            ui->btn_Download->setEnabled(false);
            ui->btn_ModuleInfo->setVisible(false);
            ui->btn_ModuleInfo->setEnabled(false);
            ui->btn_Scan->setEnabled(true);
            ui->btn_Scan->setVisible(true);
            SetLoadingStatus(STATUS_STANDBY);

            //Set error
            gstrToastString = "Failed to connect to Bluetooth device or connection interrupted during communication.";
            ClearVar();
        }
#if QT_VERSION >= 0x050A00
        //Function was added in Qt 5.10
        else if (nErrorCode == QLowEnergyController::RemoteHostClosedError)
        {
            //Remote device closed the connection
            SetLoadingStatus(STATUS_STANDBY);
            ui->btn_Disconnect->setVisible(false);
            ui->btn_Disconnect->setEnabled(false);
            ui->btn_Download->setVisible(false);
            ui->btn_Download->setEnabled(false);
            ui->btn_ModuleInfo->setVisible(false);
            ui->btn_ModuleInfo->setEnabled(false);
            ui->btn_Scan->setEnabled(true);
            ui->btn_Scan->setVisible(true);

            //Set error
            gstrToastString = "Remote Bluetooth device closed connection.";
            ClearVar();
        }
#endif

        //Set mode to idle
        nCurrentMode = MAIN_MODE_IDLE;

        //Disable cancel button and enable download filename edit
        ui->btn_Cancel->setEnabled(false);
        ui->edit_DownloadName->setReadOnly(false);

        if (tmrResponseTimeoutTimer->isActive())
        {
            //Stop timeout timer
            tmrResponseTimeoutTimer->stop();
        }

        //Show message
        ToastMessage(false);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::BLEStateChanged(
    QLowEnergyController::ControllerState lesNewState
    )
{
#ifdef ENABLE_DEBUG
    qDebug() << "state: " << lesNewState;
#endif
}

//=============================================================================
//=============================================================================
void
MainWindow::VSPServiceCharacteristicChanged(
    QLowEnergyCharacteristic lecCharacteristic,
    QByteArray baData
    )
{
    //
    if (lecCharacteristic == blechrTXChar)
    {
#ifdef ENABLE_DEBUG
        qDebug() << "TX Changed";
#endif
        baRecBuffer.append(baData.replace('\0', "+"));
        TrucateRecBuffer();

        unRecDatSize = unRecDatSize + baData.length();
        if (!tmrDisplayUpdateTimer->isActive())
        {
            tmrDisplayUpdateTimer->start();
        }
        UpdateTxRx();

        if (tmrResponseTimeoutTimer->isActive())
        {
            //Stop timeout timer
            tmrResponseTimeoutTimer->stop();
        }

        if (nCurrentMode == MAIN_MODE_VERSION)
        {
            //Waiting for version response
            baVersionResponse.append(baData);

            //Search for errors
            QRegularExpressionMatch rexpmM1Match = rxpErrorCode.match(baVersionResponse);
            if (rexpmM1Match.hasMatch())
            {
                //Error detected
                baOutputBuffer.clear();
                balOutputBufferList.clear();
                baVersionResponse.clear();
                nCurrentMode = MAIN_MODE_IDLE;
                SetLoadingStatus(STATUS_STANDBY);
                QString ErrorMsg = elErrorLookupHandle.LookupError(rexpmM1Match.captured(1).toUInt(nullptr, 16));
                gstrToastString = QString("Error retrieving module information (").append(rexpmM1Match.captured(1)).append(") ").append(ErrorMsg);
                ToastMessage(false);

                //Disable cancel button and enable download filename edit
                ui->btn_Cancel->setEnabled(false);
                ui->edit_DownloadName->setReadOnly(false);
            }
            else if (baOutputBuffer.length() == 0 && !balOutputBufferList.isEmpty() && baVersionResponse.indexOf("\n00\r") != -1)
            {
                //Command completed successfully
                baVersionResponse.replace("\n00\r", "");

                //Load next command
                baOutputBuffer = balOutputBufferList.takeFirst();
                if (bVSPBlocked == false)
                {
                    blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));
                    tmrResponseTimeoutTimer->start();
                }
            }
            else if (balOutputBufferList.isEmpty() && baOutputBuffer.isEmpty() && baVersionResponse.indexOf("\t0\t") != -1 && baVersionResponse.indexOf("\t13\t") != -1 && baVersionResponse.indexOf("00", baVersionResponse.indexOf("\t13\t")) != -1)
            {
                //Got the version response - extract
                QRegularExpressionMatch rexpmM1Match = rxpDevName.match(baVersionResponse);
                QRegularExpressionMatch rexpmM2Match = rxpXCompiler.match(baVersionResponse);
                QRegularExpressionMatch rexpmM3Match = rxpFreeSpace.match(baVersionResponse);

                if (rexpmM1Match.hasMatch() && rexpmM2Match.hasMatch() && (stgSettingsHandle->GetBool(SETTINGS_KEY_CHECKFREESPACE) == false || rexpmM3Match.hasMatch()))
                {
                    if (bIs2MPhySupported == true && rexpmM1Match.captured(1) == "BL652")
                    {
                        //BL652 detected on phone with 2M PHY
                        QRegularExpressionMatch rexpmM4Match = rxpFirmware.match(baVersionResponse);
                        if (rexpmM4Match.captured(1) == "28.7.3.0" && QMessageBox::question(this, "Continue VSP OTA operation", "Your phone has a BT v5 radio, there is a known issue which can cause a disconnect during a VSP operation on the BL652 of this firmware version - which is more likely to occur with larger applications, do you wish to continue?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
                        {
                            //User does not want to proceed, cancel.
                            baOutputBuffer.clear();
                            nCurrentMode = MAIN_MODE_IDLE;
                            SetLoadingStatus(STATUS_STANDBY);

                            //Show message
                            gstrToastString = "VSP OTA operation cancelled.";
                            ToastMessage(false);

                            //Disable cancel button and enable download filename edit
                            ui->btn_Cancel->setEnabled(false);
                            ui->edit_DownloadName->setReadOnly(false);
                            return;
                        }
                    }

                    //
                    baRecBuffer.append("Download match");
                    baRecBuffer.append(rexpmM1Match.captured(1));
                    baRecBuffer.append(rexpmM2Match.captured(1));
                    baRecBuffer.append(rexpmM2Match.captured(2));
                    nModuleFreeSpace = rexpmM3Match.captured(2).toUInt();
                    UpdateDisplay();
                    nCurrentMode = MAIN_MODE_XCOMPILING;
                    dwnDownloaderHandle->XCompileFile(rexpmM1Match.captured(1), rexpmM2Match.captured(1), rexpmM2Match.captured(2), &baFileData);

                    //Enable cancel button and disable download filename edit
                    ui->btn_Cancel->setEnabled(true);
                    ui->edit_DownloadName->setReadOnly(true);
                }
                else
                {
                    ui->edit_Display->appendPlainText("No match");
                }
            }
        }
        else if (nCurrentMode == MAIN_MODE_SPACECHECK)
        {
            //Waiting for storage space response
            baVersionResponse.append(baData);

            //Search for errors
            QRegularExpressionMatch rexpmM1Match = rxpErrorCode.match(baVersionResponse);
            if (rexpmM1Match.hasMatch())
            {
                //Error detected
                baOutputBuffer.clear();
                balOutputBufferList.clear();
                baVersionResponse.clear();
                nCurrentMode = MAIN_MODE_IDLE;
                SetLoadingStatus(STATUS_STANDBY);
                QString ErrorMsg = elErrorLookupHandle.LookupError(rexpmM1Match.captured(1).toUInt(nullptr, 16));
                gstrToastString = QString("Error retrieving storage space (").append(rexpmM1Match.captured(1)).append(") ").append(ErrorMsg);
                ToastMessage(false);

                //Disable cancel button and enable download filename edit
                ui->btn_Cancel->setEnabled(false);
                ui->edit_DownloadName->setReadOnly(false);
            }
            else if (baOutputBuffer.length() == 0 && !balOutputBufferList.isEmpty() && baVersionResponse.indexOf("\n00\r") != -1)
            {
                //Command completed successfully
                baVersionResponse.replace("\n00\r", "");

                //Load next command
                baOutputBuffer = balOutputBufferList.takeFirst();
                if (bVSPBlocked == false)
                {
                    blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));
                    tmrResponseTimeoutTimer->start();
                }
            }
            else if (bIs2MPhySupported == false && balOutputBufferList.isEmpty() && baOutputBuffer.isEmpty() && baVersionResponse.indexOf("\t6\t") != -1 && baVersionResponse.indexOf("00", baVersionResponse.indexOf("\t6\t")) != -1)
            {
                //Got the version response - extract (1M PHY)
                QRegularExpressionMatch rexpmM1Match = rxpFreeSpace.match(baVersionResponse);

                if (rexpmM1Match.hasMatch())
                {
                    //Get the free space on the module and pass to loading function
                    nModuleFreeSpace = rexpmM1Match.captured(2).toUInt();

                    //Process file data
                    ProcessFileData(true, 0, baFileData);

                    //Enable cancel button
                    ui->btn_Cancel->setEnabled(true);
                }
                else
                {
                    ui->edit_Display->appendPlainText("No match");
                }
            }
            else if (bIs2MPhySupported == true && balOutputBufferList.isEmpty() && baOutputBuffer.isEmpty() && baVersionResponse.indexOf("\t0\t") != -1 && baVersionResponse.indexOf("\t6\t") != -1 && baVersionResponse.indexOf("00", baVersionResponse.indexOf("\t6\t")) != -1)
            {
                //Got the version response - extract (2M PHY)
                QRegularExpressionMatch rexpmM1Match = rxpDevName.match(baVersionResponse);
                QRegularExpressionMatch rexpmM2Match = rxpFreeSpace.match(baVersionResponse);

                if (rexpmM1Match.hasMatch() && rexpmM2Match.hasMatch())
                {
                    //2M PHY check - look for firmwares which may have issues
                    if (rexpmM1Match.captured(1) == "BL652")
                    {
                        //BL652 detected on phone with 2M PHY
                        QRegularExpressionMatch rexpmM3Match = rxpFirmware.match(baVersionResponse);
                        if (rexpmM3Match.captured(1) == "28.7.3.0" && QMessageBox::question(this, "Continue VSP OTA operation", "Your phone has a BT v5 radio, there is a known issue which can cause a disconnect during a VSP operation on the BL652 of this firmware version - which is more likely to occur with larger applications, do you wish to continue?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
                        {
                            //User does not want to proceed, cancel.
                            baOutputBuffer.clear();
                            balOutputBufferList.clear();
                            baVersionResponse.clear();
                            nCurrentMode = MAIN_MODE_IDLE;
                            SetLoadingStatus(STATUS_STANDBY);

                            //Show message
                            gstrToastString = "VSP OTA operation cancelled.";
                            ToastMessage(false);

                            //Disable cancel button and enable download filename edit
                            ui->btn_Cancel->setEnabled(false);
                            ui->edit_DownloadName->setReadOnly(false);
                            return;
                        }
                    }

                    //Get the free space on the module and pass to loading function
                    nModuleFreeSpace = rexpmM2Match.captured(2).toUInt();

                    //Process file data
                    ProcessFileData(true, 0, baFileData);

                    //Enable cancel button
                    ui->btn_Cancel->setEnabled(true);
                }
                else
                {
                    ui->edit_Display->appendPlainText("No match");
                }
            }
        }
        else if (nCurrentMode == MAIN_MODE_DOWNLOADING || nCurrentMode == MAIN_MODE_VERIFYING)
        {
            //
            baVersionResponse.append(baData);

            //
            if (nCurrentMode == MAIN_MODE_DOWNLOADING && baOutputBuffer.length() > 25)
            {
                //Not near the end of the output so remove the success code from the buffer
                baVersionResponse.replace("\n00\r", "");
            }

            //
            QRegularExpressionMatch rexpmM1Match = rxpErrorCode.match(baVersionResponse);

            if (rexpmM1Match.hasMatch())
            {
                //An error has occured
                nCurrentMode = MAIN_MODE_IDLE;
                baOutputBuffer.clear();
                balOutputBufferList.clear();
                baVersionResponse.clear();
                SetLoadingStatus(STATUS_STANDBY);

                //Display message
                QString ErrorMsg = elErrorLookupHandle.LookupError(rexpmM1Match.captured(1).toUInt(nullptr, 16));
                gstrToastString = QString("Error during download (").append(rexpmM1Match.captured(1)).append(") ").append(ErrorMsg);
                ToastMessage(true);

                //Disable cancel button and enable download filename edit
                ui->btn_Cancel->setEnabled(false);
                ui->edit_DownloadName->setReadOnly(false);
            }

            if (nCurrentMode == MAIN_MODE_VERIFYING)
            {
qDebug() << "Buf: " << baVersionResponse;
                if (stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE) == true)
                {
                    //Check for file listing response
                    if (baVersionResponse.indexOf("06\t") != -1 && baVersionResponse.indexOf("\n00\r", baVersionResponse.indexOf("06\t")) != -1)
                    {
                        //We have the directory listing response
                        SetLoadingStatus(STATUS_STANDBY);

                        //Check if module supports CRC verification
                        QRegularExpressionMatch rexpmM2Match = rxpCRC.match(baVersionResponse);

                        if (rexpmM2Match.hasMatch() && stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE) == true && rexpmM2Match.captured(1) != strChecksumString)
                        {
                            //Module supports CRC checking and the verification test has failed
                            gstrToastString = QString("OTA download failed - checksum failure, expected 0x").append(strChecksumString).append(" got 0x").append(rexpmM2Match.captured(1)).append(".");
                        }
                        else if (baVersionResponse.indexOf(QString("06\t").append(ui->edit_DownloadName->text()).append("\r")) == -1)
                        {
                            //File is missing...
                            gstrToastString = "OTA download failed - file is missing.";
                        }
                        else
                        {
                            //File is present
                            if (stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE) == true && rexpmM2Match.hasMatch())
                            {
                                //Module supports CRC
                                gstrToastString = "OTA download complete - file & CRC verified!";
                            }
                            else
                            {
                                //Module does not support CRC
                                gstrToastString = "OTA download complete - file verified (CRC unsupported)!";
                            }

                            //Check if the device should be disonnected or reset
                            if (stgSettingsHandle->GetUInt(SETTINGS_KEY_DOWNLOADACTION) == DOWNLOAD_ACTION_DISCONNECT)
                            {
                                //Disconnect from device
                                if (lecBLEController != NULL && bDisconnectActive == false)
                                {
                                    gstrToastString += " Disconnecting...";
                                    bDisconnectActive = true;
                                    lecBLEController->disconnectFromDevice();
                                }
                            }
                            else if (stgSettingsHandle->GetUInt(SETTINGS_KEY_DOWNLOADACTION) == DOWNLOAD_ACTION_RESTART)
                            {
                                //Restart device
                                gstrToastString += " Restarting...";
                                blesvcVSPService->writeCharacteristic(blechrRXChar, "atz\r");
                            }
                        }

                        //Show the message
                        ToastMessage(false);

                        //Clean up
                        delete strChecksumString;
                        strChecksumString = NULL;
                        baOutputBuffer.clear();
                        balOutputBufferList.clear();
                        nCurrentMode = MAIN_MODE_IDLE;
                        SetLoadingStatus(STATUS_STANDBY);

                        //Disable cancel button and enable download filename edit
                        ui->btn_Cancel->setEnabled(false);
                        ui->edit_DownloadName->setReadOnly(false);
                    }
                }
                else
                {
                    //Check if data has finished transferring
                    if (baVersionResponse.indexOf("\n10\t1\t") != -1)
                    {
                        //Response received - finished
                        gstrToastString = "OTA download complete!";
                        ToastMessage(false);

                        //Check if the device should be disconnected or reset
                        if (stgSettingsHandle->GetBool(SETTINGS_KEY_DOWNLOADACTION) == DOWNLOAD_ACTION_DISCONNECT)
                        {
                            //Disconnect from device
                            if (lecBLEController != NULL && bDisconnectActive == false)
                            {
                                gstrToastString += " Disconnecting...";
                                bDisconnectActive = true;
                                lecBLEController->disconnectFromDevice();
                            }
                            else if (stgSettingsHandle->GetUInt(SETTINGS_KEY_DOWNLOADACTION) == DOWNLOAD_ACTION_RESTART)
                            {
                                //Restart device
                                gstrToastString += " Restarting...";
                                blesvcVSPService->writeCharacteristic(blechrRXChar, "atz\r");
                            }
                        }

                        //Clean up
                        baOutputBuffer.clear();
                        balOutputBufferList.clear();
                        nCurrentMode = MAIN_MODE_IDLE;
                        SetLoadingStatus(STATUS_STANDBY);

                        //Disable cancel button and enable download filename edit
                        ui->btn_Cancel->setEnabled(false);
                        ui->edit_DownloadName->setReadOnly(false);
                    }
                }
            }
        }
        else if (nCurrentMode == MAIN_MODE_QUERY)
        {
            //Waiting for module information
            baVersionResponse.append(baData);

            //Search for errors
            QRegularExpressionMatch rexpmM1Match = rxpErrorCode.match(baVersionResponse);
            if (rexpmM1Match.hasMatch())
            {
                //Error detected
                nCurrentMode = MAIN_MODE_IDLE;
                baOutputBuffer.clear();
                balOutputBufferList.clear();
                baVersionResponse.clear();
                SetLoadingStatus(STATUS_STANDBY);

                //Display message
                QString ErrorMsg = elErrorLookupHandle.LookupError(rexpmM1Match.captured(1).toUInt(nullptr, 16));
                gstrToastString = QString("Error during module query (").append(rexpmM1Match.captured(1)).append(") ").append(ErrorMsg);
                ToastMessage(true);

                //Disable cancel button and enable download filename edit
                ui->btn_Cancel->setEnabled(false);
                ui->edit_DownloadName->setReadOnly(false);
            }
            else if (baOutputBuffer.length() == 0 && !balOutputBufferList.isEmpty() && baVersionResponse.indexOf("\n00\r") != -1)
            {
                //Command completed successfully
                baVersionResponse.replace("\n00\r", "");

                //Load next command
                baOutputBuffer = balOutputBufferList.takeFirst();
                if (bVSPBlocked == false)
                {
                    blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));
                    tmrResponseTimeoutTimer->start();
                }
            }

            if (baVersionResponse.indexOf("\t0\t") != -1 && baVersionResponse.indexOf("\t6\t") != -1 && baVersionResponse.indexOf("00", baVersionResponse.indexOf("\t6\t")) != -1)
            {
                //Got the version response - extract
                QRegularExpressionMatch rexpmM2Match = rxpDevName.match(baVersionResponse);
                QRegularExpressionMatch rexpmM3Match = rxpFirmware.match(baVersionResponse);
                QRegularExpressionMatch rexpmM4Match = rxpFreeSpace.match(baVersionResponse);

                if (rexpmM2Match.hasMatch() && rexpmM3Match.hasMatch() && rexpmM4Match.hasMatch())
                {
                    //Version information received
                    if (stgSettingsHandle->GetBool(SETTINGS_KEY_CHECKFWVERSION) == true)
                    {
                        //Check if device is on latest firmware
                        nCurrentMode = MAIN_MODE_FIRMWAREVERSION;
                        dwnDownloaderHandle->CheckLatestFirmware(rexpmM2Match.captured(1), rexpmM3Match.captured(1));
                        gstrToastString = "Received module information, checking for latest firmware...";
                        ToastMessage(false);
                    }
                    else
                    {
                        //Do not check for latest firmware
                        nCurrentMode = MAIN_MODE_IDLE;
                        QMessageBox::information(this, "Module Information", QString("The connected device is a ").append(rexpmM2Match.captured(1)).append(" module on firmware version ").append(rexpmM3Match.captured(1)).append(".\r\nFlash space available: ").append(rexpmM4Match.captured(2)).append("/").append(rexpmM4Match.captured(1)).append(" bytes (").append(QString::number(rexpmM4Match.captured(2).toUInt()*100/rexpmM4Match.captured(1).toUInt())).append("%).").append((rexpmM2Match.captured(1) == "BL652" && rexpmM3Match.captured(1) == "28.7.3.0" && bIs2MPhySupported == true ? "\r\n\r\nPlease note: VSP/OTA to this device is likely to fail due to your device having a Bluetooth v5 radio with support for 2M PHY." : "")));
                        balOutputBufferList.clear();
                        baOutputBuffer.clear();
                        baVersionResponse.clear();
                        UpdateDisplay();
                    }
                }
                else
                {
                    ui->edit_Display->appendPlainText("No match");
                    UpdateDisplay();
                    nCurrentMode = MAIN_MODE_IDLE;
                }

                //Disable cancel button
                ui->btn_Cancel->setEnabled(false);
            }
        }
    }
    else
    {
        if (bHasModem == true)
        {
            if (lecCharacteristic == blechrMOChar)
            {
#ifdef ENABLE_DEBUG
                qDebug() << baData.toHex();
#endif
                if (baData.at(0) == 0x01)
                {
                    //Go
                    bVSPBlocked = false;
                    blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));
                }
                else if (baData.at(0) == 0x00)
                {
                    //Stop
                    bVSPBlocked = true;
                }
            }
        }
        else
        {
            bVSPBlocked = false;
            blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::VSPServiceCharacteristicWritten(
    QLowEnergyCharacteristic lecCharacteristic,
    QByteArray baData
    )
{
    //
    if (lecCharacteristic == blechrRXChar)
    {
        unWrittenBytes = unWrittenBytes + baData.length();
        if (stgSettingsHandle->GetBool(SETTINGS_KEY_SKIPDLDISPLAY) == false)
        {
            //Append data to receive buffer display
            baRecBuffer.append(baData);
            TrucateRecBuffer();
        }
//        qDebug() << "Wrote: " << baData;

        if (tmrResponseTimeoutTimer->isActive())
        {
            //Restart timeout timer
            tmrResponseTimeoutTimer->stop();
            tmrResponseTimeoutTimer->start();
        }

        //Send data chunk
        if (nCurrentMode == MAIN_MODE_VERSION || nCurrentMode == MAIN_MODE_SPACECHECK)
        {
            baOutputBuffer = baOutputBuffer.mid(baData.length());
            if (bVSPBlocked == false && baOutputBuffer.length() > 0)
            {
                //
                blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));
            }
        }
        else if (nCurrentMode == MAIN_MODE_DOWNLOADING)
        {
            unTotalSizeSent += baData.length();
            baOutputBuffer = baOutputBuffer.mid(baData.length());
            if (bVSPBlocked == false && baOutputBuffer.length() > 0)
            {
                //
                blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));
            }
            else if (baOutputBuffer.length() == 0)
            {
                //
                nCurrentMode = MAIN_MODE_VERIFYING;
                if (balOutputBufferList.count() > 0)
                {
                    //Send next command
                    baVersionResponse.clear();
                    baOutputBuffer = balOutputBufferList.takeFirst();
                    blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));
                }
            }

            if (!tmrDisplayUpdateTimer->isActive())
            {
                tmrDisplayUpdateTimer->start();
            }
        }
        else if (nCurrentMode == MAIN_MODE_VERIFYING)
        {
            unTotalSizeSent += baData.length();
            baOutputBuffer = baOutputBuffer.mid(baData.length());
            if (bVSPBlocked == false && baOutputBuffer.length() > 0)
            {
                //
                blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));
            }
            else if (baOutputBuffer.length() == 0)
            {
                //
                if (balOutputBufferList.count() > 0)
                {
                    //Send next command
                    baVersionResponse.clear();
                    baOutputBuffer = balOutputBufferList.takeFirst();
                    blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));
                }
            }

//            if (!tmrDisplayUpdateTimer->isActive())
//            {
//                tmrDisplayUpdateTimer->start();
//            }
        }
        else if (nCurrentMode == MAIN_MODE_QUERY)
        {
            baOutputBuffer = baOutputBuffer.mid(baData.length());
            if (bVSPBlocked == false && baOutputBuffer.length() > 0)
            {
                //
                blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));
            }
        }
        UpdateTxRx();
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::VSPServiceDescriptorWritten(
    QLowEnergyDescriptor ledDescriptor,
    QByteArray baData
    )
{
    //
    if (bHasModem == true)
    {
        if (ledDescriptor == blechrMOChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration))
        {
            baRecBuffer.append("(Now ready to send/receive!)\n");
            UpdateDisplay();
        }
    }
#ifdef ENABLE_DEBUG
    qDebug() << "desc." << ledDescriptor.name() << baData;
#endif
}

//=============================================================================
//=============================================================================
void
MainWindow::VSPServiceError(
    QLowEnergyService::ServiceError nErrorCode
    )
{
    //Service error
#ifdef ENABLE_DEBUG
    qDebug() << "BLE Service error: " << nErrorCode;
#endif
    if (nErrorCode != QLowEnergyService::NoError && nErrorCode != QLowEnergyService::OperationError)
    {
        if (nErrorCode == QLowEnergyService::OperationError)
        {
            //Operation attempted whilst service was not ready
            gstrToastString = "Operation error (invalid action), please report this issue to the developer.";
        }
        else if (nErrorCode == QLowEnergyService::CharacteristicReadError)
        {
            //Failed to read characteristic
            gstrToastString = "Characteristic read failed - Ensure device is in hardware command VSP mode, try reconnecting again then disable/re-enable Bluetooth from settings to clear the cache.";
        }
        else if (nErrorCode == QLowEnergyService::CharacteristicWriteError)
        {
            //Failed to write characteristic
            gstrToastString = "Characteristic write failed - Ensure device is in hardware command VSP mode, try reconnecting again then disable/re-enable Bluetooth from settings to clear the cache.";
            if (stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE) > 20)
            {
                //Larger packet size set, add message about
                gstrToastString += "\r\nA larger packet size is set, ensure you have correctly configured the module and that it supports this size packet.";
            }
        }
        else if (nErrorCode == QLowEnergyService::DescriptorReadError)
        {
            //Failed to read descriptor
            gstrToastString = "Descriptor read failed - Ensure device is in hardware command VSP mode, try reconnecting again then disable/re-enable Bluetooth from settings to clear the cache.";
        }
        else if (nErrorCode == QLowEnergyService::DescriptorWriteError)
        {
            //Failed to write descriptor
            gstrToastString = "Descriptor write failed - Ensure device is in hardware command VSP mode, try reconnecting again then disable/re-enable Bluetooth from settings to clear the cache.";
        }
        else if (nErrorCode == QLowEnergyService::UnknownError)
        {
            //Unknown error
            gstrToastString = "Unknown Bluetooth service error occured.";
        }

        //Disconnect from device
        if (lecBLEController != NULL && bDisconnectActive == false)
        {
            bDisconnectActive = true;
            lecBLEController->disconnectFromDevice();
        }

        //Clear buffers and set mode to idle
        baVersionResponse.clear();
        baOutputBuffer.clear();
        balOutputBufferList.clear();
        nCurrentMode = MAIN_MODE_IDLE;

        if (tmrResponseTimeoutTimer->isActive())
        {
            //Stop timeout timer
            tmrResponseTimeoutTimer->stop();
        }

        //Show message
        ToastMessage(true);

        //Disable cancel button and enable download filename edit
        ui->btn_Cancel->setEnabled(false);
        ui->edit_DownloadName->setReadOnly(false);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::VSPServiceStateChanged(
    QLowEnergyService::ServiceState nNewState
    )
{
    //Server state changed
#ifdef ENABLE_DEBUG
    qDebug() << "State: " << nNewState;
#endif
    if (nNewState == QLowEnergyService::ServiceDiscovered)
    {
        QLowEnergyService *service = qobject_cast<QLowEnergyService *>(sender());
#ifdef ENABLE_DEBUG
        qDebug() << "Service: " << service->serviceUuid() << ", looking for: " << QBluetoothUuid(stgSettingsHandle->GetString(SETTINGS_KEY_UUID));
#endif
        if (service && service->serviceUuid() == QBluetoothUuid(stgSettingsHandle->GetString(SETTINGS_KEY_UUID)))
        {
#ifdef ENABLE_DEBUG
            qDebug() << "Tx: " << QString(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).left(4)).append(stgSettingsHandle->GetString(SETTINGS_KEY_TX_OFFSET)).append(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).right(28));
            qDebug() << "Rx: " << QString(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).left(4)).append(stgSettingsHandle->GetString(SETTINGS_KEY_RX_OFFSET)).append(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).right(28));
#endif
            blechrTXChar = blesvcVSPService->characteristic(QBluetoothUuid(QString(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).left(4)).append(stgSettingsHandle->GetString(SETTINGS_KEY_TX_OFFSET)).append(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).right(28))));
            blechrRXChar = blesvcVSPService->characteristic(QBluetoothUuid(QString(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).left(4)).append(stgSettingsHandle->GetString(SETTINGS_KEY_RX_OFFSET)).append(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).right(28))));
#ifdef ENABLE_DEBUG
            qint8 unTmp = 0;
            while (unTmp < service->characteristics().count())
            {
                qDebug() << "Char: " << unTmp;
                qDebug() << service->characteristics().at(unTmp).uuid();
                qDebug() << service->characteristics().at(unTmp).handle();
                ++unTmp;
            }
            qDebug() << blechrRXChar.handle();
            qDebug() << blechrRXChar.uuid();
#endif

            if (blesvcVSPService->characteristic(QBluetoothUuid(QString(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).left(4)).append(stgSettingsHandle->GetString(SETTINGS_KEY_MO_OFFSET)).append(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).right(28)))).isValid())
            {
                //VSP active
#ifdef ENABLE_DEBUG
                qDebug() << "-- MODEM -- ";
#endif
                bHasModem = true;
                blechrMOChar = blesvcVSPService->characteristic(QBluetoothUuid(QString(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).left(4)).append(stgSettingsHandle->GetString(SETTINGS_KEY_MO_OFFSET)).append(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).right(28))));
                blechrMIChar = blesvcVSPService->characteristic(QBluetoothUuid(QString(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).left(4)).append(stgSettingsHandle->GetString(SETTINGS_KEY_MI_OFFSET)).append(stgSettingsHandle->GetString(SETTINGS_KEY_UUID).right(28))));
            }
            else
            {
                //
#ifdef ENABLE_DEBUG
                qDebug() << "-- NO MODEM -- ";
#endif
                bHasModem = false;
            }
            if (!blechrTXChar.isValid())
            {
#ifdef ENABLE_DEBUG
                qDebug() << "TX char missing.";
#endif
                if (bDisconnectActive == false)
                {
                    bDisconnectActive = true;
                    lecBLEController->disconnectFromDevice();
                }
            }

            if (!blechrRXChar.isValid())
            {
#ifdef ENABLE_DEBUG
                qDebug() << "RX char missing.";
#endif
                if (bDisconnectActive == false)
                {
                    bDisconnectActive = true;
                    lecBLEController->disconnectFromDevice();
                }
            }

            if (bHasModem == true)
            {
                if (!blechrMOChar.isValid())
                {
#ifdef ENABLE_DEBUG
                    qDebug() << "MO char missing.";
#endif
                    if (bDisconnectActive == false)
                    {
                        bDisconnectActive = true;
                        lecBLEController->disconnectFromDevice();
                    }
                }

                if (!blechrMIChar.isValid())
                {
#ifdef ENABLE_DEBUG
                    qDebug() << "MI char missing.";
#endif
                    if (bDisconnectActive == false)
                    {
                        bDisconnectActive = true;
                        lecBLEController->disconnectFromDevice();
                    }
                }
            }

            const QLowEnergyDescriptor TXDesc = blechrTXChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);

            if (!TXDesc.isValid())
            {
#ifdef ENABLE_DEBUG
                qDebug() << "TX desc missing.";
#endif
                if (bDisconnectActive == false)
                {
                    bDisconnectActive = true;
                    lecBLEController->disconnectFromDevice();
                }
            }

            if (bHasModem == true)
            {
                const QLowEnergyDescriptor MODesc = blechrMOChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);

                if (!MODesc.isValid())
                {
#ifdef ENABLE_DEBUG
                    qDebug() << "MO desc missing.";
#endif
                    if (bDisconnectActive == false)
                    {
                        bDisconnectActive = true;
                        lecBLEController->disconnectFromDevice();
                    }
                }
                blesvcVSPService->writeDescriptor(MODesc, QByteArray::fromHex("0100"));
            }

            //Enable Tx descriptor notifications
            blesvcVSPService->writeDescriptor(TXDesc, QByteArray::fromHex("0100"));

            //Set status to idle
            bVSPBlocked = false;
            baVersionResponse.clear();
            baOutputBuffer.clear();
            balOutputBufferList.clear();
            nCurrentMode = MAIN_MODE_IDLE;

            //No longer busy
            SetLoadingStatus(STATUS_STANDBY);

            if (tmrResponseTimeoutTimer->isActive())
            {
                //Stop timeout timer
                tmrResponseTimeoutTimer->stop();
            }

            //Disable cancel button and enable download filename edit
            ui->btn_Cancel->setEnabled(false);
            ui->edit_DownloadName->setReadOnly(false);
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::ConnectToDevice(
    QBluetoothDeviceInfo bdiDeviceInfo
    )
{
    //Callback for connecting to a BLE device
    if (nCurrentMode == MAIN_MODE_IDLE)
    {
        //Idle, allow connection
        if (ddaDiscoveryAgent->isActive())
        {
            //Stop scan
            ddaDiscoveryAgent->stop();
        }

        //Setup the connection request
        lecBLEController = new QLowEnergyController(bdiDeviceInfo);
//        lecBLEController->setRemoteAddressType(QLowEnergyController::PublicAddress);
        connect(lecBLEController, SIGNAL(connected()), this, SLOT(BLEConnected()));
        connect(lecBLEController, SIGNAL(disconnected()), this, SLOT(BLEDisconnected()));
        connect(lecBLEController, SIGNAL(discoveryFinished()), this, SLOT(BLEDiscoveryFinished()));
        connect(lecBLEController, SIGNAL(error(QLowEnergyController::Error)), this, SLOT(BLEError(QLowEnergyController::Error)));
        connect(lecBLEController, SIGNAL(serviceDiscovered(QBluetoothUuid)), this, SLOT(BLEDiscovered(QBluetoothUuid)));
        connect(lecBLEController, SIGNAL(stateChanged(QLowEnergyController::ControllerState)), this, SLOT(BLEStateChanged(QLowEnergyController::ControllerState)));
        lecBLEController->connectToDevice();

        //Set loading image to busy
        SetLoadingStatus(STATUS_LOADING);
        nCurrentMode = MAIN_MODE_CONNECTING;
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Disconnect_clicked(
    )
{
    //Disconnect button clicked
    if (lecBLEController != NULL && bDisconnectActive == false)
    {
        bDisconnectActive = true;
        lecBLEController->disconnectFromDevice();
    }
    ui->btn_Disconnect->setEnabled(false);
    ui->btn_Download->setEnabled(false);
    ui->btn_ModuleInfo->setEnabled(false);
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Scan_clicked(
    )
{
    //Scan for devices
    if (lecBLEController == NULL)
    {
        if (nCurrentMode == MAIN_MODE_IDLE)
        {
            dlgScanDialog->ClearDevices();
            dlgScanDialog->SetStatus(STATUS_LOADING);
            dlgScanDialog->show();
            if (stgSettingsHandle->GetBool(SETTINGS_KEY_COMPATIBLESCAN) == false)
            {
                //Normal scanning
                ddaDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
#ifdef ENABLE_DEBUG
qDebug() << "Normal scanning started...";
#endif
            }
            else
            {
                //Compatible scanning (fixed issues on Nexus 9 for unknown reasons)
                ddaDiscoveryAgent->start();
#ifdef ENABLE_DEBUG
qDebug() << "Compatible scanning started...";
#endif
            }
            SetLoadingStatus(STATUS_LOADING);
        }
    }
    else
    {
        if (bDisconnectActive == false)
        {
            bDisconnectActive = true;
            lecBLEController->disconnectFromDevice();
        }
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::ClearVar(
    )
{
    //Clears up the BLE controller
    disconnect(this, SLOT(BLEConnected()));
    disconnect(this, SLOT(BLEDisconnected()));
    disconnect(this, SLOT(BLEDiscoveryFinished()));
    disconnect(this, SLOT(BLEError(QLowEnergyController::Error)));
    disconnect(this, SLOT(BLEDiscovered(QBluetoothUuid)));
    disconnect(this, SLOT(BLEStateChanged(QLowEnergyController::ControllerState)));
    delete lecBLEController;
    lecBLEController = NULL;
}

//=============================================================================
//=============================================================================
void
MainWindow::UpdateDisplay(
    )
{
    //Updates the display
    ui->edit_Display->setPlainText(baRecBuffer);
    ui->edit_Display->verticalScrollBar()->setSliderPosition(ui->edit_Display->verticalScrollBar()->maximum());
}

//=============================================================================
//=============================================================================
void
MainWindow::UpdateTxRx(
    )
{
    //Updates the Tx/Rx count message
    ui->statusBar->showMessage(QString(" ").repeated(nStatusBarSpaces).append("Tx: ").append(QString::number(unWrittenBytes)).append(", Rx: ").append(QString::number(unRecDatSize)).append(", Tx Remaining: ").append(QString::number(unTotalAppSize - unTotalSizeSent)).append(", ").append(QString::number((unTotalAppSize == 0 ? 0 : (unTotalSizeSent * 100 / unTotalAppSize)))).append("% complete."));
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Clear_clicked(
    )
{
    //Clears the display
    unWrittenBytes = 0;
    unRecDatSize = 0;
    baRecBuffer.clear();
    UpdateDisplay();
    UpdateTxRx();
}

//=============================================================================
//=============================================================================
void
MainWindow::ProcessFileData(
    bool bSuccess,
    qint16 nErrorCode,
    QByteArray baFileData
    )
{
    //Callback for processing file data
    if (lecBLEController != NULL && bDisconnectActive == false)
    {
        if (bSuccess == false)
        {
            //Failed to select file
            if (nErrorCode == DOWNLOAD_FILESIZE_ERROR)
            {
                //Filesize not valid
                gstrToastString = QString("Invalid filesize, must be between ").append(QString::number(FILESIZE_MIN)).append(" - ").append(QString::number(FILESIZE_MAX)).append(" bytes.");
                ToastMessage(false);
            }
            else if (nErrorCode == DOWNLOAD_JSON_ERROR)
            {
                //JSON data not valid
                gstrToastString = "Error parsing HTTP response: JSON data not valid.";
                ToastMessage(false);
            }
            else if (nErrorCode == DOWNLOAD_SERVER_ERROR)
            {
                //Server error
                gstrToastString = QString("Server error: ").append(baFileData);
                ToastMessage(true);
            }
            else if (nErrorCode == DOWNLOAD_XCOMPILE_ERROR)
            {
                //Error XCompiling application
                QMessageBox::critical(this, "Error with XCompilation", baFileData, QMessageBox::Ok, QMessageBox::NoButton);
            }
            else if (nErrorCode == DOWNLOAD_GENERAL_ERROR)
            {
                //General error
                gstrToastString = QString("HTTP Download error: ").append(baFileData);
                ToastMessage(false);
            }
            else if (nErrorCode == DOWNLOAD_UNSUPPORTED_ERROR)
            {
                //Unsupported device error
                gstrToastString = QString("XCompile error: ").append(baFileData);
                ToastMessage(true);
            }
            else if (nErrorCode == DOWNLOAD_UNKNOWN_ERROR)
            {
                //Unsupported device error
                gstrToastString = QString("Unknown error - is your ISP altering your network traffic?");
                ToastMessage(true);
            }
            else
            {
                //HTTP failure response code
                gstrToastString = QString("HTTP error code: ").append(QString::number(nErrorCode));
                ToastMessage(false);
            }
            baOutputBuffer.clear();
            balOutputBufferList.clear();
            nCurrentMode = MAIN_MODE_IDLE;
            SetLoadingStatus(STATUS_STANDBY);

            if (tmrResponseTimeoutTimer->isActive())
            {
                //Stop timeout timer
                tmrResponseTimeoutTimer->stop();
            }

            //Disable cancel button and enable download filename edit
            ui->btn_Cancel->setEnabled(false);
            ui->edit_DownloadName->setReadOnly(false);
            return;
        }
        else if (baFileData.isNull() || baFileData.isEmpty())
        {
            //No file selected
            gstrToastString = "File selection cancelled: returned data is null.";
            ToastMessage(false);
            baOutputBuffer.clear();
            balOutputBufferList.clear();
            nCurrentMode = MAIN_MODE_IDLE;
            SetLoadingStatus(STATUS_STANDBY);

            if (tmrResponseTimeoutTimer->isActive())
            {
                //Stop timeout timer
                tmrResponseTimeoutTimer->stop();
            }

            //Disable cancel button and enable download filename edit
            ui->btn_Cancel->setEnabled(false);
            ui->edit_DownloadName->setReadOnly(false);
            return;
        }

#ifdef ENABLE_DEBUG
qDebug() << "Got: " << baFileData;
#endif

        //Check available module space, if enabled
        if (stgSettingsHandle->GetBool(SETTINGS_KEY_CHECKFREESPACE) == true && baFileData.length() > nModuleFreeSpace)
        {
            //Insufficient space available on the module to download this application, check if the user is sure they want to download it
            if (QMessageBox::question(this, "Insufficient module space", QString("There is insufficient storage space available on the module, ").append(QString::number(baFileData.length())).append(" bytes are required but only ").append(QString::number(nModuleFreeSpace)).append(" bytes are free therefore the OTA will likely fail.\r\n\r\nAre you sure you want to continue with this operation?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
            {
                //User does not want to continue, cancel operation
                gstrToastString = "Insufficient module storage space, OTA cancelled!";

                //Show message
                ToastMessage(false);

                //Set status back to being idle
                baOutputBuffer.clear();
                balOutputBufferList.clear();
                nCurrentMode = MAIN_MODE_IDLE;
                SetLoadingStatus(STATUS_STANDBY);

                if (tmrResponseTimeoutTimer->isActive())
                {
                    //Stop timeout timer
                    tmrResponseTimeoutTimer->stop();
                }

                //Disable cancel button and enable download filename edit
                ui->btn_Cancel->setEnabled(false);
                ui->edit_DownloadName->setReadOnly(false);
                return;
            }
        }

        //Check if the checksum is being calculated
        if (stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE) == true)
        {
            chkChecksum = new ChecksumCalculator();
        }

        //One byte at a time, convert the data to hex
        QString gstrHexData = "";
        int i = 0;

        while (i < baFileData.length())
        {
            quint8 ThisByte = baFileData[i];
            if (stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE) == true)
            {
                //Add to checksum
                chkChecksum->AddByte(ThisByte);
            }

            QString strThisHex;
            strThisHex.setNum(ThisByte, 16);
            if (strThisHex.length() == 1)
            {
                //Expand to 2 characters
                strThisHex.prepend("0");
            }

            //Add the hex character to the string
//            qDebug() << "Add: " << strThisHex;
            gstrHexData.append(strThisHex.toUpper());
            ++i;
        }

        //Clean up
        if (stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE) == true)
        {
            strChecksumString = new QString(chkChecksum->GetChecksumHexString());
            delete chkChecksum;
            chkChecksum = NULL;
        }

        //Clear output buffers
        baOutputBuffer.clear();
        balOutputBufferList.clear();

        if (stgSettingsHandle->GetBool(SETTINGS_KEY_DELFILE) == true)
        {
            //Add delete file command
            baOutputBuffer.append(QString("AT+del \"").append(ui->edit_DownloadName->text()).append("\"\r"));
        }

        //Create the file writing data
        baOutputBuffer.append(QString("AT+fow \"").append(ui->edit_DownloadName->text()).append("\"\r"));
        while (gstrHexData.length() > 0)
        {
            baOutputBuffer.append(QString("AT+fwrh \"").append(gstrHexData.length() > 56 ? gstrHexData.left(56) : gstrHexData).append("\"\r"));
            if (gstrHexData.length() > 56)
            {
                gstrHexData = gstrHexData.right(gstrHexData.length() - 56);
            }
            else
            {
                gstrHexData = "";
            }
        }

        //Closing the file
        balOutputBufferList.append("at+fcl\r");

        if (stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE) == true)
        {
            //Add commands for verifying file exists and was transferred successfully
            balOutputBufferList.append("at i 0xc12c\r");
            balOutputBufferList.append("at+dir\r");
        }
        else
        {
            //Send dummy command
            balOutputBufferList.append("at i 1\r");
        }

        //Set variables for progress tracking and clear any received responses - note that the total app size is the size of the encoded buffer data, not just purely the application size
        unTotalSizeSent = 0;
        unTotalAppSize = baOutputBuffer.length();
        baVersionResponse.clear();

        //Add in the length of all extra commands
        qint8 abc = 0;
        while (abc < balOutputBufferList.length())
        {
            unTotalAppSize += balOutputBufferList[abc].length();
            ++abc;
        }

        //Set mode to downloading application
        nCurrentMode = MAIN_MODE_DOWNLOADING;

#ifdef ENABLE_DEBUG
qDebug() << "nCurrentMode is now: " << nCurrentMode << ", Data: " << baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE));
#endif

        //Start writing the data
        blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));

        //Enable cancel button and disable download filename edit
        ui->btn_Cancel->setEnabled(true);
        ui->edit_DownloadName->setReadOnly(true);

        //Output message
        gstrToastString = "Transferring OTA data...";
        ToastMessage(false);
    }
    else
    {
        //
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Download_clicked(
    )
{
    //Download button clicked
    if (lecBLEController != NULL && nCurrentMode == MAIN_MODE_IDLE)
    {
        if (nSelectedFileType == FILE_TYPE_LOCALFILE)
        {
            //Local file
            if (strLocalFilename.isNull() || strLocalFilename.isEmpty())
            {
                //No file selected
#ifdef ENABLE_DEBUG
                qDebug() << "No file selected.";
#endif
            }
#ifndef Q_OS_ANDROID
            else if (!QFile::exists(strLocalFilename))
            {
                //File doesn't exist
#ifdef ENABLE_DEBUG
                qDebug() << "File doesn't exist.";
#endif
            }
#endif
            else
            {
                //Is this a source file or compiled application
                if (stgSettingsHandle->GetBool(SETTINGS_KEY_ONLINEXCOMP) == true && (strLocalFilename.right(3).toLower() == ".sb" || strLocalFilename.right(4).toLower() == ".txt"))
                {
                    //Source file and XCompilation enabled, load file data into byte array
#ifndef Q_OS_ANDROID
                    baFileData.clear();
                    QFile FileHandle;
                    FileHandle.setFileName(strLocalFilename);
                    if (!FileHandle.open(QFile::ReadOnly | QFile::Text))
                    {
                        //Failed ot open file for reading
#ifdef ENABLE_DEBUG
                        qDebug() << "Failed to open file.";
#endif
                        return;
                    }

                    //Read contents into byte array
                    baFileData = FileHandle.readAll();

                    //Close file handle
                    FileHandle.close();
#endif
                    //Set status to busy
                    SetLoadingStatus(STATUS_LOADING);

#ifdef ENABLE_DEBUG
                    qDebug() << "Read file data (" << baFileData.length() << ").";
#endif

                    //Fetch details from module
                    nCurrentMode = MAIN_MODE_VERSION;
                    baVersionResponse.clear();
                    balOutputBufferList.clear();
                    balOutputBufferList.append("at i 0\r\n");
                    if (bIs2MPhySupported == true)
                    {
                        //Check module firmware for 2M PHY devices
                        balOutputBufferList.append("at i 3\r\n");
                    }
                    if (stgSettingsHandle->GetBool(SETTINGS_KEY_CHECKFREESPACE) == true)
                    {
                        //Check if there is sufficient free space
                        balOutputBufferList.append("at i 6\r\n");
                    }
                    balOutputBufferList.append("at i 13\r\n");
                    baOutputBuffer = balOutputBufferList.takeFirst();
                    blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));

                    //Show message
                    gstrToastString = "Fetching details for XCompilation from module...";
                    ToastMessage(false);

                    //Start response timeout timer
                    tmrResponseTimeoutTimer->start();

                    //Enable cancel button and disable download filename edit
                    ui->btn_Cancel->setEnabled(true);
                    ui->edit_DownloadName->setReadOnly(true);
                }
                else
                {
                    //Application file or XCompilation disabled, load directly to module. Set status to busy
                    SetLoadingStatus(STATUS_LOADING);

                    //Fetch details from module
                    nCurrentMode = MAIN_MODE_SPACECHECK;
                    baVersionResponse.clear();
                    balOutputBufferList.clear();
                    if (bIs2MPhySupported == true)
                    {
                        //Check module information for 2M PHY devices
                        balOutputBufferList.append("at i 0\r\n");
                        balOutputBufferList.append("at i 3\r\n");
                    }
                    if (stgSettingsHandle->GetBool(SETTINGS_KEY_CHECKFREESPACE) == true)
                    {
                        //Check if there is sufficient free space
                        balOutputBufferList.append("at i 6\r\n");
                    }

                    if (!balOutputBufferList.isEmpty())
                    {
                        //Commands to send
                        baOutputBuffer = balOutputBufferList.takeFirst();
                        blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));

                        //Show message
                        gstrToastString = "Checking module storage space...";
                        ToastMessage(false);
                    }
                    else
                    {
                        //Load the file directly to the module
                        ProcessFileData(true, 0, baFileData);
                    }

                    //Start response timeout timer
                    tmrResponseTimeoutTimer->start();

                    //Enable cancel button and disable download filename edit
                    ui->btn_Cancel->setEnabled(true);
                    ui->edit_DownloadName->setReadOnly(true);
                }
            }
        }
        else if (nSelectedFileType == FILE_TYPE_REMOTEURL)
        {
            //Remote file
            nCurrentMode = MAIN_MODE_ONLINE_DOWNLOAD;
            dwnDownloaderHandle->DownloadFile(strLocalFilename);

            //Enable cancel button and disable download filename edit
            ui->btn_Cancel->setEnabled(true);
            ui->edit_DownloadName->setReadOnly(true);
        }
#ifndef Q_OS_ANDROID
        else if (nSelectedFileType == FILE_TYPE_DROPBOX)
        {
            //Dropbox file
        }
#endif
        else
        {
            //No file selected
            gstrToastString = "No file selected, please click the '...' button to select a file.";
            ToastMessage(false);
        }
    }
    else if (lecBLEController == NULL)
    {
        //Not connected to a device
        gstrToastString = "Not connected to a VSP BLE device, please click the 'Scan' button to list available devices.";
        ToastMessage(false);
    }
    else if (nCurrentMode == MAIN_MODE_IDLE)
    {
        //Application is currently busy
        gstrToastString = "Currently busy processing OTA upgrade...";
        ToastMessage(false);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::CancelScan(
    )
{
    //Cancel scanning for devices
    if (ddaDiscoveryAgent->isActive())
    {
        //Scan is active, stop scanning
        ddaDiscoveryAgent->stop();
        SetLoadingStatus(STATUS_STANDBY);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Settings_clicked(
    )
{
    //Settings button clicked
    if (nCurrentMode == MAIN_MODE_IDLE)
    {
        //Not busy - show application settings
        dlgSettingsView = new SettingsDialog(this);
        connect(dlgSettingsView, SIGNAL(SaveSettings(QString,QString,QString,QString,QString,bool,
#ifdef Q_OS_ANDROID
            bool,
#endif
            quint8,bool,bool,quint8,bool,quint8,bool,bool,bool,bool)), this, SLOT(SettingsUpdated(QString,QString,QString,QString,QString,bool,
#ifdef Q_OS_ANDROID
            bool,
#endif
            quint8,bool,bool,quint8,bool,quint8,bool,bool,bool,bool)));
        dlgSettingsView->SetValues(stgSettingsHandle->GetString(SETTINGS_KEY_UUID), stgSettingsHandle->GetString(SETTINGS_KEY_TX_OFFSET), stgSettingsHandle->GetString(SETTINGS_KEY_RX_OFFSET), stgSettingsHandle->GetString(SETTINGS_KEY_MO_OFFSET), stgSettingsHandle->GetString(SETTINGS_KEY_MI_OFFSET), stgSettingsHandle->GetBool(SETTINGS_KEY_RESTRICTUUID),
#ifdef Q_OS_ANDROID
            stgSettingsHandle->GetBool(SETTINGS_KEY_COMPATIBLESCAN),
#endif
            stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE), stgSettingsHandle->GetBool(SETTINGS_KEY_DELFILE), stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE), stgSettingsHandle->GetUInt(SETTINGS_KEY_DOWNLOADACTION), stgSettingsHandle->GetBool(SETTINGS_KEY_SKIPDLDISPLAY), stgSettingsHandle->GetUInt(SETTINGS_KEY_SCROLLBACKSIZE), stgSettingsHandle->GetBool(SETTINGS_KEY_ONLINEXCOMP), stgSettingsHandle->GetBool(SETTINGS_KEY_SSL), stgSettingsHandle->GetBool(SETTINGS_KEY_CHECKFWVERSION), stgSettingsHandle->GetBool(SETTINGS_KEY_CHECKFREESPACE), elErrorLookupHandle.DatabaseVersion());
        dlgSettingsView->show();
    }
}

//=============================================================================
//=============================================================================
#ifdef Q_OS_ANDROID
void
MainWindow::AndroidOpen(
    QString strFilename,
    QByteArray baReturnedFileData
    )
{
    //Android file selection callback
#ifdef ENABLE_DEBUG
    qDebug() << "File name: " << strFilename << ", File data: " << baReturnedFileData;
#endif

    if (!strFilename.isNull())
    {
        //Provided filename is valid
        if (baReturnedFileData.length() > FILESIZE_MIN && baReturnedFileData.length() < FILESIZE_MAX)
        {
            //Valid filesize
            strLocalFilename = strFilename;
            QFontMetrics fmFontMet(ui->label_Filename->font());
            ui->label_Filename->setText(fmFontMet.elidedText(strFilename, Qt::ElideMiddle, ui->label_Filename->maximumWidth()));
            ui->label_Filesize->setText(QString::number(baReturnedFileData.length()));

#ifdef ENABLE_DEBUG
            qDebug() << "Got: " << strFilename;
#endif
            nSelectedFileType = FILE_TYPE_LOCALFILE;
        }

        //Get the shortened filename of the application
        ui->edit_DownloadName->setText(strLocalFilename.left(strLocalFilename.indexOf(".")));

        //
        baFileData.clear();
        baFileData = baReturnedFileData;
    }
    else
    {
        //File selection cancelled - display message
        gstrToastString = "File selection cancelled.";
        ToastMessage(false);
    }

    //Clean up
    disconnect(afdFileDialog, SIGNAL(existingFileNameReady(QString,QByteArray)), this, SLOT(AndroidOpen(QString,QByteArray)));
    delete afdFileDialog;
    afdFileDialog = NULL;
}
#endif

//=============================================================================
//=============================================================================
void
MainWindow::SettingsUpdated(
    QString strVSPUUID,
    QString strblechrTXChar,
    QString strblechrRXChar,
    QString strblechrMOChar,
    QString strblechrMIChar,
    bool bRestrictAdvertising,
#ifdef Q_OS_ANDROID
    bool bCompatibleScanning,
#endif
    quint8 unPacketSize,
    bool bDelFile,
    bool bVerifyFile,
    quint8 unDownloadAction,
    bool bSkipDownloadDisplay,
    quint8 unScrollbackSize,
    bool bXCompile,
    bool bSSL,
    bool bCheckFirmware,
    bool bFreeSpaceCheck
    )
{
    //Callback for settings being updated
    disconnect(dlgSettingsView, SIGNAL(SaveSettings(QString,QString,QString,QString,QString,bool,
#ifdef Q_OS_ANDROID
        bool,
#endif
        quint8,bool,bool,quint8,bool,quint8,bool,bool,bool,bool)), this, SLOT(SettingsUpdated(QString,QString,QString,QString,QString,bool,
#ifdef Q_OS_ANDROID
        bool,
#endif
        quint8,bool,bool,quint8,bool,quint8,bool,bool,bool,bool)));

    if (strVSPUUID != NULL)
    {
        //Update settings
        stgSettingsHandle->SetString(SETTINGS_KEY_UUID, strVSPUUID);
        stgSettingsHandle->SetString(SETTINGS_KEY_TX_OFFSET, strblechrTXChar);
        stgSettingsHandle->SetString(SETTINGS_KEY_RX_OFFSET, strblechrRXChar);
        stgSettingsHandle->SetString(SETTINGS_KEY_MO_OFFSET, strblechrMOChar);
        stgSettingsHandle->SetString(SETTINGS_KEY_MI_OFFSET, strblechrMIChar);
        stgSettingsHandle->SetBool(SETTINGS_KEY_RESTRICTUUID, bRestrictAdvertising);
#ifdef Q_OS_ANDROID
        stgSettingsHandle->SetBool(SETTINGS_KEY_COMPATIBLESCAN, bCompatibleScanning);
#endif
        stgSettingsHandle->SetUInt(SETTINGS_KEY_PACKETSIZE, unPacketSize);
        stgSettingsHandle->SetBool(SETTINGS_KEY_DELFILE, bDelFile);
        stgSettingsHandle->SetBool(SETTINGS_KEY_VERIFYFILE, bVerifyFile);
        stgSettingsHandle->SetUInt(SETTINGS_KEY_DOWNLOADACTION, unDownloadAction);
        stgSettingsHandle->SetBool(SETTINGS_KEY_SKIPDLDISPLAY, bSkipDownloadDisplay);
        stgSettingsHandle->SetUInt(SETTINGS_KEY_SCROLLBACKSIZE, unScrollbackSize);
        stgSettingsHandle->SetBool(SETTINGS_KEY_ONLINEXCOMP, bXCompile);
        stgSettingsHandle->SetBool(SETTINGS_KEY_SSL, bSSL);
        stgSettingsHandle->SetBool(SETTINGS_KEY_CHECKFWVERSION, bCheckFirmware);
        stgSettingsHandle->SetBool(SETTINGS_KEY_CHECKFREESPACE, bFreeSpaceCheck);

        //Change SSL option
        dwnDownloaderHandle->SetSSLSupport(bSSL);
    }

    //Clean up the settings dialog in the main event loop
    dlgSettingsView->deleteLater();
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_SelectFile_clicked(
    )
{
    //File selection button clicked
    dlgFileTypeDialog->open();

#ifdef Q_OS_ANDROID
    //Fix for some faulty and badly designed file selection systems on some android devices i.e. onedrive
    dlgFileTypeDialog->move((QApplication::desktop()->availableGeometry().width()/2 - dlgFileTypeDialog->width()/2), (QApplication::desktop()->availableGeometry().height()/2 - dlgFileTypeDialog->height()/2));

    //Fix for some bug in Qt which decies to cut the top and bottom of word-wrapped text off
    tmrStartupTimer = new QTimer();
    connect(tmrStartupTimer, SIGNAL(timeout()), this, SLOT(FileTypeSelectionFixBrokenQtTextHeightTimerElapsed()));
    tmrStartupTimer->setInterval(5);
    tmrStartupTimer->setSingleShot(true);
    tmrStartupTimer->start();
#endif
}

//=============================================================================
//=============================================================================
void
MainWindow::ToastMessage(
    bool bToastLong
    )
{
    //Shows a status message
#ifdef Q_OS_ANDROID
    //Android
    QtAndroid::runOnAndroidThread([bToastLong]
    {
        QAndroidJniObject JavaMsg = QAndroidJniObject::fromString(gstrToastString);
        QAndroidJniObject Toast = QAndroidJniObject::callStaticObjectMethod("android/widget/Toast", "makeText", "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;", QtAndroid::androidContext().object(), JavaMsg.object(), (bToastLong == true ? ANDROID_TOAST_DURATION_LONG : ANDROID_TOAST_DURATION_SHORT));
        Toast.callMethod<void>("show");
    });
#else
    //Other OS
    //TODO
#endif
}

//=============================================================================
//=============================================================================
void
MainWindow::LoadImages(
    )
{
    //Load the busy animation
    movieLoadingAnimation = new QMovie(":/Internal/loader.gif");
    movieLoadingAnimation->setScaledSize(QSize(LOADING_IMAGE_WIDTH, LOADING_IMAGE_HEIGHT));

    //Load the stanby picture
    pixmapStandbyPicture = new QPixmap(":/Internal/standby.png");
}

//=============================================================================
//=============================================================================
void
MainWindow::SetLoadingStatus(
    quint8 unStatus
    )
{
    //Changes loading status to show a loading image or an idle image
    if (unOldStatus == unStatus)
    {
        //Nothing to change
        return;
    }

    if (unStatus == STATUS_STANDBY)
    {
        //Standby
        labelStatusBarLoader->setPixmap(*pixmapStandbyPicture);
        movieLoadingAnimation->stop();
    }
    else if (unStatus == STATUS_LOADING)
    {
        //Loading
        movieLoadingAnimation->start();
        labelStatusBarLoader->setMovie(movieLoadingAnimation);
    }
    unOldStatus = unStatus;
}

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_ModuleInfo_clicked(
    )
{
    //Fetch information about module
    if (nCurrentMode == MAIN_MODE_IDLE)
    {
        nCurrentMode = MAIN_MODE_QUERY;
        baVersionResponse.clear();
        balOutputBufferList.append("at i 0\r\n");
        balOutputBufferList.append("at i 3\r\n");
        balOutputBufferList.append("at i 6\r\n");
        baOutputBuffer = balOutputBufferList.takeFirst();
        blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));

        //Start response timeout timer
        tmrResponseTimeoutTimer->start();

        //Enable cancel button
        ui->btn_Cancel->setEnabled(true);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::ExternalToastMessage(
    QString strMessage,
    bool bToastLong
    )
{
    //Callback for other classes to display a toast message
    gstrToastString = strMessage;
    ToastMessage(bToastLong);
}

//=============================================================================
//=============================================================================
void
MainWindow::FileTypeChanged(
    qint8 nFileType,
    QString strData
    )
{
    //Type of file changed - show file selection dialog
    if (nFileType == FILE_TYPE_LOCALFILE)
    {
#ifdef Q_OS_ANDROID
        //Android
        afdFileDialog = new AndroidFileDialog();
        connect(afdFileDialog, SIGNAL(existingFileNameReady(QString,QByteArray)), this, SLOT(AndroidOpen(QString,QByteArray)));
        if (!afdFileDialog->provideExistingFileName())
        {
            //File open dialogue failed
            connect(afdFileDialog, SIGNAL(existingFileNameReady(QString,QByteArray)), this, SLOT(AndroidOpen(QString,QByteArray)));
            delete afdFileDialog;
            QMessageBox::critical(this, "Error opening file selector", "An error occured whilst attempting to open the Android File Selector dialogue, please report this issue including details of which device you are using it on and what the firmware version is.", QMessageBox::Ok, QMessageBox::NoButton);
        }
#else
        //
        QString Filename = QFileDialog::getOpenFileName(this, "Select application", stgSettingsHandle->GetString(SETTINGS_KEY_LASTDIR), "smartBASIC Source/Application (*.sb, *.uwc);;All Files (*.*)", NULL);
        if (!Filename.isEmpty() && !Filename.isNull())
        {
            //File selected
            stgSettingsHandle->SetString(SETTINGS_KEY_LASTDIR, Filename.left(Filename.lastIndexOf("/")));
        }
#endif
    }
    else if (nFileType == FILE_TYPE_REMOTEURL)
    {
        //Online file selected
        nSelectedFileType = FILE_TYPE_REMOTEURL;
        strLocalFilename = strData;

        //Set labels
        QString strURL = strData.right(strData.length() - strData.lastIndexOf("/") - 1);
        strURL = QUrl::fromPercentEncoding(strURL.toUtf8());
        ui->label_Filename->setText(strURL);
        ui->edit_DownloadName->setText(strURL.left(strURL.indexOf(".")));
        ui->label_Filesize->setText("N/A");

        //Shortern text
        QFontMetrics fmFontMet(ui->label_Filename->font());
        ui->label_Filename->setText(fmFontMet.elidedText(strURL, Qt::ElideMiddle, ui->label_Filename->maximumWidth()));
    }
#ifndef Q_OS_ANDROID
    else if (nFileType == FILE_TYPE_DROPBOX)
    {
        //File from dropbox selected
        //TODO
    }
#endif
}

//=============================================================================
//=============================================================================
void
MainWindow::FileDownloaded(
    bool bSuccess,
    qint16 nErrorCode,
    QByteArray baDownloadedFileData
    )
{
    //Callback for file download notification
    if (lecBLEController != NULL && bDisconnectActive == false)
    {
        if (bSuccess == true)
        {
            //Successfully downloaded file
            ui->label_Filesize->setText(QString::number(baDownloadedFileData.length()));

            //Is this a source file or compiled application?
            if (stgSettingsHandle->GetBool(SETTINGS_KEY_ONLINEXCOMP) == true && (strLocalFilename.right(3).toLower() == ".sb" || strLocalFilename.right(4).toLower() == ".txt"))
            {
                //Source file and XCompilation enabled, load file data into byte array
                baFileData.clear();
                baFileData = baDownloadedFileData;

                //Fetch details from module
                nCurrentMode = MAIN_MODE_VERSION;
                baVersionResponse.clear();
                balOutputBufferList.clear();
                balOutputBufferList.append("at i 0\r\n");
                balOutputBufferList.append("at i 3\r\n");
                balOutputBufferList.append("at i 13\r\n");
                if (stgSettingsHandle->GetBool(SETTINGS_KEY_CHECKFREESPACE) == true)
                {
                    //Check if there is sufficient free space
                    balOutputBufferList.append("at i 6\r\n");
                }
                baOutputBuffer = balOutputBufferList.takeFirst();
                blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));

                //Enable cancel button and disable download filename edit
                ui->btn_Cancel->setEnabled(true);
                ui->edit_DownloadName->setReadOnly(true);
            }
            else
            {
                //Application file or XCompilation disabled, load directly to module
                if (baDownloadedFileData.length() > nModuleFreeSpace)
                {
                    //Insufficient space available on the module to download this application, check if the user is sure they want to download it
                    if (QMessageBox::question(this, "Insufficient module space", QString("There is insufficient storage space available on the module, ").append(QString::number(baDownloadedFileData.length())).append(" bytes are required but only ").append(QString::number(nModuleFreeSpace)).append(" bytes are free therefore the OTA will likely fail.\r\n\r\nAre you sure you want to continue with this operation?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
                    {
                        //User does not want to continue, cancel operation
                        gstrToastString = "Insufficient module storage space, OTA cancelled!";

                        //Show message
                        ToastMessage(false);

                        //Set status back to being idle
                        baOutputBuffer.clear();
                        balOutputBufferList.clear();
                        nCurrentMode = MAIN_MODE_IDLE;
                        SetLoadingStatus(STATUS_STANDBY);

                        if (tmrResponseTimeoutTimer->isActive())
                        {
                            //Stop timeout timer
                            tmrResponseTimeoutTimer->stop();
                        }

                        //Disable cancel button and enable download filename edit
                        ui->btn_Cancel->setEnabled(false);
                        ui->edit_DownloadName->setReadOnly(false);
                        return;
                    }
                }

                if (stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE) == true)
                {
                    //Checksum is being calculated, create object
                    chkChecksum = new ChecksumCalculator();
                }

                //One byte at a time, convert the data to hex
                QString gstrHexData = "";
                int i = 0;

                while (i < baDownloadedFileData.length())
                {
                    quint8 ThisByte = baDownloadedFileData[i];
                    if (stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE) == true)
                    {
                        //Add to checksum
                        chkChecksum->AddByte(ThisByte);
                    }

                    QString strThisHex;
                    strThisHex.setNum(ThisByte, 16);
                    if (strThisHex.length() == 1)
                    {
                        //Expand to 2 characters
                        strThisHex.prepend("0");
                    }

                    //Add the hex character to the string
//                    qDebug() << "Add: " << strThisHex;
                    gstrHexData.append(strThisHex.toUpper());
                    ++i;
                }

                //Clean up
                if (stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE) == true)
                {
                    strChecksumString = new QString(chkChecksum->GetChecksumHexString());
                    delete chkChecksum;
                    chkChecksum = NULL;
                }

                //Clear output buffer
                baOutputBuffer.clear();

                if (stgSettingsHandle->GetBool(SETTINGS_KEY_DELFILE) == true)
                {
                    //Add delete file command
                    baOutputBuffer.append(QString("AT+del \"").append(ui->edit_DownloadName->text()).append("\"\r"));
                }

                //Create the file writing data
                baOutputBuffer.append(QString("AT+fow \"").append(ui->edit_DownloadName->text()).append("\"\r"));
                while (gstrHexData.length() > 0)
                {
                    baOutputBuffer.append(QString("AT+fwrh \"").append(gstrHexData.length() > 56 ? gstrHexData.left(56) : gstrHexData).append("\"\r"));
                    if (gstrHexData.length() > 56)
                    {
                        gstrHexData = gstrHexData.right(gstrHexData.length() - 56);
                    }
                    else
                    {
                        gstrHexData = "";
                    }
                }

                //Closing the file
                baOutputBuffer.append(QString("at+fcl\r"));

                if (stgSettingsHandle->GetBool(SETTINGS_KEY_VERIFYFILE) == true)
                {
                    //Add commands for verifying file exists and was transferred successfully
                    baOutputBuffer.append(QString("at i 0xc12c\rat+dir\r"));
                }
                else
                {
                    //Send dummy command
                    baOutputBuffer.append(QString("at i 1\r"));
                }

                //Set variables for progress tracking and clear any received responses - note that the total app size is the size of the encoded buffer data, not just purely the application size
                unTotalSizeSent = 0;
                unTotalAppSize = baOutputBuffer.length();
                baVersionResponse.clear();

                //Set mode to downloading application
                nCurrentMode = MAIN_MODE_DOWNLOADING;

#ifdef ENABLE_DEBUG
qDebug() << "nCurrentMode is now: " << nCurrentMode << ", Data: " << baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE));
#endif

                //Start writing the data
                blesvcVSPService->writeCharacteristic(blechrRXChar, baOutputBuffer.left(stgSettingsHandle->GetUInt(SETTINGS_KEY_PACKETSIZE)));

                //Enable cancel button and disable download filename edit
                ui->btn_Cancel->setEnabled(true);
                ui->edit_DownloadName->setReadOnly(true);
            }
        }
        else
        {
            //Failed to download file
            if (nErrorCode == DOWNLOAD_FILESIZE_ERROR)
            {
                //Invalid filesize (too big or small)
                gstrToastString = "Error: filesize is either too big or small to be used.";
            }
            else if (nErrorCode == DOWNLOAD_SSL_CERT_ERROR)
            {
                //SSL certificate error
                gstrToastString = "Error: specified URL's SSL certificate is not valid.";
            }
#ifdef QT_NO_SSL
            else if (nErrorCode == DOWNLOAD_SSL_SUPPORT_ERROR)
            {
                //SSL not supported error
                gstrToastString = "Error: specified URL requires SSL but SSL support is not available.";
            }
#endif
            else if (nErrorCode == DOWNLOAD_GENERAL_ERROR)
            {
                //General error
                gstrToastString = QString("HTTP Download error: ").append(baDownloadedFileData);
            }
            else
            {
                //HTTP error code
                gstrToastString = QString("Failed to download file, HTTP response code: ").append(QString::number(nErrorCode));
            }

            //Show message
            ToastMessage(false);

            //Set status back to being idle
            baOutputBuffer.clear();
            balOutputBufferList.clear();
            nCurrentMode = MAIN_MODE_IDLE;
            SetLoadingStatus(STATUS_STANDBY);

            if (tmrResponseTimeoutTimer->isActive())
            {
                //Stop timeout timer
                tmrResponseTimeoutTimer->stop();
            }

            //Disable cancel button and enable download filename edit
            ui->btn_Cancel->setEnabled(false);
            ui->edit_DownloadName->setReadOnly(false);
        }
    }
    else
    {
        //Response received but device has disconnected
//        gstrToastString = "";
//        ToastMessage(false);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::DownloaderStatusChanged(
    quint8 unStatus
    )
{
    //Download object status has changed
    if (unStatus == DOWNLOAD_MODE_IDLE)
    {
        //Inactive
    }
    else if (unStatus == DOWNLOAD_MODE_DEV_SUPPORTED)
    {
        //Checking if module and firmware is supported
        gstrToastString = "Checking for online XCompiler support...";
        ToastMessage(false);
    }
    else if (unStatus == DOWNLOAD_MODE_XCOMPILE)
    {
        //XCompiling application
        gstrToastString = "XCompiling application...";
        ToastMessage(false);
    }
    else if (unStatus == DOWNLOAD_MODE_DOWNLOAD_FILE)
    {
        //Downloading remote file
        gstrToastString = "Downloading remote file...";
        ToastMessage(false);
    }
}

//=============================================================================
//=============================================================================
void
MainWindow::StartupTimerElapsed(
    )
{
    //
    ui->label_Filename->setMaximumWidth(ui->label_Filename->width() - 40);

    //
    disconnect(tmrStartupTimer, SIGNAL(timeout()), this, SLOT(StartupTimerElapsed()));
    delete tmrStartupTimer;
    tmrStartupTimer = NULL;
}

//=============================================================================
//=============================================================================
void
MainWindow::TimeoutTimerElapsed(
    )
{
    //Timeout whilst awaiting response from module
    if (nCurrentMode == MAIN_MODE_VERSION)
    {
        //Timeout whilst attempting to retrieve module firmware details
        gstrToastString = "Response timeout awaiting module firmware details - please ensure module is in hardware command mode VSP and retry.";
    }
    else if (nCurrentMode == MAIN_MODE_SPACECHECK)
    {
        //Timeout whilst attempting to retrieve module storage space
        gstrToastString = "Response timeout awaiting module storage space - please ensure module is in hardware command mode VSP and retry.";
    }
    else if (nCurrentMode == MAIN_MODE_DOWNLOADING)
    {
        //Timeout whilst downloading application
        gstrToastString = "Response timeout whilst downloading application to module - please try again.";
    }
    else if (nCurrentMode == MAIN_MODE_VERIFYING)
    {
        //Timeout whilst attempting to verify application
        gstrToastString = "Response timeout whilst attempting to verify application - please try again.";
    }
    else if (nCurrentMode == MAIN_MODE_QUERY)
    {
        //Timeout whilst awaiting module response to query
        gstrToastString = "Response timeout awaiting module query details - please ensure module is in hardware command mode VSP and retry.";
    }
    else
    {
        //Unhandled timeout
        return;
    }

    //Show message
    ToastMessage(true);

    //Clear variables and set mode back to being idle
    nCurrentMode = MAIN_MODE_IDLE;
    balOutputBufferList.clear();
    baFileData.clear();
    baOutputBuffer.clear();
    baVersionResponse.clear();
    SetLoadingStatus(STATUS_STANDBY);

    if (tmrResponseTimeoutTimer->isActive())
    {
        //Stop timeout timer
        tmrResponseTimeoutTimer->stop();
    }

    //Disable cancel button and enable download filename edit
    ui->btn_Cancel->setEnabled(false);
    ui->edit_DownloadName->setReadOnly(false);
}

//=============================================================================
//=============================================================================
#ifdef Q_OS_ANDROID
void
MainWindow::FileTypeSelectionFixBrokenQtTextHeightTimerElapsed(
    )
{
    //Fix for some bug in Qt which decies to cut the top and bottom of word-wrapped text off
    disconnect(this, SLOT(FileTypeSelectionFixBrokenQtTextHeightTimerElapsed()));
    dlgFileTypeDialog->FixBrokenQtTextHeight();
    delete tmrStartupTimer;
    tmrStartupTimer = NULL;
}
#endif

//=============================================================================
//=============================================================================
void
MainWindow::on_btn_Cancel_clicked(
    )
{
    //Cancel current action (if there is an active activity)
    if (nCurrentMode != MAIN_MODE_IDLE)
    {
        if (nCurrentMode == MAIN_MODE_ONLINE_DOWNLOAD || nCurrentMode == MAIN_MODE_XCOMPILING)
        {
            //Pending web request, cancel it
            dwnDownloaderHandle->CancelRequest();
        }
        else if (nCurrentMode == MAIN_MODE_QUERY || nCurrentMode == MAIN_MODE_VERIFYING || nCurrentMode == MAIN_MODE_DOWNLOADING || nCurrentMode == MAIN_MODE_VERSION || nCurrentMode == MAIN_MODE_SPACECHECK)
        {
            //Pending sending data to/from module
            baOutputBuffer.clear();
            balOutputBufferList.clear();
            baFileData.clear();
            baVersionResponse.clear();
            nCurrentMode = MAIN_MODE_IDLE;
            SetLoadingStatus(STATUS_STANDBY);

            if (tmrResponseTimeoutTimer->isActive())
            {
                //Stop timeout timer
                tmrResponseTimeoutTimer->stop();
            }

            //Show message
            gstrToastString = "Operation cancelled.";
            ToastMessage(false);

            //Disable cancel button and enable download filename edit
            ui->btn_Cancel->setEnabled(false);
            ui->edit_DownloadName->setReadOnly(false);
        }
    }

    //Disable button
    ui->btn_Cancel->setEnabled(false);
}

//=============================================================================
//=============================================================================
void
MainWindow::FirmwareVersionCheck(
    bool bSuccess,
    qint16 nErrorCode,
    QString strData
    )
{
    //Response to latest firmware check
    QRegularExpressionMatch rexpmM2Match = rxpDevName.match(baVersionResponse);
    QRegularExpressionMatch rexpmM3Match = rxpFirmware.match(baVersionResponse);
    QRegularExpressionMatch rexpmM4Match = rxpFreeSpace.match(baVersionResponse);
    nCurrentMode = MAIN_MODE_IDLE;
    QString strExtraInfo = "";

    if (bSuccess == false)
    {
        //Error with request
        gstrToastString = QString("Firmware version check failed: ").append((nErrorCode == DOWNLOAD_SSL_CERT_ERROR ? "SSL certificate invalid" : (nErrorCode == DOWNLOAD_JSON_ERROR ? "JSON decoding failed" : (nErrorCode == DOWNLOAD_GENERAL_ERROR ? strData : QString("Status code ").append(QString::number(nErrorCode)).append(", ").append(strData)))));
        ToastMessage(false);
    }
    else
    {
        //Version response received
        if (nErrorCode == FIRMWARE_CHECK_OLD)
        {
            //Outdated
            strExtraInfo = QString(", which is outdated, the latest firmware is: ").append(strData);
        }
        else if (nErrorCode == FIRMWARE_CHECK_CURRENT)
        {
            //Latest
            strExtraInfo = ", which is up-to-date";
        }
        else if (nErrorCode == FIRMWARE_CHECK_TEST)
        {
            //Test
            strExtraInfo = ", which is an engineering/test firmware";
        }
        else if (nErrorCode == FIRMWARE_CHECK_UNSUPPORTED)
        {
            //Unsupported
            gstrToastString = "Firmware/device unsupported, latest firmware not known.";
            ToastMessage(false);
        }
    }

    //Show information
    QMessageBox::information(this, "Module Information", QString("The connected device is a ").append(rexpmM2Match.captured(1)).append(" module on firmware version ").append(rexpmM3Match.captured(1)).append(strExtraInfo).append(".\r\nFlash space available: ").append(rexpmM4Match.captured(2)).append("/").append(rexpmM4Match.captured(1)).append(" bytes (").append(QString::number(rexpmM4Match.captured(2).toUInt()*100/rexpmM4Match.captured(1).toUInt())).append("%).").append((rexpmM2Match.captured(1) == "BL652" && rexpmM3Match.captured(1) == "28.7.3.0" && bIs2MPhySupported == true ? "\r\n\r\nPlease note: VSP/OTA to this device is likely to fail due to your device having a Bluetooth v5 radio with support for 2M PHY." : "")));

    //Clean up
    balOutputBufferList.clear();
    baOutputBuffer.clear();
    baVersionResponse.clear();
    UpdateDisplay();
}

//=============================================================================
//=============================================================================
void
MainWindow::TrucateRecBuffer(
    )
{
    //Trucates the receive buffer to a smaller size if desired
    if (stgSettingsHandle->GetInt(SETTINGS_KEY_SCROLLBACKSIZE) > 0)
    {
        //Trucation is enabled
        if (baRecBuffer.count("\n") > stgSettingsHandle->GetInt(SETTINGS_KEY_SCROLLBACKSIZE))
        {
            //Remove lines
            qint8 nLineCount = baRecBuffer.count("\n") - stgSettingsHandle->GetInt(SETTINGS_KEY_SCROLLBACKSIZE);
            quint16 unCutoffPosition = 0;
            while (nLineCount > 0)
            {
                unCutoffPosition = baRecBuffer.indexOf("\n", unCutoffPosition+1);
                --nLineCount;
            }

            //Perform the truncation
            baRecBuffer = baRecBuffer.right(baRecBuffer.length()-unCutoffPosition);
        }
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
