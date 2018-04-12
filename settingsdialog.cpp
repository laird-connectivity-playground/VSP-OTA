/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: settingsdialog.cpp
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
#include "settingsdialog.h"
#include "ui_settingsdialog.h"

//=============================================================================
//=============================================================================
SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog)
{
    //Constructor
    ui->setupUi(this);

#ifdef Q_OS_ANDROID
    //Use full screen size on Android
    this->setFixedSize(QApplication::desktop()->availableGeometry().width(), QApplication::desktop()->availableGeometry().height());
#else
    //Remove compatible scanning option for non-android systems
    ui->check_CompatibleScan->deleteLater();
#endif
}

//=============================================================================
//=============================================================================
SettingsDialog::~SettingsDialog(
    )
{
    //Destructor
    delete ui;
}

//=============================================================================
//=============================================================================
void
SettingsDialog::SetValues(
    QString strVSPUUID,
    QString strTxChar,
    QString strRxChar,
    QString strMoChar,
    QString strMiChar,
    bool bRestrictAdvertising,
#ifdef Q_OS_ANDROID
    bool bCompatibleScanning,
#endif
    quint8 unPacketSize,
    bool bDelFile,
    bool bVerifyChecksum,
    quint8 unDownloadAction,
    bool bSkipDownloadDisplay,
    quint8 unScrollbackSize,
    bool bXCompile,
    bool bSSL,
    bool bCheckFirmware,
    bool bFreeSpaceCheck,
    QString strDatabaseVersion
    )
{
    //Load settings
    ui->edit_VSPUUID->setText(strVSPUUID);
    ui->edit_TxOffset->setText(strTxChar);
    ui->edit_RxOffset->setText(strRxChar);
    ui->edit_MoOffset->setText(strMoChar);
    ui->edit_MiOffset->setText(strMiChar);
    ui->check_RestrictAdvertising->setChecked(bRestrictAdvertising);
#ifdef Q_OS_ANDROID
    ui->check_CompatibleScan->setChecked(bCompatibleScanning);
#endif
    ui->edit_PacketSize->setValue(unPacketSize);
    ui->check_DelFile->setChecked(bDelFile);
    ui->check_VerifyChecksum->setChecked(bVerifyChecksum);
    if (unDownloadAction == DOWNLOAD_ACTION_NOTHING)
    {
        //No action after successful download
        ui->rdo_DownloadNothing->setChecked(true);
    }
    else if (unDownloadAction == DOWNLOAD_ACTION_DISCONNECT)
    {
        //Disconnect from moduled after successful download
        ui->rdo_DownloadDisconnect->setChecked(true);
    }
    else if (unDownloadAction == DOWNLOAD_ACTION_RESTART)
    {
        //Restart module after successful download
        ui->rdo_DownloadRestart->setChecked(true);
    }
    ui->check_SkipDownloadDisplay->setChecked(bSkipDownloadDisplay);
    ui->edit_Scrollback->setValue(unScrollbackSize);
    ui->check_XCompile->setChecked(bXCompile);
    ui->check_SSL->setChecked(bSSL);
    ui->check_FirmwareCheck->setChecked(bCheckFirmware);
    ui->check_FreeSpaceCheck->setChecked(bFreeSpaceCheck);
    ui->label_ErrorCodeVersion->setText(ui->label_ErrorCodeVersion->text().append(strDatabaseVersion));

#ifndef UseSSL
    ui->check_SSL->setEnabled(false);
#else
    ui->check_SSL->setEnabled(ui->check_XCompile->isChecked());
#endif
}

//=============================================================================
//=============================================================================
void
SettingsDialog::on_btn_Save_clicked(
    )
{
    //Save button clicked
    emit SaveSettings(ui->edit_VSPUUID->text(), ui->edit_TxOffset->text(), ui->edit_RxOffset->text(), ui->edit_MoOffset->text(), ui->edit_MiOffset->text(), ui->check_RestrictAdvertising->isChecked(),
#ifdef Q_OS_ANDROID
        ui->check_CompatibleScan->isChecked(),
#endif
        ui->edit_PacketSize->value(), ui->check_DelFile->isChecked(), ui->check_VerifyChecksum->isChecked(), (ui->rdo_DownloadNothing->isChecked() ? DOWNLOAD_ACTION_NOTHING : (ui->rdo_DownloadDisconnect->isChecked() ? DOWNLOAD_ACTION_DISCONNECT : (ui->rdo_DownloadRestart->isChecked() ? DOWNLOAD_ACTION_RESTART : DOWNLOAD_ACTION_NOTHING))), ui->check_SkipDownloadDisplay->isChecked(), ui->edit_Scrollback->value(), ui->check_XCompile->isChecked(), ui->check_SSL->isChecked(), ui->check_FirmwareCheck->isChecked(), ui->check_FreeSpaceCheck->isChecked());
    this->close();
}

