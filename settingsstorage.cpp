/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: settingsstorage.cpp
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
#include "settingsstorage.h"

//=============================================================================
//=============================================================================
SettingsStorage::SettingsStorage(QObject *parent) : QObject(parent)
{
    //Constructor
    SettingsHandle = NULL;
}

//=============================================================================
//=============================================================================
SettingsStorage::~SettingsStorage(
    )
{
    //Destructor
    if (SettingsHandle != NULL)
    {
        delete SettingsHandle;
    }
}

//=============================================================================
//=============================================================================
qint8
SettingsStorage::LoadSettings(
    )
{
    //Load settings
    SettingsHandle = new QSettings(SETTINGS_FILENAME);
    if (!SettingsHandle->isWritable())
    {
        //Error whilst loading settings (not writable)
        return SETTINGS_LOAD_ERROR;
    }

    if (!SettingsHandle->contains(SETTINGS_KEY_VERSION))
    {
        //No saved settings present
        return SETTINGS_LOAD_NONE;
    }

    //Settings loaded successfully
    return SETTINGS_LOAD_OK;
}

//=============================================================================
//=============================================================================
qint8
SettingsStorage::SaveSettings(
    )
{
    //Empty save handler as QSettings does this automatically
    return SETTINGS_SAVE_OK;
}

//=============================================================================
//=============================================================================
void
SettingsStorage::DefaultSettings(
    )
{
    //Set default settings
    SettingsHandle->clear();
    SettingsHandle->setValue(SETTINGS_KEY_VERSION, APP_VERSION);
    SettingsHandle->setValue(SETTINGS_KEY_UUID, SETTINGS_VALUE_UUID);
    SettingsHandle->setValue(SETTINGS_KEY_TX_OFFSET, SETTINGS_VALUE_TX_OFFSET);
    SettingsHandle->setValue(SETTINGS_KEY_RX_OFFSET, SETTINGS_VALUE_RX_OFFSET);
    SettingsHandle->setValue(SETTINGS_KEY_MO_OFFSET, SETTINGS_VALUE_MO_OFFSET);
    SettingsHandle->setValue(SETTINGS_KEY_MI_OFFSET, SETTINGS_VALUE_MI_OFFSET);
    SettingsHandle->setValue(SETTINGS_KEY_RESTRICTUUID, SETTINGS_VALUE_RESTRICTUUID);
    SettingsHandle->setValue(SETTINGS_KEY_PACKETSIZE, SETTINGS_VALUE_PACKETSIZE);
    SettingsHandle->setValue(SETTINGS_KEY_ONLINEXCOMP, SETTINGS_VALUE_ONLINEXCOMP);
    SettingsHandle->setValue(SETTINGS_KEY_SSL, SETTINGS_VALUE_SSL);
    SettingsHandle->setValue(SETTINGS_KEY_LASTDIR, SETTINGS_VALUE_LASTDIR);
    SettingsHandle->setValue(SETTINGS_KEY_DELFILE, SETTINGS_VALUE_DELFILE);
    SettingsHandle->setValue(SETTINGS_KEY_VERIFYFILE, SETTINGS_VALUE_VERIFYFILE);
    SettingsHandle->setValue(SETTINGS_KEY_DOWNLOADACTION, SETTINGS_VALUE_DOWNLOADACTION);
    SettingsHandle->setValue(SETTINGS_KEY_SKIPDLDISPLAY, SETTINGS_VALUE_SKIPDLDISPLAY);
    SettingsHandle->setValue(SETTINGS_KEY_SCROLLBACKSIZE, SETTINGS_VALUE_SCROLLBACKSIZE);
    SettingsHandle->setValue(SETTINGS_KEY_CHECKFWVERSION, SETTINGS_VALUE_CHECKFWVERSION);
}

//=============================================================================
//=============================================================================
QString
SettingsStorage::GetString(
    QString Key
    )
{
    //Get a string value
    return SettingsHandle->value(Key).toString();
}

//=============================================================================
//=============================================================================
void
SettingsStorage::SetString(
    QString Key,
    QString NewValue
    )
{
    //Set a string value
    if (!SettingsHandle->value(Key).isValid() || SettingsHandle->value(Key).isNull() || SettingsHandle->value(Key).toString() != NewValue)
    {
        SettingsHandle->setValue(Key, NewValue);
    }
}

//=============================================================================
//=============================================================================
qint8
SettingsStorage::GetInt(
    QString Key
    )
{
    //Get a signed integer value
    return SettingsHandle->value(Key).toInt();
}

//=============================================================================
//=============================================================================
void
SettingsStorage::SetInt(
    QString Key,
    qint8 NewValue
    )
{
    //Set a signed integer value
    if (!SettingsHandle->value(Key).isValid() || SettingsHandle->value(Key).isNull() || SettingsHandle->value(Key).toInt() != NewValue)
    {
        SettingsHandle->setValue(Key, NewValue);
    }
}

//=============================================================================
//=============================================================================
quint8
SettingsStorage::GetUInt(
    QString Key
    )
{
    //Get an unsigned integer value
    return SettingsHandle->value(Key).toUInt();
}

//=============================================================================
//=============================================================================
void
SettingsStorage::SetUInt(
    QString Key,
    quint8 NewValue
    )
{
    //Set an unsigned integer value
    if (!SettingsHandle->value(Key).isValid() || SettingsHandle->value(Key).isNull() || SettingsHandle->value(Key).toUInt() != NewValue)
    {
        SettingsHandle->setValue(Key, NewValue);
    }
}

//=============================================================================
//=============================================================================
bool
SettingsStorage::GetBool(
    QString Key
    )
{
    //Get a bool value
    return SettingsHandle->value(Key).toBool();
}

//=============================================================================
//=============================================================================
void
SettingsStorage::SetBool(
    QString Key,
    bool NewValue
    )
{
    //Set a bool value
    if (!SettingsHandle->value(Key).isValid() || SettingsHandle->value(Key).isNull() || SettingsHandle->value(Key).toBool() != NewValue)
    {
        SettingsHandle->setValue(Key, NewValue);
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
