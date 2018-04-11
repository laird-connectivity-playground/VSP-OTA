/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: downloader.cpp
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
#include "downloader.h"

Downloader::Downloader(QWidget *parent) : QWidget(parent)
{
    //Constructor
    nmManager = new QNetworkAccessManager();
    connect(nmManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    connect(nmManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrors(QNetworkReply*,QList<QSslError>)));

#ifdef UseSSL
    //Load SSL certificate
    QFile fileCert(":/Internal/UwTerminalX.crt");
    if (fileCert.open(QIODevice::ReadOnly))
    {
        //Load certificate data
        sslcLairdSSL = new QSslCertificate(fileCert.readAll());
        QSslSocket::addDefaultCaCertificate(*sslcLairdSSL);
        fileCert.close();
    }
#endif

    //Disable SSL by default
    baFileData = NULL;
    bEnableSSL = false;

    //
    nmrLastRequest = NULL;
}

//=============================================================================
//=============================================================================
Downloader::~Downloader(
    )
{
    //Destructor
    if (baFileData != NULL)
    {
        baFileData->clear();
        delete baFileData;
    }
    disconnect(nmManager, 0, 0, 0);
    delete nmManager;

#ifdef UseSSL
    //Clean up SSL certificate
    if (sslcLairdSSL != NULL)
    {
        delete sslcLairdSSL;
        sslcLairdSSL = NULL;
    }
#endif
}

//=============================================================================
//=============================================================================
#ifdef UseSSL
void
Downloader::sslErrors(
    QNetworkReply *nrReply,
    QList<QSslError> lstSSLErrors
    )
{
    //Error detected with SSL
#ifdef ENABLE_DEBUG
    qDebug() << "SSL error: " << lstSSLErrors;
#endif
    if (sslcLairdSSL != NULL && nrReply->sslConfiguration().peerCertificate() == *sslcLairdSSL)
    {
        //Server certificate matches
#ifdef ENABLE_DEBUG
    qDebug() << "Accepted Laird SSL certificate";
#endif
        nrReply->ignoreSslErrors(lstSSLErrors);
    }
    else
    {
        //Certificate error
        nmrLastRequest = NULL;
        if (unDownloaderMode == DOWNLOAD_MODE_DEV_SUPPORTED || unDownloaderMode == DOWNLOAD_MODE_XCOMPILE)
        {
            emit XCompileComplete(false, DOWNLOAD_SSL_CERT_ERROR, NULL);
        }
        else if (unDownloaderMode == DOWNLOAD_MODE_DOWNLOAD_FILE)
        {
            emit FileDownloaded(false, DOWNLOAD_SSL_CERT_ERROR, NULL);
        }
        else if (unDownloaderMode == DOWNLOAD_MODE_LATEST_FIRMWARE)
        {
            emit FirmwareResponse(false, DOWNLOAD_SSL_CERT_ERROR, NULL);
        }

        //Set status back to idle
        unDownloaderMode = DOWNLOAD_MODE_IDLE;
    }
}
#endif

