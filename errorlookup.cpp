/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: errorlookup.cpp
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
#include "errorlookup.h"

//=============================================================================
//=============================================================================
ErrorLookup::ErrorLookup(QObject *parent) : QObject(parent)
{
    //Constructor
    IsOpen = false;
    if (QFile::exists(":/Internal/codes.csv"))
    {
        //Internal error code file exists - open it
        IsOpen = true;
        pErrorMessages = new QSettings(QString(":/Internal/codes.csv"), QSettings::IniFormat);
    }
}

//=============================================================================
//=============================================================================
ErrorLookup::~ErrorLookup(
    )
{
    //Destructor
    if (IsOpen == true)
    {
        delete pErrorMessages;
    }
}

//=============================================================================
//=============================================================================
QString
ErrorLookup::LookupError(
    qint32 nErrorCode
    )
{
    //Returns the string of an error code
    if (IsOpen == false)
    {
        return "Undefined Error Code";
    }
    return pErrorMessages->value(QString::number(nErrorCode), "Undefined Error Code").toString();
}

//=============================================================================
//=============================================================================
QString
ErrorLookup::DatabaseVersion(
    )
{
    //Returns the version of the error code database
    if (IsOpen == false)
    {
        return "(Not Loaded)";
    }
    return pErrorMessages->value("Version", "0.00").toString();
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
