/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: mainwindow.h
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QMainWindow>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QBluetoothLocalDevice>
#include <QLowEnergyController>
#include <QMovie>
#include <QElapsedTimer>
#include <QTimer>
#include <QFile>
#include <QDateTime>
#include <QRegularExpression>
#include <QScrollBar>
#include <QFileDialog>
#include <math.h>
#include "downloader.h"
#include "scanselection.h"
#include "settingsdialog.h"
#include "settingsstorage.h"
#include "errorlookup.h"
#include "filetypeselection.h"
#include "checksumcalculator.h"
#include "target.h"

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QAndroidJniEnvironment>
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroid>
#include <QAndroidActivityResultReceiver>
#include "androidfiledialog.h"
#endif

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class MainWindow;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit
    MainWindow(
        QWidget *parent = 0
        );
    ~MainWindow(
        );

private slots:
    void
    FoundDevice(
        QBluetoothDeviceInfo bdiDeviceInfo
        );
    void
    BTError(
        QBluetoothDeviceDiscoveryAgent::Error nErrorCode
        );
    void
    BTFinishedScan(
        );
    void
    BLEConnected(
        );
    void
    BLEDisconnected(
        );
    void
    BLEDiscoveryFinished(
        );
    void
    BLEDiscovered(
        QBluetoothUuid
        );
    void
    BLEError(
        QLowEnergyController::Error nErrorCode
        );
    void
    BLEStateChanged(
        QLowEnergyController::ControllerState lesNewState
        );
    void
    VSPServiceCharacteristicChanged(
        QLowEnergyCharacteristic lecCharacteristic,
        QByteArray baData
        );
    void
    VSPServiceCharacteristicWritten(
        QLowEnergyCharacteristic lecCharacteristic,
        QByteArray baData
        );
    void
    VSPServiceDescriptorWritten(
        QLowEnergyDescriptor ledDescriptor,
        QByteArray baData
        );
    void
    VSPServiceError(
        QLowEnergyService::ServiceError nErrorCode
        );
    void
    VSPServiceStateChanged(
        QLowEnergyService::ServiceState nNewState
        );
    void
    ConnectToDevice(
        QBluetoothDeviceInfo bdiDeviceInfo
        );
    void
    on_btn_Disconnect_clicked(
        );
    void
    on_btn_Scan_clicked(
        );
    void
    ClearVar(
        );
    void
    on_btn_Clear_clicked(
        );
    void
    UpdateDisplay(
        );
    void
    ProcessFileData(
        bool bSuccess,
        qint16 nErrorCode,
        QByteArray FileData
        );
    void
    on_btn_Download_clicked(
        );
    void
    CancelScan(
        );
    void
    on_btn_Settings_clicked(
        );
    void
    SettingsUpdated(
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
        );
#ifdef Q_OS_ANDROID
    void
    AndroidOpen(
        QString strFilename,
        QByteArray baReturnedFileData
        );
#endif
    void
    ToastMessage(
        bool bToastLong
        );
    void
    on_btn_SelectFile_clicked(
        );
    void
    LoadImages(
        );
    void
    SetLoadingStatus(
        quint8 unStatus
        );
    void
    on_btn_ModuleInfo_clicked(
        );
    void
    ExternalToastMessage(
        QString strMessage,
        bool bToastLong
        );
    void
    FileTypeChanged(
        qint8 nFileType,
        QString strData
        );
    void
    FileDownloaded(
        bool bSuccess,
        qint16 nErrorCode,
        QByteArray baDownloadedFileData
        );
    void
    DownloaderStatusChanged(
        quint8 unStatus
        );
    void
    StartupTimerElapsed(
        );
    void
    TimeoutTimerElapsed(
        );
#ifdef Q_OS_ANDROID
    void
    FileTypeSelectionFixBrokenQtTextHeightTimerElapsed(
        );
#endif
    void
    on_btn_Cancel_clicked(
        );
    void
    FirmwareVersionCheck(
        bool bSuccess,
        qint16 nErrorCode,
        QString strData
        );
    void
    TrucateRecBuffer(
        );

private:
    void
    UpdateTxRx(
        );

    Ui::MainWindow *ui;

    //Bools
    bool bVSPBlocked; //True if VSP host has notified saying to stop sending data
    bool bHasModem;
    bool bIs2MPhySupported;
    bool bDisconnectActive;
    bool bIsConnected;

    //Integers
    qint8 nSelectedFileType;
    qint8 nCurrentMode;
    qint8 nStatusBarSpaces;
    quint8 unOldStatus;
    qint32 nModuleFreeSpace;
    quint32 unRecDatSize;     //Amount of data received
    quint32 unWrittenBytes;   //Number of bytes written
    quint32 unTotalAppSize;   //Total size of application to load
    quint32 unTotalSizeSent;  //Total size of application which has been sent

    //Strings
    QString strLocalFilename;
    QString *strChecksumString;

    //Byte arrays
    QByteArray baRecBuffer;
    QByteArray baVersionResponse;
    QByteArray baFileData;
    QByteArray baOutputBuffer;

    //Byte array lists
    QByteArrayList balOutputBufferList;

    //Timers
    QTimer *tmrDisconnectCleanUpTimer;
    QTimer *tmrDisplayUpdateTimer;
    QTimer *tmrStartupTimer;
    QTimer *tmrResponseTimeoutTimer;

    //Regular expressions
    QRegularExpression rxpDevName;
    QRegularExpression rxpXCompiler;
    QRegularExpression rxpFreeSpace;
    QRegularExpression rxpFirmware;
    QRegularExpression rxpErrorCode;
    QRegularExpression rxpFileListing;
    QRegularExpression rxpCRC;

    //Objects
    Downloader *dwnDownloaderHandle;
    SettingsStorage *stgSettingsHandle;
    ChecksumCalculator *chkChecksum;
    ErrorLookup elErrorLookupHandle;

    //Dialogs
    ScanSelection *dlgScanDialog;
    SettingsDialog *dlgSettingsView;
    FileTypeSelection *dlgFileTypeDialog;
#ifdef Q_OS_ANDROID
    AndroidFileDialog *afdFileDialog;
#endif

    //Bluetooth-related
    QLowEnergyService *blesvcVSPService; //VSP service
    QLowEnergyCharacteristic blechrRXChar; //RX characteristic
    QLowEnergyCharacteristic blechrTXChar; //TX characteristic
    QLowEnergyCharacteristic blechrMIChar; //Modem in characteristic
    QLowEnergyCharacteristic blechrMOChar; //Modem out characteristic
    QBluetoothDeviceDiscoveryAgent *ddaDiscoveryAgent; //
    QLowEnergyController *lecBLEController; //

    //Status bar objects
    QMovie *movieLoadingAnimation;
    QPixmap *pixmapStandbyPicture;
    QLabel *labelStatusBarLoader;
};

#endif // MAINWINDOW_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
