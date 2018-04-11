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
#include "androidfiledialog.h"

//=============================================================================
//=============================================================================
AndroidFileDialog::ResultReceiver::ResultReceiver(AndroidFileDialog *dialog) : _dialog(dialog)
{
    //Constructor
}

//=============================================================================
//=============================================================================
AndroidFileDialog::ResultReceiver::~ResultReceiver(
    )
{
    //Destructor
}

//=============================================================================
//=============================================================================
void
AndroidFileDialog::ResultReceiver::handleActivityResult(
    int nReceiverRequestCode,
    int nResultCode,
    const QAndroidJniObject &jniData
    )
{
    //Callback from java/android code with file selection
    jint RESULT_OK = QAndroidJniObject::getStaticField<jint>("android/app/Activity", "RESULT_OK");

    if (nReceiverRequestCode == nEXISTING_FILE_NAME_REQUEST && nResultCode == RESULT_OK)
    {
        //Selection accepted
        QAndroidJniObject jniURI = jniData.callObjectMethod("getData", "()Landroid/net/Uri;");
        QByteArray baFileData = uriToFileData(jniURI);
        QString strFilename = uriToFilename(jniURI);

        if (strFilename == NULL)
        {
            //Filename is empty - decode from URI
            strFilename = QUrl::fromPercentEncoding(jniURI.toString().toUtf8());
            if (strFilename.left(4) == "enc=")
            {
                //Encrypted filename
                strFilename.clear();
            }
            else if (strFilename.indexOf("/") != -1)
            {
                //POSIX-style path name
                strFilename = strFilename.right(strFilename.length() - strFilename.lastIndexOf("/") - 1);
            }
            else if (strFilename.indexOf("\\") != -1)
            {
                //Windows-style path name
                strFilename = strFilename.right(strFilename.length() - strFilename.lastIndexOf("\\") - 1);
            }
            else if (strFilename.length() > 32)
            {
                //Very long filename, probably encrypted or otherwise non-readable
                strFilename.clear();
            }
        }
        else
        {
            if (strFilename.indexOf("/") != -1)
            {
                //POSIX-style path name
                strFilename = strFilename.right(strFilename.length() - strFilename.lastIndexOf("/") - 1);
            }
        }
#ifdef ENABLE_DEBUG
qDebug() << "Got filename as: " << strFilename;
#endif
        _dialog->emitExistingFileNameReady(strFilename, baFileData);
    }
    else
    {
        //Selection cancelled
        _dialog->emitExistingFileNameReady(NULL, NULL);
    }
}

//=============================================================================
//=============================================================================
QByteArray
AndroidFileDialog::ResultReceiver::uriToFileData(
    QAndroidJniObject jniURI
    )
{
    //Returns file data from URI as byte array
    QAndroidJniObject jniFileSelector = QAndroidJniObject("org/laird/vsp/FileSelector", "(Landroid/content/Context;)V", QtAndroid::androidContext().object());
    QAndroidJniObject jniFileContentsObject = jniFileSelector.callObjectMethod("readFileContents", "(Landroid/net/Uri;)[B", jniURI.object<jobject>());
    if (jniFileContentsObject.isValid())
    {
        //File data retrieved successfully
        jbyteArray jbaFileContents = jniFileContentsObject.object<jbyteArray>();

        //Convert from a java byte array to a QByteArray
        QAndroidJniEnvironment jnieEnv;
        QByteArray baResultArray;
        jsize szLen = jnieEnv->GetArrayLength(jbaFileContents);
        baResultArray.resize(szLen);
        jnieEnv->GetByteArrayRegion(jbaFileContents, 0, szLen, reinterpret_cast<jbyte*>(baResultArray.data()));
        return baResultArray;
    }
    else
    {
        //Unable to get file contents
        return NULL;
    }
}

//=============================================================================
//=============================================================================
QString
AndroidFileDialog::ResultReceiver::uriToFilename(
    QAndroidJniObject jniURI
    )
{
    //Converts a URI to a filename with supported systems
    QAndroidJniObject jniFileSelector = QAndroidJniObject("org/laird/vsp/FileSelector", "(Landroid/content/Context;)V", QtAndroid::androidContext().object());
    QAndroidJniObject jniFilenameObject = jniFileSelector.callObjectMethod("getFileName", "(Landroid/net/Uri;)Ljava/lang/String;", jniURI.object<jobject>());
    if (jniFilenameObject.isValid())
    {
        //Filename is valid
        QAndroidJniEnvironment jnieEnv;
        jstring strFilenameString = jniFilenameObject.object<jstring>();
        return QString(jnieEnv->GetStringUTFChars(strFilenameString, 0));
    }
    else
    {
        //Unable to get filename
        return NULL;
    }
}

//=============================================================================
//=============================================================================
AndroidFileDialog::AndroidFileDialog(QObject *parent) : QObject(parent)
{
    //Constructor
    receiver = new ResultReceiver(this);
}

//=============================================================================
//=============================================================================
AndroidFileDialog::~AndroidFileDialog(
    )
{
    //Destructor
    delete receiver;
}

//=============================================================================
//=============================================================================
bool
AndroidFileDialog::provideExistingFileName(
    )
{
    //Opens file selection dialogue
    QAndroidJniObject jniACTION_GET_CONTENT = QAndroidJniObject::fromString("android.intent.action.GET_CONTENT");
    QAndroidJniObject jniCATEGORY_OPENABLE = QAndroidJniObject::fromString("android.intent.category.OPENABLE");
    QAndroidJniObject jniIntent("android/content/Intent");

    if (jniACTION_GET_CONTENT.isValid() && jniIntent.isValid())
    {
        //Able to open file selection dialogue
        jniIntent.callObjectMethod("setAction", "(Ljava/lang/String;)Landroid/content/Intent;", jniACTION_GET_CONTENT.object<jstring>());

        //Allow text files and binary files
        jniIntent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString("text/plain").object<jstring>());
        jniIntent.callObjectMethod("putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString("android.intent.extra.MIME_TYPES").object<jstring>(), QAndroidJniObject::fromString("application/octet-stream").object<jstring>());
        if (jniCATEGORY_OPENABLE.isValid())
        {
            //Openable files only
            jniIntent.callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;", jniCATEGORY_OPENABLE.object<jstring>());
        }

        //Show the dialog
        QtAndroid::startActivity(jniIntent.object<jobject>(), nEXISTING_FILE_NAME_REQUEST, receiver);
        return true;
    }
    else
    {
        //Failed to create file selection context
        return false;
    }
}

//=============================================================================
//=============================================================================
void
AndroidFileDialog::emitExistingFileNameReady(
    QString strFilename,
    QByteArray baResult
    )
{
    //Callback for file selection
    emit existingFileNameReady(strFilename, baResult);
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
