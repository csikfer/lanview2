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
#if defined(Q_CC_GNU)
#include <execinfo.h>
#elif 0 && defined(Q_CC_MSVC)
#include <Windows.h>
#include <WinBase.h>
#include <DbgHelp.h>
#endif

cBackTrace::cBackTrace(size_t _size)
{
#if defined(Q_CC_GNU)
    int     size;
    char ** symbols;
    void * buffer[_size];
    size   = backtrace(buffer, int(_size));
    symbols= backtrace_symbols(buffer, size);
    if (symbols == nullptr) {
        *this << QObject::tr("backtrace_symbols(...) failed.");
    }
    else {
        for (int i = 2; i < size; ++i) {    // cBackTrace, és cError nem kell.
            *this << QString(symbols[i]);
        }
        free(symbols);
    }
#elif 0 && defined(Q_CC_MSVC)
    unsigned short frames;
    SYMBOL_INFO    symbol;
    HANDLE         process;
    void * buffer[_size];
    process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE );
    frames = CaptureStackBackTrace( 0, (DWORD)_size, buffer, NULL );
    for(int i = 2; i < frames; i++) {
        SymFromAddr( process, ( DWORD64 )( buffer[ i ] ), 0, &symbol );
        *this << QString(symbol.Name);
    }
    SymCleanup(process);
#else
    (void)_size;
#endif
}

class lanView;
extern qlonglong sendError(const cError *pe, lanView *_instance = nullptr);

QList<cError *> cError::errorList;
int         cError::mMaxErrCount= 6;
bool        cError::mDropAll = false;
QMap<int, QString>  cError::mErrorStringMap;

cError::cError()
    : mFuncName(), mSrcName(), mErrorSubMsg(), mThreadName()
    , mSqlErrDrText(), mSqlErrDbText(), mSqlQuery(), mSqlBounds()
{
    mSrcLine = mErrorSubCode = mErrorCode = -1;
    mErrorSysCode = errno;
    mSqlErrType   = QSqlError::NoError;
    mDataLine     = -1;
    mDataPos      = -1;
    circulation();
}

cError::cError(const char * _mSrcName, int _mSrcLine, const char * _mFuncName, int _mErrorCode,
           qlonglong _mErrorSubCode, const QString& _mErrorSubMsg)
    : mFuncName(), mSrcName(), mErrorSubMsg(), mThreadName()
    , mSqlErrDrText(), mSqlErrDbText(), mSqlQuery(), mSqlBounds()
    , mDataMsg(), mDataName()
{
    if (mDropAll) {
//      delete this;
        throw &_no_init_;
    }
    mFuncName     = QString::fromUtf8(_mFuncName);
    mSrcName      = QString::fromUtf8(_mSrcName);
    mErrorSysCode = errno;
    mSrcLine      = _mSrcLine;
    mErrorCode    = _mErrorCode;
    mErrorSubCode = _mErrorSubCode;
    mErrorSubMsg  = _mErrorSubMsg;
    mSqlErrType   = QSqlError::NoError;
    mDataLine     = -1;
    mDataPos      = -1;
    slBackTrace   = cBackTrace();
    circulation();
}

cError::cError(const QString& _mSrcName, int _mSrcLine, const QString& _mFuncName, int _mErrorCode,
           qlonglong _mErrorSubCode, const QString& _mErrorSubMsg)
    : mFuncName(_mFuncName), mSrcName(_mSrcName), mErrorSubMsg(), mThreadName()
    , mSqlErrDrText(), mSqlErrDbText(), mSqlQuery(), mSqlBounds()
    , mDataMsg(), mDataName()
{
    if (mDropAll) {
//      delete this;
        throw &_no_init_;
    }
    mErrorSysCode = errno;
    mSrcLine      = _mSrcLine;
    mErrorCode    = _mErrorCode;
    mErrorSubCode = _mErrorSubCode;
    mErrorSubMsg  = _mErrorSubMsg;
    mSqlErrType   = QSqlError::NoError;
    mDataLine     = -1;
    mDataPos      = -1;
    slBackTrace   = cBackTrace();
    circulation();
}

cError::~cError()
{
    int i = errorList.removeAll(this);
    if (i != 1) {
        DERR() << "Invalid error object ..." << endl;
    }
}

void cError::circulation()
{
    pThread = QThread::currentThread();
    errorList << this;
    if (pThread == QCoreApplication::instance()->thread()) {
        mThreadName = _sMainThread;
    }
    else {
        mThreadName = pThread->objectName();
    }

    if (errCount() > mMaxErrCount) {
        QTextStream cerr(stderr, QIODevice::WriteOnly);
        cerr << QObject::tr("*** Error circulation **** Thread object name %1").arg(mThreadName) << endl;
        int n = 1;
        foreach (cError * pe, errorList) {
            qlonglong eid = sendError(pe);
            QString em = eid == NULL_ID ? QObject::tr("\n --- A hiba rekord kiírása sikertelen.") : QObject::tr("\n --- Kiírt hiba rekord id : %1").arg(eid);
            cerr << "[#" << n++ << QChar(']') << QChar(' ') << pe->msg() << em << endl;
            ++n;
        }
        exit(-1);
    }
}

