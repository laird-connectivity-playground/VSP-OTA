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
    stgSettingsHandle = NULL;
}

//=============================================================================
//=============================================================================
SettingsStorage::~SettingsStorage(
    )
{
    //Destructor
    if (stgSettingsHandle != NULL)
    {
        delete stgSettingsHandle;
    }
}

//=============================================================================
//=============================================================================
qint8
SettingsStorage::LoadSettings(
    )
{
    //Load settings
    stgSettingsHandle = new QSettings(SETTINGS_FILENAME);
    if (!stgSettingsHandle->isWritable())
    {
        //Error whilst loading settings (not writable)
        return SETTINGS_LOAD_ERROR;
    }

    if (!stgSettingsHandle->contains(SETTINGS_KEY_VERSION))
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
    stgSettingsHandle->clear();
    stgSettingsHandle->setValue(SETTINGS_KEY_VERSION, APP_VERSION);
    stgSettingsHandle->setValue(SETTINGS_KEY_UUID, SETTINGS_VALUE_UUID);
    stgSettingsHandle->setValue(SETTINGS_KEY_TX_OFFSET, SETTINGS_VALUE_TX_OFFSET);
    stgSettingsHandle->setValue(SETTINGS_KEY_RX_OFFSET, SETTINGS_VALUE_RX_OFFSET);
    stgSettingsHandle->setValue(SETTINGS_KEY_MO_OFFSET, SETTINGS_VALUE_MO_OFFSET);
    stgSettingsHandle->setValue(SETTINGS_KEY_MI_OFFSET, SETTINGS_VALUE_MI_OFFSET);
    stgSettingsHandle->setValue(SETTINGS_KEY_RESTRICTUUID, SETTINGS_VALUE_RESTRICTUUID);
#ifdef Q_OS_ANDROID
    stgSettingsHandle->setValue(SETTINGS_KEY_COMPATIBLESCAN, SETTINGS_VALUE_COMPATIBLESCAN);
#endif
    stgSettingsHandle->setValue(SETTINGS_KEY_PACKETSIZE, SETTINGS_VALUE_PACKETSIZE);
    stgSettingsHandle->setValue(SETTINGS_KEY_ONLINEXCOMP, SETTINGS_VALUE_ONLINEXCOMP);
    stgSettingsHandle->setValue(SETTINGS_KEY_SSL, SETTINGS_VALUE_SSL);
    stgSettingsHandle->setValue(SETTINGS_KEY_LASTDIR, SETTINGS_VALUE_LASTDIR);
    stgSettingsHandle->setValue(SETTINGS_KEY_DELFILE, SETTINGS_VALUE_DELFILE);
    stgSettingsHandle->setValue(SETTINGS_KEY_VERIFYFILE, SETTINGS_VALUE_VERIFYFILE);
    stgSettingsHandle->setValue(SETTINGS_KEY_DOWNLOADACTION, SETTINGS_VALUE_DOWNLOADACTION);
    stgSettingsHandle->setValue(SETTINGS_KEY_SKIPDLDISPLAY, SETTINGS_VALUE_SKIPDLDISPLAY);
    stgSettingsHandle->setValue(SETTINGS_KEY_SCROLLBACKSIZE, SETTINGS_VALUE_SCROLLBACKSIZE);
    stgSettingsHandle->setValue(SETTINGS_KEY_CHECKFWVERSION, SETTINGS_VALUE_CHECKFWVERSION);
    stgSettingsHandle->setValue(SETTINGS_KEY_CHECKFREESPACE, SETTINGS_VALUE_CHECKFREESPACE);
}

//=============================================================================
//=============================================================================
QString
SettingsStorage::GetString(
    QString strKey
    )
{
    //Get a string value
    return stgSettingsHandle->value(strKey).toString();
}

//=============================================================================
//=============================================================================
void
SettingsStorage::SetString(
    QString strKey,
    QString strNewValue
    )
{
    //Set a string value
    if (!stgSettingsHandle->value(strKey).isValid() || stgSettingsHandle->value(strKey).isNull() || stgSettingsHandle->value(strKey).toString() != strNewValue)
    {
        stgSettingsHandle->setValue(strKey, strNewValue);
    }
}

//=============================================================================
//=============================================================================
qint8
SettingsStorage::GetInt(
    QString strKey
    )
{
    //Get a signed integer value
    return stgSettingsHandle->value(strKey).toInt();
}

//=============================================================================
//=============================================================================
void
SettingsStorage::SetInt(
    QString strKey,
    qint8 nNewValue
    )
{
    //Set a signed integer value
    if (!stgSettingsHandle->value(strKey).isValid() || stgSettingsHandle->value(strKey).isNull() || stgSettingsHandle->value(strKey).toInt() != nNewValue)
    {
        stgSettingsHandle->setValue(strKey, nNewValue);
    }
}

//=============================================================================
//=============================================================================
quint8
SettingsStorage::GetUInt(
    QString strKey
    )
{
    //Get an unsigned integer value
    return stgSettingsHandle->value(strKey).toUInt();
}

//=============================================================================
//=============================================================================
void
SettingsStorage::SetUInt(
    QString strKey,
    quint8 unNewValue
    )
{
    //Set an unsigned integer value
    if (!stgSettingsHandle->value(strKey).isValid() || stgSettingsHandle->value(strKey).isNull() || stgSettingsHandle->value(strKey).toUInt() != unNewValue)
    {
        stgSettingsHandle->setValue(strKey, unNewValue);
    }
}

//=============================================================================
//=============================================================================
bool
SettingsStorage::GetBool(
    QString strKey
    )
{
    //Get a bool value
    return stgSettingsHandle->value(strKey).toBool();
}

//=============================================================================
//=============================================================================
void
SettingsStorage::SetBool(
    QString strKey,
    bool bNewValue
    )
{
    //Set a bool value
    if (!stgSettingsHandle->value(strKey).isValid() || stgSettingsHandle->value(strKey).isNull() || stgSettingsHandle->value(strKey).toBool() != bNewValue)
    {
        stgSettingsHandle->setValue(strKey, bNewValue);
    }
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
