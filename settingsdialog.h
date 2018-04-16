/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: settingsdialog.h
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
#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QDialog>
#include <QDesktopWidget>
#include <QMessageBox>
#include "target.h"

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class SettingsDialog;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit
    SettingsDialog(
        QWidget *parent = 0
        );
    ~SettingsDialog(
        );
    void
    SetValues(
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
        );
#ifdef Q_OS_ANDROID
    void
    UpdateWindowSize(
        );
#endif

signals:
    void
    SaveSettings(
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
        quint8 unDownloadActions,
        bool bSkipDownloadDisplay,
        quint8 unScrollbackSize,
        bool bXCompile,
        bool bSSL,
        bool bCheckFirmware,
        bool bFreeSpaceCheck
        );

private slots:
    void
    on_btn_Save_clicked(
        );
    void
    on_btn_Discard_clicked(
        );
    void
    on_btn_Defaults_clicked(
        );
    void
    on_btn_About_clicked(
        );

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
