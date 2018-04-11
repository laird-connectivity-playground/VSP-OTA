/******************************************************************************
** Base code by stackoverflow user jaskmar from https://stackoverflow.com/questions/15079406/qt-necessitas-reasonable-qfiledialog-replacement-skin
**
** Project: UwVSP-OTA
**
** Module: androidfiledialog.h
**
** Notes:
**
**
*******************************************************************************/
#ifndef ANDROIDFILEDIALOG_H
#define ANDROIDFILEDIALOG_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include <QUrl>
#include <QtAndroid>
#include <QAndroidJniObject>
#include <QAndroidActivityResultReceiver>
#include <QAndroidJniEnvironment>
#include "target.h"

/******************************************************************************/
// Class definitions
/******************************************************************************/
class AndroidFileDialog : public QObject
{
    Q_OBJECT public:
        explicit
        AndroidFileDialog(
            QObject *parent = 0
            );
        virtual
        ~AndroidFileDialog(
            );
        bool
        provideExistingFileName(
            );

    private:
        class ResultReceiver : public QAndroidActivityResultReceiver
        {
                AndroidFileDialog *_dialog;
            public:
                ResultReceiver(
                    AndroidFileDialog *dialog
                    );
                virtual
                ~ResultReceiver(
                    );
                void
                handleActivityResult(
                    int nReceiverRequestCode,
                    int nResultCode,
                    const QAndroidJniObject &jniData
                    );
                QByteArray
                uriToFileData(
                    QAndroidJniObject jniURI
                    );
                QString
                uriToFilename(
                    QAndroidJniObject jniURI
                    );
        };
        static const int nEXISTING_FILE_NAME_REQUEST = 1;
        ResultReceiver *receiver;
        void
        emitExistingFileNameReady(
            QString strFilename,
            QByteArray baResult
            );

    signals:
        void
        existingFileNameReady(
            QString strFilename,
            QByteArray baResult
            );
};

#endif // ANDROIDFILEDIALOG_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