void cError::exception()
{
    if (mDropAll) {
//      delete this;
        throw &_no_init_;
    }
    QString m = QObject::tr("throw this : %1").arg(msg());
    if (cDebug::getInstance() != nullptr) {
        PDEB(EXCEPT) << m << endl;
        cDebug::flushAll();
    }
#if LV2_THROW
    throw(this);
#else
    {
        QString mm = QObject::tr("Exception (throw) is not working, exit.");
        if (cDebug::getInstance() != nullptr) {
            if (!ONDB(EXCEPT)) cDebug::cout() << m << endl;
            cDebug::cout() << mm << endl;
        }
        else {
            std::cout << m.toStdString() << std::endl << mm.toStdString() << std::endl;
        }
        sendError(this);
        exit(mErrorCode);
    }
#endif
}

QString cError::errorMsg(int __ErrorCode)
{
    QMap<int, QString>::const_iterator i = mErrorStringMap.find(__ErrorCode);
    if (i != mErrorStringMap.end()) {
        return *i;
    }
    return (QObject::tr("Ismeretlen hiba kód"));
}

cError& cError::nested(const char * _mSrcName, int _mSrcLine, const char * _mFuncName)
{
    mErrorSubMsg    = msg();
    mFuncName       = _mFuncName;
    mSrcName        = _mSrcName;
    mSrcLine        = _mSrcLine;
    mErrorSubCode   = mErrorCode;          ///< Error sub code
    mErrorCode      = eError::ENESTED;
    pThread         = QThread::currentThread();
    if (pThread == QCoreApplication::instance()->thread()) {
        mThreadName = _sMainThread;
    }
    else {
        mThreadName = pThread->objectName();
    }
    return *this;
}

QString cError::msg() const
{
    QString r;
    if (!mThreadName.isEmpty()) r = QChar('{') + mThreadName + QChar('}');
    r += QString("%1[%2]:%3: %4 #%5(\"%6\" #%7)")
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
            if (!mSqlErrCode.isEmpty()) r +=  mSqlErrCode;
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
    if (!slBackTrace.isEmpty()) {
        r += QString("\n\nBackTrace:\n") +  slBackTrace.join("\n");
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

bool cError::errStat()
{
    bool r = false;
    foreach (cError *pe, errorList) {
        r = pe->mErrorCode != eError::EOK;
        if (r) break;
    }
    return r;
}

void cErrorException(const QString& _mSrcName, int _mSrcLine, const QString& _mFuncName, int _mErrorCode, qlonglong _mErrorSubCode, const QString& _mErrorSubMsg)
{
    auto *pe = new cError(_mSrcName, _mSrcLine, _mFuncName, _mErrorCode, _mErrorSubCode, _mErrorSubMsg);
    pe->exception();
}

QString SqlErrorTypeToString(int __et)
{
    switch (__et) {
    case QSqlError::NoError:            return QObject::tr("No error occurred.");
    case QSqlError::ConnectionError:    return QObject::tr("Connection error.");
    case QSqlError::StatementError:     return QObject::tr("SQL statement syntax error.");
    case QSqlError::TransactionError:   return QObject::tr("Transaction failed error.");
    case QSqlError::UnknownError:       return QObject::tr("Unknown error");
    }
    return QObject::tr("Unknown SQL Error type.");
}

QString _sql_err_bound(QSqlQuery& q, const QString& pref)
{
    if (q.boundValues().isEmpty()) {
        return QString();
    }
    QMapIterator<QString, QVariant> i(q.boundValues());
    QString r;
    while (i.hasNext()) {
        i.next();
        r += i.key() + " = " + debVariantToString(i.value()) + "; ";
    }
    r.chop(2);
    return  pref + r;
}

void _sql_err_deb_(const QSqlError& le, const char * _fi, int _li, const char * _fu, const QString& s)
{
    if (ONDB(DERROR)) {
        cDebug::cout() <<  __DERRH(_fi, _li, _fu) << " ...\n";
        cDebug::cout() << QObject::tr("SQL ERROR ") << le.nativeErrorCode()
                       << QObject::tr("; type:") << SqlErrorTypeToString(le.type()) << "\n";
        cDebug::cout() << QObject::tr("driverText   : ") << le.driverText() << "\n";
        cDebug::cout() << QObject::tr("databaseText : ") << le.databaseText();
        if (s.size() > 0) cDebug::cout() << QObject::tr("SQL string   : ") << s << "\n";
        cDebug::cout() << flush;
    }
}

void _sql_derr_deb_(QSqlQuery& q, const char * _fi, int _li, const char * _fu, const QString& s)
{
    if (ONDB(DERROR)) {
        cDebug::cout() <<  __DERRH(_fi, _li, _fu) << " " << s << "\n";
        cDebug::cout() << QObject::tr("SQL string : ") << q.lastQuery();
        cDebug::cout() << _sql_err_bound(q, QObject::tr("\nSQL bounds : "));
        cDebug::cout() << flush;
    }
}

void _sql_err_ex(cError *pe, const QSqlError& le, const QString& sql, const QString& bound)
{
    pe->mSqlErrCode   = le.nativeErrorCode();
    pe->mSqlErrType   = le.type();
    pe->mSqlErrDrText = le.driverText();
    pe->mSqlErrDbText = le.databaseText();
    pe->mSqlQuery     = sql;
    pe->mSqlBounds    = bound;
    pe->exception();
}

void _sql_derr_ex(cError *pe, QSqlQuery& q)
{
    pe->mSqlQuery     = q.lastQuery();
    pe->mSqlBounds    = _sql_err_bound(q);
    pe->exception();
}
