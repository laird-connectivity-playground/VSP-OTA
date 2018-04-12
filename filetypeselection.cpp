/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: filetypeselection.cpp
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
#include "filetypeselection.h"
#include "ui_filetypeselection.h"

//=============================================================================
//=============================================================================
FileTypeSelection::FileTypeSelection(QWidget *parent) : QDialog(parent), ui(new Ui::FileTypeSelection)
{
    //Constructor
    ui->setupUi(this);

    //Currently not implemented
    ui->radio_Dropbox->deleteLater();

#ifdef Q_OS_ANDROID
    //Make the dialogue stand out on android platforms
    QPalette palCPalette = this->palette();
    QColor colCColor = palCPalette.color(QPalette::Active, QPalette::Window).toRgb();
    colCColor.setRed(colCColor.red()-15);
    colCColor.setBlue(colCColor.blue()-15);
    colCColor.setGreen(colCColor.green()-15);
    palCPalette.setColor(QPalette::Active, QPalette::Window, colCColor);
    this->setPalette(palCPalette);

    //Text height not yet fixed
    bAndroidTextHeightFixed = false;
#else
    //Remove android information label for non-android builds
    ui->label_AndroidInfo->deleteLater();
#endif

    //Setup URL verification regular expression
    rxpURL.setPattern("http(s)?://(.*?)/(.*).([a-z0-9]{2,4})");
    rxpURL.setPatternOptions(QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption);
}

//=============================================================================
//=============================================================================
FileTypeSelection::~FileTypeSelection(
    )
{
    //Destructor
    delete ui;
}

//=============================================================================
//=============================================================================
void
FileTypeSelection::on_buttons_accepted(
    )
{
    //Accept button has been clicked
    qint8 unFileTypeSelected = -5;
    if (ui->radio_LocalFile->isChecked())
    {
        //Local file
        unFileTypeSelected = FILE_TYPE_LOCALFILE;
    }
    else if (ui->radio_RemoteURL->isChecked())
    {
        //Remote URL - check that URL is valid
        QRegularExpressionMatch rexpmURLMatch = rxpURL.match(ui->edit_RemoteFileURL->text());

        if (!rexpmURLMatch.hasMatch())
        {
            //Selected filetype or URL not valid
            emit DisplayMessage("Supplied URL to .sb/.uwc/.txt file is not valid.", false);
            return;
        }

        //Set type
        unFileTypeSelected = FILE_TYPE_REMOTEURL;
    }
#ifndef Q_OS_ANDROID
    else if (ui->radio_Dropbox->isChecked())
    {
        //Dropbox
        unFileTypeSelected = FILE_TYPE_DROPBOX;
    }
#endif

    //Pass this information back to the parent
    if (unFileTypeSelected != -5)
    {
        emit FileTypeChanged(unFileTypeSelected , (unFileTypeSelected  == FILE_TYPE_REMOTEURL ? ui->edit_RemoteFileURL->text() : NULL));
    }
    this->close();
}

//=============================================================================
//=============================================================================
void
FileTypeSelection::on_buttons_rejected(
    )
{
    //Close the select window
    this->close();
}

//=============================================================================
//=============================================================================
void
FileTypeSelection::on_btn_Paste_clicked(
    )
{
    //Paste button because Android Qt's line edit cursor position has been broken for years... QTBUG-58503
    ui->edit_RemoteFileURL->setText(QApplication::clipboard()->text());
    ui->radio_RemoteURL->setChecked(true);
}

//=============================================================================
//=============================================================================
#ifdef Q_OS_ANDROID
void
FileTypeSelection::FixBrokenQtTextHeight(
    )
{
    //Fixes broken text height on android QTBUG-67487
    if (bAndroidTextHeightFixed == false)
    {
        QFontMetrics fmFontMet(ui->label_AndroidInfo->font());
        ui->label_AndroidInfo->setMinimumHeight(fmFontMet.boundingRect(0, 0, ui->label_AndroidInfo->width(), ui->label_AndroidInfo->height(), Qt::TextWordWrap, ui->label_AndroidInfo->text()).height() + 4);
        bAndroidTextHeightFixed = true;
    }
}
#endif

/******************************************************************************/
// END OF FILE
/******************************************************************************/
