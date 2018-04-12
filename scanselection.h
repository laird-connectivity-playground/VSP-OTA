/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: scanselection.h
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
#ifndef SCANSELECTION_H
#define SCANSELECTION_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QDialog>
#include <QListWidgetItem>
#include <QDesktopWidget>
#include <QList>
#include <QBluetoothAddress>
#ifdef Q_OS_MAC
#include <QBluetoothUuid>
#endif
#include <QBluetoothDeviceInfo>
#include "target.h"

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class ScanSelection;
}

typedef struct
{
    QBluetoothDeviceInfo diDeviceInfo;
} DeviceInfoStruct;

/******************************************************************************/
// Class definitions
/******************************************************************************/
class ScanSelection : public QDialog
{
    Q_OBJECT

public:
    explicit
    ScanSelection(
        QWidget *parent = 0
        );
    ~ScanSelection(
        );
    void
    AddDevice(
        QBluetoothDeviceInfo diDeviceInfo
        );
    void
    ClearDevices(
        );
    void
    SetStatus(
        qint8 nNewStatus
        );

signals:
    void
    DeviceSelected(
        QBluetoothDeviceInfo diDeviceInfo
        );
    void
    WindowClosed(
        );
    void
    ScanningFinished(
        quint8 unStatus
        );

private slots:
    void
    on_btn_OK_clicked(
        );
    void
    on_btn_Cancel_clicked(
        );
    void
    on_list_Devices_itemDoubleClicked(
        QListWidgetItem *
        );

private:
    Ui::ScanSelection *ui;
    QList<DeviceInfoStruct> lstDeviceArray;
};

#endif // SCANSELECTION_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