//=============================================================================
//=============================================================================
void
SettingsDialog::on_btn_Discard_clicked(
    )
{
    //Discard settings
    emit SaveSettings(NULL, NULL, NULL, NULL, NULL, NULL,
#ifdef Q_OS_ANDROID
        NULL,
#endif
        0, NULL, NULL, 0, NULL, 0, NULL, NULL, NULL, NULL);
    this->close();
}

//=============================================================================
//=============================================================================
void
SettingsDialog::on_btn_Defaults_clicked(
    )
{
    //Restore settings to default
    if (QMessageBox::question(this, "Restore default settings?", "Are you sure you wish to restore the default settings?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        //User has confirmed restoring settings to default
        ui->edit_VSPUUID->setText(SETTINGS_VALUE_UUID);
        ui->edit_TxOffset->setText(SETTINGS_VALUE_TX_OFFSET);
        ui->edit_RxOffset->setText(SETTINGS_VALUE_RX_OFFSET);
        ui->edit_MoOffset->setText(SETTINGS_VALUE_MO_OFFSET);
        ui->edit_MiOffset->setText(SETTINGS_VALUE_MI_OFFSET);
        ui->check_RestrictAdvertising->setChecked(SETTINGS_VALUE_RESTRICTUUID);
#ifdef Q_OS_ANDROID
        ui->check_CompatibleScan->setChecked(SETTINGS_VALUE_COMPATIBLESCAN);
#endif
        ui->edit_PacketSize->setValue(SETTINGS_VALUE_PACKETSIZE);
        ui->check_DelFile->setChecked(SETTINGS_VALUE_DELFILE);
        ui->check_VerifyChecksum->setChecked(SETTINGS_VALUE_VERIFYFILE);
#if (SETTINGS_VALUE_DOWNLOADACTION == DOWNLOAD_ACTION_NOTHING)
        ui->rdo_DownloadNothing->setChecked(true);
#elif (SETTINGS_VALUE_DOWNLOADACTION == DOWNLOAD_ACTION_DISCONNECT)
        ui->rdo_DownloadDisconnect->setChecked(true);
#elif (SETTINGS_VALUE_DOWNLOADACTION == DOWNLOAD_ACTION_RESTART)
        ui->rdo_DownloadRestart->setChecked(true);
#endif
        ui->check_SkipDownloadDisplay->setChecked(SETTINGS_VALUE_SKIPDLDISPLAY);
        ui->edit_Scrollback->setValue(SETTINGS_VALUE_SCROLLBACKSIZE);
        ui->check_XCompile->setChecked(SETTINGS_VALUE_ONLINEXCOMP);
        ui->check_SSL->setChecked(SETTINGS_VALUE_SSL);
        ui->check_FirmwareCheck->setChecked(SETTINGS_VALUE_CHECKFWVERSION);
        ui->check_FreeSpaceCheck->setChecked(SETTINGS_VALUE_CHECKFREESPACE);
    }
}

//=============================================================================
//=============================================================================
void
SettingsDialog::on_btn_About_clicked(
    )
{
    //Show information on application
    QMessageBox::information(this, "About Laird OTA VSP", QString("Laird OTA VSP v").append(APP_VERSION).append(" is a Qt-based GPLv3 licensed (not including later versions) open-source application for customers to use and modify for online XCompilation and downloading of smartBASIC files to Laird's wireless Bluetooth modules. The source code for this project can be freely viewed and downloaded from Github: https://github.com/LairdCP/Laird_OTA_VSP\r\n\r\nPlease note that online requests to the XCompilation server will store the HTTP headers from your request (IP address, time/date of access, header size, browser and operating system) on the server, temporary storage is used for storing uploaded and XCompiled application data which is purged from the server after the data has been sent back to your client device. Submitted applications are not stored or analysed beyond what is required for the XCompilation process.\r\n\r\nFor license information on software libraries that are integrated into this application, please check the help documentation from the startup screen of the application."), QMessageBox::Ok, QMessageBox::NoButton);
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
