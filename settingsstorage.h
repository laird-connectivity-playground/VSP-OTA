/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: settingsstorage.h
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
#ifndef SETTINGSSTORAGE_H
#define SETTINGSSTORAGE_H

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
class SettingsStorage : public QObject
{
    Q_OBJECT

public:
    explicit
    SettingsStorage(
        QObject *parent = nullptr
        );
    ~SettingsStorage(
        );
    qint8
    LoadSettings(
        );
    qint8
    SaveSettings(
        );
    void
    DefaultSettings(
        );
    QString
    GetString(
        QString Key
        );
    void
    SetString(
        QString Key,
        QString NewValue
        );
    qint8
    GetInt(
        QString Key
        );
    void
    SetInt(
        QString Key,
        qint8 NewValue
        );
    quint8
    GetUInt(
        QString Key
        );
    void
    SetUInt(
        QString Key,
        quint8 NewValue
        );
    bool
    GetBool(
        QString Key
        );
    void
    SetBool(
        QString Key,
        bool NewValue
        );

private:
    QSettings *SettingsHandle;
};

#endif // SETTINGSSTORAGE_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
