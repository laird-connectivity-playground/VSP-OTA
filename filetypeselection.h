/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: filetypeselection.h
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
#ifndef FILETYPESELECTION_H
#define FILETYPESELECTION_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QDialog>
#include <QRegularExpression>
#include <QClipboard>
#include "target.h"

/******************************************************************************/
// Forward declaration of Class, Struct & Unions
/******************************************************************************/
namespace Ui
{
    class FileTypeSelection;
}

/******************************************************************************/
// Class definitions
/******************************************************************************/
class FileTypeSelection : public QDialog
{
    Q_OBJECT

public:
    explicit
    FileTypeSelection(
        QWidget *parent = 0
        );
    ~FileTypeSelection(
        );
#ifdef Q_OS_ANDROID
    void
    FixBrokenQtTextHeight(
        );
#endif

private slots:
    void
    on_buttons_accepted(
        );
    void
    on_buttons_rejected(
        );
    void
    on_btn_Paste_clicked(
        );

signals:
    void
    FileTypeChanged(
        qint8 nNewType,
        QString strData
        );
    void
    DisplayMessage(
        QString strMessage,
        bool bToastLong
        );

private:
    Ui::FileTypeSelection *ui;
#ifdef Q_OS_ANDROID
    bool bAndroidTextHeightFixed;
#endif
    QRegularExpression rxpURL;
};

#endif // FILETYPESELECTION_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