//=============================================================================
//=============================================================================
void
Downloader::replyFinished(
    QNetworkReply* nrReply
    )
{
    //Response received from server regarding online XCompilation
    nmrLastRequest = NULL;
    if (nrReply->error() != QNetworkReply::NoError && nrReply->error() != QNetworkReply::ServiceUnavailableError)
    {
        //An error occured
        if (baFileData != NULL)
        {
            //Clean up
            baFileData->clear();
            delete baFileData;
            baFileData = NULL;
        }

#ifdef ENABLE_DEBUG
        qDebug() << nrReply->error();
        qDebug() << nrReply->errorString();
        qDebug() << "Error: " << nrReply->readAll();
#endif

        //Pass back error to parent
        if (unDownloaderMode == DOWNLOAD_MODE_DEV_SUPPORTED || unDownloaderMode == DOWNLOAD_MODE_XCOMPILE)
        {
            emit XCompileComplete(false, DOWNLOAD_GENERAL_ERROR, nrReply->errorString().toUtf8());
        }
        else if (unDownloaderMode == DOWNLOAD_MODE_DOWNLOAD_FILE)
        {
            emit FileDownloaded(false, DOWNLOAD_GENERAL_ERROR, nrReply->errorString().toUtf8());
        }
        else if (unDownloaderMode == DOWNLOAD_MODE_LATEST_FIRMWARE)
        {
            emit FirmwareResponse(false, DOWNLOAD_GENERAL_ERROR, nrReply->errorString());
        }

        //Set status back to idle
        unDownloaderMode = DOWNLOAD_MODE_IDLE;
    }
    else
    {
        if (unDownloaderMode == DOWNLOAD_MODE_DEV_SUPPORTED)
        {
            //Check if device is supported
            QJsonParseError jpeJsonError;
            QJsonDocument jdJsonData = QJsonDocument::fromJson(nrReply->readAll(), &jpeJsonError);
            if (jpeJsonError.error == QJsonParseError::NoError)
            {
                //Decoded JSON
                QJsonObject joJsonObject = jdJsonData.object();
                if (nrReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 503)
                {
                    //Server responded with error - device not supported
#ifdef ENABLE_DEBUG
                    qDebug() << "e1";
#endif

                    //Pass this back to the parent
                    if (joJsonObject["Result"].toString() == "-3")
                    {
                        //Unsupported device/firmware
                        emit XCompileComplete(false, DOWNLOAD_UNSUPPORTED_ERROR, "Your device and/or firmware are not supported.");
                    }
                    else
                    {
                        //Other error
                        emit XCompileComplete(false, DOWNLOAD_UNSUPPORTED_ERROR, joJsonObject["Error"].toString().toUtf8());
                    }

                    //Set status back to idle
                    unDownloaderMode = DOWNLOAD_MODE_IDLE;
                }
                else if (nrReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
                {
                    //Server responded with OK
                    if (joJsonObject["Result"].toString() == "1")
                    {
                        //Device supported, XCompile application
                        QNetworkRequest nrThisReq(QUrl(QString((bEnableSSL == true ? WEB_PROTOCOL_SSL : WEB_PROTOCOL_NORMAL)).append("://").append(XCOMPILE_SERVER_HOSTNAME).append("/xcompile.php?JSON=1")));
                        QByteArray baPostData;
                        baPostData.append("-----------------------------17192614014659\r\nContent-Disposition: form-data; name=\"file_XComp\"\r\n\r\n").append(joJsonObject["ID"].toString()).append("\r\n");
                        baPostData.append("-----------------------------17192614014659\r\nContent-Disposition: form-data; name=\"file_sB\"; filename=\"test.sb\"\r\nContent-Type: application/octet-stream\r\n\r\n");

                        //Set the mode
                        unDownloaderMode = DOWNLOAD_MODE_XCOMPILE;

                        //Append the data to the POST request
                        baPostData.append(*baFileData);
                        baPostData.append("\r\n-----------------------------17192614014659--\r\n");
                        nrThisReq.setRawHeader("Content-Type", "multipart/form-data; boundary=---------------------------17192614014659");
                        nrThisReq.setRawHeader("Content-Length", QString(baPostData.length()).toUtf8());
                        nmrLastRequest = nmManager->post(nrThisReq, baPostData);
#ifdef ENABLE_DEBUG
                        qDebug() << "Sent XCompile request...";
#endif
                        emit StatusChanged(DOWNLOAD_MODE_XCOMPILE);
                    }
                    else
                    {
                        //Device should be supported but something went wrong...
#ifdef ENABLE_DEBUG
                        qDebug() << "Device should be supported but something went wrong";
#endif
                        emit XCompileComplete(false, DOWNLOAD_UNKNOWN_ERROR, NULL);
                    }
                }
                else
                {
                    //Unknown response
#ifdef ENABLE_DEBUG
                    qDebug() << "Server response is not known.";
#endif
                    emit XCompileComplete(false, DOWNLOAD_UNKNOWN_ERROR, NULL);
                }
            }
            else
            {
                //Error whilst decoding JSON
#ifdef ENABLE_DEBUG
                qDebug() << "Error whilst decoding JSON";
#endif
                emit XCompileComplete(false, DOWNLOAD_JSON_ERROR, NULL);
            }

            //Clean up
            baFileData->clear();
            delete baFileData;
            baFileData = NULL;
        }
        else if (unDownloaderMode == DOWNLOAD_MODE_XCOMPILE)
        {
            //XCompile result
            unDownloaderMode = DOWNLOAD_MODE_IDLE;
            if (nrReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 503)
            {
#ifdef ENABLE_DEBUG
                qDebug() << "Error during XCompilation";
#endif
                //Error compiling
                QJsonParseError jpeJsonError;
                QJsonDocument jdJsonData = QJsonDocument::fromJson(nrReply->readAll(), &jpeJsonError);
                if (jpeJsonError.error == QJsonParseError::NoError)
                {
                    //Decoded JSON
                    QJsonObject joJsonObject = jdJsonData.object();

                    //Server responded with error
                    if (joJsonObject["Result"].toString() == "-9")
                    {
                        //Error whilst compiling, show results
#ifdef ENABLE_DEBUG
                        qDebug() << "  -> Error: " << joJsonObject["Error"].toString() << " - " << joJsonObject["Description"].toString();
#endif
                        emit XCompileComplete(false, DOWNLOAD_XCOMPILE_ERROR, QString("Failed to compile ").append(joJsonObject["Result"].toString()).append("; ").append(joJsonObject["Error"].toString().append("\r\n").append(joJsonObject["Description"].toString())).toUtf8());
                    }
                    else
                    {
                        //Server responded with error
#ifdef ENABLE_DEBUG
                        qDebug() << "  -> Server error: " << joJsonObject["Result"].toString() << " - " << joJsonObject["Error"].toString();
#endif
                        emit XCompileComplete(false, DOWNLOAD_SERVER_ERROR, QString("Server responded with error code ").append(joJsonObject["Result"].toString()).append("; ").append(joJsonObject["Error"].toString()).toUtf8());
                    }
                }
                else
                {
                    //Error whilst decoding JSON
#ifdef ENABLE_DEBUG
                    qDebug() << "  -> Error decoding JSON response";
#endif
                    emit XCompileComplete(false, DOWNLOAD_JSON_ERROR, NULL);
                }
            }
            else if (nrReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
            {
                //Compiled - save file
#ifdef ENABLE_DEBUG
                qDebug() << "XCompilation complete";
//                qDebug() << "File data: " << *baFileData;
#endif
                emit XCompileComplete(true, 0, nrReply->readAll());
            }
            else
            {
                //Unknown response
#ifdef ENABLE_DEBUG
                qDebug() << "XCompilation failed with unknown response.";
#endif
                emit XCompileComplete(false, nrReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), nrReply->readAll());
            }
        }
        else if (unDownloaderMode == DOWNLOAD_MODE_DOWNLOAD_FILE)
        {
            //Download remote file
            unDownloaderMode = DOWNLOAD_MODE_IDLE;
#ifdef ENABLE_DEBUG
            qDebug() << "Remote file download:";
#endif

            //Check if download was successful
            if (nrReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
            {
                //Successfully received file
                if (nrReply->size() > FILESIZE_MIN && nrReply->size() < FILESIZE_MAX)
                {
                    //Valid file size
#ifdef ENABLE_DEBUG
                    qDebug() << "Remote file download:";
#endif
                    emit FileDownloaded(true, 0, nrReply->readAll());
                }
                else
                {
                    //Invalid file size
#ifdef ENABLE_DEBUG
                    qDebug() << "  -> Filesize too big/small, got: " << nrReply->size() << ", expected " << FILESIZE_MIN << "-" << FILESIZE_MAX;
#endif
                    emit FileDownloaded(false, DOWNLOAD_FILESIZE_ERROR, NULL);
                }
            }
            else
            {
                //Error downloading file
#ifdef ENABLE_DEBUG
                qDebug() << "  -> Error downloading file: " << nrReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
#endif
                emit FileDownloaded(false, nrReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), NULL);
            }
        }
        else if (unDownloaderMode == DOWNLOAD_MODE_LATEST_FIRMWARE)
        {
            //Firmware version response
            unDownloaderMode = DOWNLOAD_MODE_IDLE;
            if (nrReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 503)
            {
#ifdef ENABLE_DEBUG
                qDebug() << "Unsupported/invalid device/firmware";
#endif
                //Unsupported/invalid device/firmware
                emit FirmwareResponse(false, DOWNLOAD_JSON_ERROR, NULL);
            }
            else if (nrReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)
            {
                //Firmware response
                QJsonParseError jpeJsonError;
                QJsonDocument jdJsonData = QJsonDocument::fromJson(nrReply->readAll(), &jpeJsonError);
                if (jpeJsonError.error == QJsonParseError::NoError)
                {
                    //Decoded JSON
                    QJsonObject joJsonObject = jdJsonData.object();

#ifdef ENABLE_DEBUG
                    qDebug() << "Valid firmware response, Result: " << joJsonObject["Result"].toString() << ", Error: " << joJsonObject["Error"].toString() << ", Firmware: " << joJsonObject["Firmware"].toString();
#endif

                    if (joJsonObject["Result"].toString().toInt() == FIRMWARE_CHECK_OLD || joJsonObject["Result"].toString().toInt() == FIRMWARE_CHECK_CURRENT || joJsonObject["Result"].toString().toInt() == FIRMWARE_CHECK_TEST)
                    {
                        //Supported, pass back to parent
                        emit FirmwareResponse(true, joJsonObject["Result"].toString().toInt(), (joJsonObject["Result"].toString().toInt() == FIRMWARE_CHECK_OLD ? joJsonObject["Firmware"].toString() : NULL));
                    }
                    else
                    {
                        //Unsupported
                        emit FirmwareResponse(true, FIRMWARE_CHECK_UNSUPPORTED, NULL);
                    }
                }
                else
                {
                    //JSON decoding failed
                    emit FirmwareResponse(false, DOWNLOAD_JSON_ERROR, NULL);
                }
            }
            else
            {
                //Unknown response
#ifdef ENABLE_DEBUG
                qDebug() << "Firmware response failed with unknown response.";
#endif
                emit FirmwareResponse(false, nrReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), nrReply->readAll());
            }
        }
    }

    //Queue the network reply object to be deleted
    nrReply->deleteLater();
}

