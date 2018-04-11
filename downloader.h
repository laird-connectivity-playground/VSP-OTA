/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: downloader.h
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
#ifndef DOWNLOADER_H
#define DOWNLOADER_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QWidget>
#include "target.h"
#ifdef UseSSL
#include <QFile>
#endif

/******************************************************************************/
// Class definitions
/******************************************************************************/
class Downloader : public QWidget
{
    Q_OBJECT

public:
    Downloader(
        QWidget *parent = 0
        );
    ~Downloader(
        );
    void
    XCompileFile(
        QString strDevID,
        QString strLanguageHashA,
        QString strLanguageHashB,
        QByteArray *baSourceFileData
        );
    void
    SetSSLSupport(
        bool bNewEnableSSL
        );
    void
    DownloadFile(
        QString strDownloadURL
        );
    void
    CancelRequest(
        );
    void
    CheckLatestFirmware(
        QString strDevID,
        QString strFirmwareVersion
        );

private slots:
    void
    replyFinished(
        QNetworkReply* nrReply
        );
#ifdef UseSSL
    void
    sslErrors(
        QNetworkReply* nrReply,
        QList<QSslError> lstSSLErrors
        );
#endif

signals:
    void
    XCompileComplete(
        bool bSuccess,
        qint16 nErrorCode,
        QByteArray baFileData
        );
    void
    FileDownloaded(
        bool bSuccess,
        qint16 nErrorCode,
        QByteArray baFileData
        );
    void
    FirmwareResponse(
        bool bSuccess,
        qint16 nErrorCode,
        QString strLatestFirmware
        );
    void
    StatusChanged(
        quint8 unStatus
        );

private:
    QNetworkAccessManager *nmManager;
    QByteArray *baFileData;
    quint8 unDownloaderMode;
    bool bEnableSSL;
#ifdef UseSSL
    QSslCertificate *sslcLairdSSL = NULL; //Holds the Laird SSL certificate
#endif
    QNetworkReply *nmrLastRequest;
};

#endif // DOWNLOADER_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
