/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: target.h
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
#ifndef TARGET_H
#define TARGET_H

/******************************************************************************/
// Defines
/******************************************************************************/
#define ENABLE_DEBUG                     //Set to allow debugging messages using QDebug
#define UseSSL                           //Set to enable SSL (requires OpenSSL libraries)

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QString>
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#endif
#ifdef ENABLE_DEBUG
#include <QDebug>
#endif

/******************************************************************************/
// Constants
/******************************************************************************/
//Version of the application
const QString APP_VERSION                    = "0.93";

//Download actions
const quint8  DOWNLOAD_ACTION_NOTHING        = 0;
const quint8  DOWNLOAD_ACTION_DISCONNECT     = 1;
const quint8  DOWNLOAD_ACTION_RESTART        = 2;

//Keys and default values for settings
const QString SETTINGS_KEY_VERSION           = "Version";
const QString SETTINGS_KEY_UUID              = "UUID";
const QString SETTINGS_VALUE_UUID            = "569a1101-b87f-490c-92cb-11ba5ea5167c";
const QString SETTINGS_KEY_TX_OFFSET         = "TxChar";
const QString SETTINGS_VALUE_TX_OFFSET       = "2000";
const QString SETTINGS_KEY_RX_OFFSET         = "RxChar";
const QString SETTINGS_VALUE_RX_OFFSET       = "2001";
const QString SETTINGS_KEY_MO_OFFSET         = "MoChar";
const QString SETTINGS_VALUE_MO_OFFSET       = "2002";
const QString SETTINGS_KEY_MI_OFFSET         = "MiChar";
const QString SETTINGS_VALUE_MI_OFFSET       = "2003";
const QString SETTINGS_KEY_RESTRICTUUID      = "RestrictUUID";
//const bool    SETTINGS_VALUE_RESTRICTUUID    = true;
const bool    SETTINGS_VALUE_RESTRICTUUID    = false;
const QString SETTINGS_KEY_COMPATIBLESCAN    = "CompatibleScan";
const bool    SETTINGS_VALUE_COMPATIBLESCAN  = false;
const QString SETTINGS_KEY_PACKETSIZE        = "PacketSize";
const quint8  SETTINGS_VALUE_PACKETSIZE      = 20;
const QString SETTINGS_KEY_ONLINEXCOMP       = "OnlineXComp";
const bool    SETTINGS_VALUE_ONLINEXCOMP     = true;
const QString SETTINGS_KEY_SSL               = "EnableSSL";
const bool    SETTINGS_VALUE_SSL             = false;
const QString SETTINGS_KEY_LASTDIR           = "LastDir";
const QString SETTINGS_VALUE_LASTDIR         = "";
const QString SETTINGS_KEY_DELFILE           = "DelFile";
const bool    SETTINGS_VALUE_DELFILE         = true;
const QString SETTINGS_KEY_VERIFYFILE        = "VerifyFile";
const bool    SETTINGS_VALUE_VERIFYFILE      = true;
//const QString SETTINGS_KEY_DCAFTER           = "DCAfter";
//const bool    SETTINGS_VALUE_DCAFTER         = true;
const QString SETTINGS_KEY_DOWNLOADACTION    = "DownloadAction";
const quint8  SETTINGS_VALUE_DOWNLOADACTION  = DOWNLOAD_ACTION_DISCONNECT;
const QString SETTINGS_KEY_SKIPDLDISPLAY     = "SkipDownloadDisplay";
const bool    SETTINGS_VALUE_SKIPDLDISPLAY   = false;
const QString SETTINGS_KEY_SCROLLBACKSIZE    = "ScrollbackSize";
const quint8  SETTINGS_VALUE_SCROLLBACKSIZE  = 32;
const QString SETTINGS_KEY_CHECKFWVERSION     = "CheckFirmwareVersion";
const bool    SETTINGS_VALUE_CHECKFWVERSION   = true;
const QString SETTINGS_KEY_CHECKFREESPACE     = "CheckFreeSpace";
const bool    SETTINGS_VALUE_CHECKFREESPACE   = true;

//Values for application status
const quint8  STATUS_STANDBY                 = 0;
const quint8  STATUS_LOADING                 = 1;