//=============================================================================
//=============================================================================
void
Downloader::XCompileFile(
    QString strDevID,
    QString strLanguageHashA,
    QString strLanguageHashB,
    QByteArray *baSourceFileData
    )
{
    //Send request to check if XCompiler exists
    unDownloaderMode = DOWNLOAD_MODE_DEV_SUPPORTED;
    baFileData = new QByteArray(*baSourceFileData);
    nmrLastRequest = nmManager->get(QNetworkRequest(QUrl(QString((bEnableSSL == true ? WEB_PROTOCOL_SSL : WEB_PROTOCOL_NORMAL)).append("://").append(XCOMPILE_SERVER_HOSTNAME).append("/supported.php?JSON=1&Dev=").append(strDevID).append("&HashA=").append(strLanguageHashA).append("&HashB=").append(strLanguageHashB))));
    emit StatusChanged(DOWNLOAD_MODE_DEV_SUPPORTED);
}

//=============================================================================
//=============================================================================
void
Downloader::SetSSLSupport(
    bool bNewEnableSSL
    )
{
    //Set SSL status
#ifndef QT_NO_SSL
    bEnableSSL = bNewEnableSSL;
#endif
}

//=============================================================================
//=============================================================================
void
Downloader::DownloadFile(
    QString strDownloadURL
    )
{
    //Downloads a file
    unDownloaderMode = DOWNLOAD_MODE_DOWNLOAD_FILE;
#ifdef QT_NO_SSL
    //Check if URL requires SSL
    if (strDownloadURL.length() > 6 && strDownloadURL.left(6).toLower() == "https:")
    {
        //SSL URL detected without SSL support being active - fail
        emit FileDownloaded(false, DOWNLOAD_SSL_SUPPORT_ERROR, NULL);
        return;
    }
#endif
    nmrLastRequest = nmManager->get(QNetworkRequest(QUrl(strDownloadURL)));
    emit StatusChanged(DOWNLOAD_MODE_DOWNLOAD_FILE);
}

//=============================================================================
//=============================================================================
void
Downloader::CancelRequest(
    )
{
    //Cancels a pending web requests
    if (nmrLastRequest != NULL)
    {
        nmrLastRequest->abort();
        nmrLastRequest->deleteLater();
        unDownloaderMode = DOWNLOAD_MODE_IDLE;
    }
}

//=============================================================================
//=============================================================================
void
Downloader::CheckLatestFirmware(
    QString strDevID,
    QString strFirmwareVersion
    )
{
    //Checks if the firmware for a module is the latest version
    unDownloaderMode = DOWNLOAD_MODE_LATEST_FIRMWARE;
    nmrLastRequest = nmManager->get(QNetworkRequest(QUrl(QString((bEnableSSL == true ? WEB_PROTOCOL_SSL : WEB_PROTOCOL_NORMAL)).append("://").append(XCOMPILE_SERVER_HOSTNAME).append("/latest_firmware.php?JSON=1&Dev=").append(strDevID).append("&FW=").append(strFirmwareVersion))));
    emit StatusChanged(DOWNLOAD_MODE_LATEST_FIRMWARE);
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
