/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: scanselection.cpp
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
#include "scanselection.h"
#include "ui_scanselection.h"

ScanSelection::ScanSelection(QWidget *parent) : QDialog(parent), ui(new Ui::ScanSelection)
{
    //Constructor
    ui->setupUi(this);

#ifdef Q_OS_ANDROID
    //Make the dialogue stand out on android platforms
    QPalette palCPalette = this->palette();
    QColor colCColor = palCPalette.color(QPalette::Active, QPalette::Window).toRgb();
    colCColor.setRed(colCColor.red()-15);
    colCColor.setBlue(colCColor.blue()-15);
    colCColor.setGreen(colCColor.green()-15);
    palCPalette.setColor(QPalette::Active, QPalette::Window, colCColor);
    this->setPalette(palCPalette);
#endif

#ifdef Q_OS_ANDROID
    //Set screen size appropriately
    this->setFixedSize((QApplication::desktop()->availableGeometry().width()*75/100), (QApplication::desktop()->availableGeometry().height()*75/100));
#endif
}

//=============================================================================
//=============================================================================
ScanSelection::~ScanSelection(
    )
{
    //Destructor
    delete ui;
}

//=============================================================================
//=============================================================================
void
ScanSelection::AddDevice(
    QBluetoothDeviceInfo diDeviceInfo
    )
{
    //Adds device to selection list
    int i = 0;
    while (i < lstDeviceArray.count())
    {
#ifdef Q_OS_MAC
        if (lstDeviceArray[i].diDeviceInfo.deviceUuid() == diDeviceInfo.deviceUuid())
#else
        if (lstDeviceArray[i].diDeviceInfo.address() == diDeviceInfo.address())
#endif
        {
            //Update
            lstDeviceArray[i].diDeviceInfo = diDeviceInfo;
#ifdef Q_OS_MAC
            ui->list_Devices->item(i)->setText(QString(diDeviceInfo.name()).append(" (").append(QString::number(diDeviceInfo.rssi())).append(")"));
#else
            ui->list_Devices->item(i)->setText(QString(diDeviceInfo.address().toString()).append(": ").append(diDeviceInfo.name()).append(" (").append(QString::number(diDeviceInfo.rssi())).append(")"));
#endif
            return;
        }
        ++i;
    }

    //Append to array
    DeviceInfoStruct disNewDevice;
    disNewDevice.diDeviceInfo = diDeviceInfo;
    lstDeviceArray.append(disNewDevice);
#ifdef Q_OS_MAC
    ui->list_Devices->addItem(QString(diDeviceInfo.name()).append(" (").append(QString::number(diDeviceInfo.rssi())).append(")"));
#else
    ui->list_Devices->addItem(QString(diDeviceInfo.address().toString()).append(": ").append(diDeviceInfo.name()).append(" (").append(QString::number(diDeviceInfo.rssi())).append(")"));
#endif
}

//=============================================================================
//=============================================================================
void
ScanSelection::ClearDevices(
    )
{
    //Clears list of devices
    ui->list_Devices->clear();
    lstDeviceArray.clear();
}

//=============================================================================
//=============================================================================
void
ScanSelection::on_btn_OK_clicked(
    )
{
    //OK button clicked
    if (ui->list_Devices->currentRow() >= 0)
    {
        //A row is selected
        emit DeviceSelected(lstDeviceArray.at(ui->list_Devices->currentRow()).diDeviceInfo);
        this->hide();
    }
}

//=============================================================================
//=============================================================================
void
ScanSelection::on_btn_Cancel_clicked(
    )
{
    //Cancel button clicked
    emit WindowClosed();
    this->hide();
}

//=============================================================================
//=============================================================================
void
ScanSelection::on_list_Devices_itemDoubleClicked(
    QListWidgetItem *
    )
{
    //Item double clicked
    on_btn_OK_clicked();
}

//=============================================================================
//=============================================================================
void
ScanSelection::SetStatus(
    qint8 nNewStatus
    )
{
    //Change the status text
    if (nNewStatus == STATUS_LOADING)
    {
        ui->label_Status->setText("Scanning...");
    }
    else if (nNewStatus == STATUS_STANDBY)
    {
        ui->label_Status->setText("Finished scanning.");
    }
    emit ScanningFinished(nNewStatus);
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
