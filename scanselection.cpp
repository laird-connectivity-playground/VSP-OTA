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
    QPalette CPalette = this->palette();
    QColor CColor = CPalette.color(QPalette::Active, QPalette::Window).toRgb();
    CColor.setRed(CColor.red()-15);
    CColor.setBlue(CColor.blue()-15);
    CColor.setGreen(CColor.green()-15);
    CPalette.setColor(QPalette::Active, QPalette::Window, CColor);
    this->setPalette(CPalette);
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
    QBluetoothDeviceInfo DeviceInfo
    )
{
    //Adds device to selection list
    int i = 0;
    while (i < DeviceArray.count())
    {
#ifdef Q_OS_MAC
        if (DeviceArray[i].DeviceUUID == DeviceInfo.deviceUuid())
#else
        if (DeviceArray[i].DeviceAddress == DeviceInfo.address())
#endif
        {
            //Update
            DeviceArray[i].DeviceInfo = DeviceInfo;
#ifdef Q_OS_MAC
            ui->list_Devices->item(i)->setText(QString(DeviceInfo.name()).append(" (").append(QString::number(DeviceInfo.rssi())).append(")"));
#else
            ui->list_Devices->item(i)->setText(QString(DeviceInfo.address().toString()).append(": ").append(DeviceInfo.name()).append(" (").append(QString::number(DeviceInfo.rssi())).append(")"));
#endif
            return;
        }
        ++i;
    }

    //
    DeviceInfoStruct tmpstruct;
#ifdef Q_OS_MAC
    tmpstruct.DeviceUUID = DeviceInfo.deviceUuid();
#else
    tmpstruct.DeviceAddress = DeviceInfo.address();
#endif
    tmpstruct.DeviceInfo = DeviceInfo;
    DeviceArray.append(tmpstruct);
#ifdef Q_OS_MAC
    ui->list_Devices->addItem(QString(DeviceInfo.name()).append(" (").append(QString::number(DeviceInfo.rssi())).append(")"));
#else
    ui->list_Devices->addItem(QString(DeviceInfo.address().toString()).append(": ").append(DeviceInfo.name()).append(" (").append(QString::number(DeviceInfo.rssi())).append(")"));
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
    DeviceArray.clear();
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
        emit DeviceSelected(DeviceArray.at(ui->list_Devices->currentRow()).DeviceInfo);
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
    qint8 NewStatus
    )
{
    //Change the status text
    if (NewStatus == STATUS_LOADING)
    {
        ui->label_Status->setText("Scanning...");
    }
    else if (NewStatus == STATUS_STANDBY)
    {
        ui->label_Status->setText("Finished scanning.");
    }
    emit ScanningFinished(NewStatus);
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
