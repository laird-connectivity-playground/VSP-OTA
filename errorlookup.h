/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: errorlookup.h
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
#ifndef ERRORLOOKUP_H
#define ERRORLOOKUP_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QSettings>
#include <QFile>
#include "target.h"

/******************************************************************************/
// Class definitions
/******************************************************************************/
class ErrorLookup : public QObject
{
    Q_OBJECT
public:
    explicit
    ErrorLookup(
        QObject *parent = nullptr
        );
    ~ErrorLookup(
        );
    QString
    LookupError(
        qint32 nErrorCode
        );
    QString
    DatabaseVersion(
        );

private:
    QSettings *stgErrorMessages;
    bool bIsOpen;
};

#endif // ERRORLOOKUP_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
