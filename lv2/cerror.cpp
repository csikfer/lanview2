/***************************************************************************
 *   Copyright (C) 2005 by Csiki Ferenc                                    *
 *   csikfer@csikferpc                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <QtCore>
#include "cdebug.h"
#include "cerror.h"
#include <iostream>

cError  *cError::pLastError = NULL;
QThread *cError::pLastThread = NULL;
int      cError::mErrCount   = 0;
int      cError::mMaxErrCount= 4;
bool     cError::mDropAll = false;
QMap<int, QString>  cError::mErrorStringMap;

cError::cError()
    : mFuncName(), mSrcName(), mErrorSubMsg(), mThreadName()
    , mSqlErrDrText(), mSqlErrDbText(), mSqlQuery(), mSqlBounds()
{
    mSrcLine = mErrorSubCode = mErrorCode = -1;
    mErrorSysCode = errno;
    mSqlErrNum    = -1;
    mSqlErrType   = QSqlError::NoError;
    mDataLine     = -1;
    mDataPos      = -1;
    circulation();
}

cError::cError(const char * _mSrcName, int _mSrcLine, const char * _mFuncName, int _mErrorCode,
           int _mErrorSubCode, const QString& _mErrorSubMsg)
    : mFuncName(_mFuncName), mSrcName(_mSrcName), mErrorSubMsg(), mThreadName()
    , mSqlErrDrText(), mSqlErrDbText(), mSqlQuery(), mSqlBounds()
    , mDataMsg(), mDataName()
{
    if (mDropAll) {
//      delete this;
        throw _no_init_;
    }
    mErrorSysCode = errno;
    mSrcLine      = _mSrcLine;
    mErrorCode    = _mErrorCode;
    mErrorSubCode = _mErrorSubCode;
    mErrorSubMsg  = _mErrorSubMsg;
    mSqlErrNum    = -1;
    mSqlErrType   = QSqlError::NoError;
    mDataLine     = -1;
    mDataPos      = -1;
    circulation();
}

cError::~cError()
{
    --mErrCount;
    if (pLastError == this) pLastError = NULL;
}

void cError::circulation()
{
    pPrevError = pLastError;
    pLastError = this;
    QThread *pThread = QThread::currentThread();
    if (pThread == QCoreApplication::instance()->thread()) {
        mThreadName = _sMainThread;
    }
    else {
        mThreadName = pThread->objectName();
    }
    if (pPrevError && (pThread == pLastThread || ++mErrCount > mMaxErrCount)) {
        QTextStream cerr(stderr, QIODevice::WriteOnly);
        cerr << QObject::trUtf8("*** Error circulation **** Thread object name %1").arg(pThread->objectName()) << endl;
        int n = 1;
        for (cError *p = this; p; p = p->pPrevError) {
            cerr << QChar('[') << n++ << QChar(']') << QChar(' ') << p->msg() << endl;
        }
        exit(-1);
    }
    pLastThread = pThread;
}

void cError::exception(void)
{
    QString m = QObject::trUtf8("throw this : %1").arg(msg());
    if (cDebug::getInstance() != NULL) PDEB(EXCEPT) << m << endl;
    throw(this);
    {
        QString mm = QObject::trUtf8("Exception (throw) is not working, exit.");
        if (cDebug::getInstance() != NULL) {
            if (!ONDB(EXCEPT)) cDebug::cout() << m << endl;
            cDebug::cout() << mm << endl;
        }
        else {
            std::cout << m.toStdString() << std::endl << mm.toStdString() << std::endl;
        }
        exit(mErrorCode);
    }
}

QString cError::errorMsg(int __ErrorCode)
{
    QMap<int, QString>::const_iterator i = mErrorStringMap.find(__ErrorCode);
    if (i != mErrorStringMap.end()) {
        return *i;
    }
    return (QObject::trUtf8("Ismeretlen hiba kód"));
}

QString cError::msg(void) const
{
    QString r;
    if (!mThreadName.isEmpty()) r = QChar('{') + mThreadName + QChar('}');
    r += QString("%1[%2]:%3: %4 #%5(\"%6\" #%7")
        .arg(mSrcName)
        .arg(mSrcLine)
        .arg(mFuncName)
        .arg(errorMsg())
        .arg(mErrorCode)
        .arg(mErrorSubMsg)
        .arg(mErrorSubCode);
    if (mErrorCode != eError::EOK) {
        if (mErrorSysCode != 0) {
            r += QString(" / errno = %1, %2)")
                .arg(mErrorSysCode)
                .arg(errorSysMsg());
        }
        if (mSqlErrType != QSqlError::NoError) {
            r += "\nSQL ERROR ";
            if ( mSqlErrNum != -1) r +=  QChar('#') + QString::number(mSqlErrNum);
            r += " : ";
            r += "; type:" + SqlErrorTypeToString(mSqlErrType) + "\n";
            r += "driverText   : " + mSqlErrDrText + "\n";
            r += "databaseText : " + mSqlErrDbText + "\n";
            if (!mSqlQuery.isEmpty()) {
                r += "SQL string   : " + mSqlQuery + "\n";
                if (!mSqlBounds.isEmpty()) {
                     r += "SQL bounds   : " + mSqlBounds + "\n";
                }
            }
        }
        if (mDataLine >= 0) {
            r += QString("DATA : %1[%2] :\n").arg(mDataName).arg(mDataLine) + mDataMsg;
        }
    }
    return r;
}

void cError::addErrorString(int __ErrorCode, const QString& __ErrorString)
{
    QMap<int, QString>::const_iterator i = mErrorStringMap.find(__ErrorCode);
    if (i == mErrorStringMap.end()) {
        mErrorStringMap.insert(__ErrorCode, __ErrorString);
        return;
    }
    QString prev = *i;
    EXCEPTION(REDEFERRSTR, __ErrorCode, QString("\"%1\" to \"%2\"").arg(prev, __ErrorString));
}

void cError::init()
{
#define ERCODES_DEFINE
#include "errcodes.h"
}

QString SqlErrorTypeToString(int __et)
{
    switch (__et) {
    case QSqlError::NoError:            return QObject::trUtf8("No error occurred.");
    case QSqlError::ConnectionError:    return QObject::trUtf8("Connection error.");
    case QSqlError::StatementError:     return QObject::trUtf8("SQL statement syntax error.");
    case QSqlError::TransactionError:   return QObject::trUtf8("Transaction failed error.");
    case QSqlError::UnknownError:       return QObject::trUtf8("Unknown error");
    }
    return QObject::trUtf8("Unknown SQL Error type.");
}
