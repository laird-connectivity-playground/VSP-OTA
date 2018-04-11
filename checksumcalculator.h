/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: checksumcalculator.h
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
#ifndef CHECKSUMCALCULATOR_H
#define CHECKSUMCALCULATOR_H

/******************************************************************************/
// Include Files
/******************************************************************************/
#include <QObject>
#include "target.h"

/******************************************************************************/
// Constants
/******************************************************************************/
const quint16 CHECKSUM_DEFAULT = 0xFFFF;

/******************************************************************************/
// Class definitions
/******************************************************************************/
class ChecksumCalculator : public QObject
{
    Q_OBJECT
public:
    explicit
    ChecksumCalculator(
        QObject *parent = nullptr
        );
    ~ChecksumCalculator(
        );
    void
    ResetChecksum(
        );
    quint16
    GetChecksum(
        );
    QString
    GetChecksumHexString(
        );
    void
    AddByte(
        unsigned char ucChar
        );

private:
    quint16 unCrcVal;
};

#endif // CHECKSUMCALCULATOR_H

/******************************************************************************/
// END OF FILE
/******************************************************************************/
