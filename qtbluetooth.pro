#-------------------------------------------------
#
# Project created by QtCreator 2015-06-19T07:37:14
#
#-------------------------------------------------

QT       += core gui bluetooth network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qtbluetooth
TEMPLATE = app

SOURCES += \
        main.cpp               \
        mainwindow.cpp         \
        scanselection.cpp      \
        downloader.cpp         \
        settingsdialog.cpp     \
        settingsstorage.cpp    \
        errorlookup.cpp        \
        filetypeselection.cpp  \
        checksumcalculator.cpp

HEADERS  += \
        mainwindow.h           \
        scanselection.h        \
        downloader.h           \
        settingsdialog.h       \
        target.h               \
        settingsstorage.h      \
        errorlookup.h          \
        filetypeselection.h    \
        checksumcalculator.h

FORMS    += \
        mainwindow.ui          \
        scanselection.ui       \
        settingsdialog.ui      \
        filetypeselection.ui

RESOURCES += \
    resources.qrc

android {
    QT += androidextras
    SOURCES += androidfiledialog.cpp
    HEADERS += androidfiledialog.h

    DISTFILES += \
        android/AndroidManifest.xml                       \
        android/gradle/wrapper/gradle-wrapper.jar         \
        android/gradlew                                   \
        android/res/values/libs.xml                       \
        android/build.gradle                              \
        android/gradle/wrapper/gradle-wrapper.properties  \
        android/gradlew.bat

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

    #SSL libraries for Android
    ANDROID_EXTRA_LIBS = $$PWD/SSL_LIBRARIES/$$ANDROID_TARGET_ARCH/libcrypto.so $$PWD/SSL_LIBRARIES/$$ANDROID_TARGET_ARCH/libssl.so
}
