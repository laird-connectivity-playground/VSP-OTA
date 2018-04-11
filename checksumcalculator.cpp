/******************************************************************************
** Copyright (C) 2018 Laird
**
** Project: UwVSP-OTA
**
** Module: checksumcalculator.cpp
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
#include "checksumcalculator.h"

//=============================================================================
//=============================================================================
ChecksumCalculator::ChecksumCalculator(QObject *parent) : QObject(parent)
{
    //Constructor
    ResetChecksum();
}

//=============================================================================
//=============================================================================
ChecksumCalculator::~ChecksumCalculator(
    )
{
    //Destructor
}

//=============================================================================
//=============================================================================
void
ChecksumCalculator::ResetChecksum(
    )
{
    //Resets checksum to default value
    unCrcVal = CHECKSUM_DEFAULT;
}

//=============================================================================
//=============================================================================
quint16
ChecksumCalculator::GetChecksum(
    )
{
    //Returns calculated checksum
    return unCrcVal;
}

//=============================================================================
//=============================================================================
QString
ChecksumCalculator::GetChecksumHexString(
    )
{
    //Returns calculated checksum as a hex-encoded string
    QString strChecksum = QString::number(unCrcVal, 16).toUpper();
    while (strChecksum.length() < 4)
    {
        //Expand to 4 characters
        strChecksum.insert(0, "0");
    }
    return strChecksum;
}

//=============================================================================
//=============================================================================
void
ChecksumCalculator::AddByte(
    unsigned char ucChar
    )
{
    //Calculates checksum of a byte
    quint8 i;
    quint16 usData;

    for(i=0, usData=(quint32)0xff & ucChar++; i < 8; i++, usData >>= 1)
    {
        if ((unCrcVal & 0x0001) ^ (usData & 0x0001))
        {
            unCrcVal = (unCrcVal >> 1) ^ 0x8408;
        }
        else
        {
            unCrcVal >>= 1;
        }
    }
    unCrcVal = ~unCrcVal;
    usData = unCrcVal;
    unCrcVal = (unCrcVal << 8) | (usData >> 8 & 0xff);
}

/******************************************************************************/
// END OF FILE
/******************************************************************************/