//Values for loading image (in status bar) size
const int     LOADING_IMAGE_WIDTH            = 32;
const int     LOADING_IMAGE_HEIGHT           = 32;

//Minimum and maximum size of valid sb/uwc files (in bytes) - 128KB
const qint32  FILESIZE_MIN                   = 0;
const qint32  FILESIZE_MAX                   = 131072;

//Error code emitted when a downloaded file's size is not valid
const qint16  DOWNLOAD_FILESIZE_ERROR        = -1;
const qint16  DOWNLOAD_JSON_ERROR            = -2;
const qint16  DOWNLOAD_SERVER_ERROR          = -3;
const qint16  DOWNLOAD_XCOMPILE_ERROR        = -4;
const qint16  DOWNLOAD_SSL_SUPPORT_ERROR     = -5;
const qint16  DOWNLOAD_SSL_CERT_ERROR        = -6;
const qint16  DOWNLOAD_GENERAL_ERROR         = -7;
const qint16  DOWNLOAD_UNSUPPORTED_ERROR     = -8;
const qint16  DOWNLOAD_UNKNOWN_ERROR         = -9;

//Hostname or IP address of the online XCompilation server, and protocols
const QString XCOMPILE_SERVER_HOSTNAME       = "uwterminalx.lairdtech.com";
const QString WEB_PROTOCOL_NORMAL            = "http";
const QString WEB_PROTOCOL_SSL               = "https";

//Where the select file resides
const qint8   FILE_TYPE_LOCALFILE            = 1;
const qint8   FILE_TYPE_REMOTEURL            = 2;
const qint8   FILE_TYPE_DROPBOX              = 3;

//What mode the main application is in (CurrentMode)
const quint8  MAIN_MODE_IDLE                 = 0;
const quint8  MAIN_MODE_CONNECTING           = 1;
const quint8  MAIN_MODE_DISCOVERING          = 2;
const quint8  MAIN_MODE_VERSION              = 3;
const quint8  MAIN_MODE_SPACECHECK           = 4;
const quint8  MAIN_MODE_XCOMPILING           = 5;
const quint8  MAIN_MODE_ONLINE_DOWNLOAD      = 6;
const quint8  MAIN_MODE_DOWNLOADING          = 7;
const quint8  MAIN_MODE_VERIFYING            = 8;
const quint8  MAIN_MODE_QUERY                = 9;
const quint8  MAIN_MODE_FIRMWAREVERSION      = 10;

//What mode the downloader module is in
const quint8  DOWNLOAD_MODE_IDLE             = 0;
const quint8  DOWNLOAD_MODE_DEV_SUPPORTED    = 1;
const quint8  DOWNLOAD_MODE_XCOMPILE         = 2;
const quint8  DOWNLOAD_MODE_DOWNLOAD_FILE    = 3;
const quint8  DOWNLOAD_MODE_LATEST_FIRMWARE  = 4;

//Responses to firmware version checking
const quint8  FIRMWARE_CHECK_OLD             = 1;
const quint8  FIRMWARE_CHECK_CURRENT         = 2;
const quint8  FIRMWARE_CHECK_TEST            = 3;
const quint8  FIRMWARE_CHECK_UNSUPPORTED     = 99;

//Timeout interval for a timeout response
const quint16 TIMEOUT_TIMER_INTERVAL         = 3000;

//Timeout for scanning for devices
const quint16 TIMEOUT_BLE_SCAN               = 20000;

//Settings load return codes
const qint8   SETTINGS_LOAD_OK               = 0;
const qint8   SETTINGS_LOAD_NONE             = 1;
const qint8   SETTINGS_LOAD_ERROR            = 2;

//Settings save return codes
const qint8   SETTINGS_SAVE_OK               = 0;
const qint8   SETTINGS_SAVE_ERROR            = 1;

//Filename of settings file (on supported platforms)
const QString SETTINGS_FILENAME              = "OTA_VSP.ini";

#ifdef Q_OS_ANDROID
//Duration for displaying notifications on Android (1 = long, 0 = short)
const jint    ANDROID_TOAST_DURATION_SHORT   = 0;
const jint    ANDROID_TOAST_DURATION_LONG    = 1;
#endif

#endif // TARGET_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
