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
#include <cstdarg>
#include <sys/types.h>
#include <cstdio>


#include <QtCore>

#if defined (Q_OS_WIN)
#include <io.h>
#else
#include <unistd.h>
#endif

#include "cdebug.h"
#include "cerror.h"
// #include "lv2types.h"

QString quotedString(const QString& __s, const QChar& __q)
{
    if (__s.isNull()) return "NULL";
    QString r = __q;
    foreach (QChar c, __s) {
        if (c.isLetterOrNumber())   r += c;
        else if (c == __q)          r += QString("\\") + c;
        else if (c.isNull())        r += "\\0";
        else {
            char cc = c.toLatin1();
            switch (cc) {
            case 0:     r += "\\?"; break;
            case '\n':  r += "\\n"; break;
            case '\r':  r += "\\r"; break;
            case '\t':  r += "\\t"; break;
            case '\\':  r += "\\\\"; break;
            default:
                if (isprint(cc)) r += cc;
                else {
                    constexpr int octal = 8;
                    r += "\\" + QString::number(int(cc), octal);
                }
            }
        }
    }
    return r + __q;
}

QString quotedStringList(const QStringList& __sl, const QChar &__q , const QChar &__s)
{
    QString r;
    foreach (QString s, __sl) {
        r += quotedString(s, __q) + __s;
    }
    r.chop(1);
    return r;
}

QString quotedVariantList(const QVariantList& __sl, const QChar &__q, const QChar &__s)
{
    QString r;
    foreach (QVariant v, __sl) {
        r += quotedString(v.toString(), __q) + __s;
    }
    r.chop(1);
    return r;
}

cDebug *cDebug::instance = nullptr;
bool    cDebug::disabled = false;

void cDebug::init(const QString& fn, qlonglong mask)
{
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
    cDebug::instance = new cDebug(mask, fn);
}

cDebug::cDebug() : mFName(fNameCnv(QString()))
{
    mCout                = nullptr;
    mFile                = nullptr;
    mMsgQueue            = nullptr;
    mMsgQueueMutex       = nullptr;
    mThreadStreamsMap    = nullptr;
    mThreadStreamsMapMutex= nullptr;
    mThreadMsgQueue      = nullptr;
    mThreadMsgQueueMutex = nullptr;

    mMask = INFO | DERROR | WARNING | MODMASK;
    mCout = new debugStream(stderr);
    if (QTextStream::Ok != mCout->stream.status()) EXCEPTION(EFOPEN, -1, _sStdErr);
    disabled = false;
}


cDebug::cDebug(qlonglong _mMask, const QString& _fn) : mFName(_fn)
{
    mCout                = nullptr;
    mFile                = nullptr;
    mMsgQueue            = nullptr;
    mMsgQueueMutex       = nullptr;
    mThreadStreamsMap    = nullptr;
    mThreadStreamsMapMutex= nullptr;
    mThreadMsgQueue      = nullptr;
    mThreadMsgQueueMutex = nullptr;
    mMaxLogSize          = 0;
    mArcNum              = 0;

    mMask = _mMask;
#if defined (Q_OS_WIN)
    if      (mFName == _sStdOut || mFName == _sStdErr) {
        mCout = new debugStream(stdout);
    }
#else
    if      (mFName == _sStdOut) {
        mCout = new debugStream(stdout);
    }
    else if (mFName == _sStdErr) {
        mCout = new debugStream(stderr);
    }
#endif
    else {
        mFile = new QFile(mFName);
        if (! mFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            delete mFile;
            mFile = nullptr;
            EXCEPTION(EFOPEN, 0, _fn);
        }
        constexpr qlonglong defaultMaxLogSize = 1024 * 1024 * 128;  // 128 MiByte
        constexpr int       defaultArcLogNumb = 8;
        mMaxLogSize = defaultMaxLogSize;
        mArcNum     = defaultArcLogNumb;
        mCout = new debugStream(mFile);
    }
    if (QTextStream::Ok != mCout->stream.status()) EXCEPTION(EFOPEN, -1, mFName);
    disabled = false;
}

cDebug::~cDebug()
{
    flushAll();
    disabled = true;
    constexpr int oneSecTO = 1000;
    if (mThreadMsgQueueMutex != nullptr) {
        mThreadMsgQueueMutex->tryLock(oneSecTO);
        delete mThreadMsgQueueMutex;
        mThreadMsgQueueMutex = nullptr;
    }
    if (mThreadMsgQueue != nullptr) {
        delete mThreadMsgQueue;
    }
    if (mThreadStreamsMapMutex != nullptr) {
        mThreadStreamsMapMutex->tryLock(oneSecTO);
        delete mThreadStreamsMapMutex;
        mThreadStreamsMapMutex = nullptr;
    }
    if (mThreadStreamsMap != nullptr) {
        debugStream *pS;
        foreach(pS, *mThreadStreamsMap) {
            mCout->disconnect(mCout, SLOT(sRedyLineFromThread()));
            delete pS;
            pS = nullptr;
        }
        delete mThreadStreamsMap;
    }
    if (this == instance) {
        if (mMask & INFO) *mCout << QObject::tr("Remove debug object.") << endl;
        instance = nullptr;
    }
    else {
        *mCout << QObject::tr("Remove debug object, this not equal instance.") << VDEBPTR(instance) << QChar(',') << VDEBPTR(this) << endl;
    }
    if (mMsgQueue) {
        delete mMsgQueue;
    }
    if (mCout) {
        delete mCout;
    }
    if (mFile) {
        delete mFile;
    }
}

bool cDebug::pDeb(qlonglong mask)
{
    if (!__pDeb(mask)) return false;
    cout().lastMask = mask;
    return true;
}

bool cDebug::__pDeb(qlonglong mask)
{
    if (disabled) {
        return false;
    }
    if (instance == nullptr) {
        instance = new cDebug();
    }
    if (mask & DERROR) return true;
    qlonglong mm = instance->mMask & mask;
    bool       r = mask && (mm) == mask;
    return r;
}


debugStream *cDebug::pCout()
{
    // printf("cDebug::cout(void)\n");
    if (nullptr == instance || nullptr == instance->mCout) EXCEPTION(EPROGFAIL);
    return instance->mCout;
}

debugStream& cDebug::cout()
{
    debugStream *pS;
    if (nullptr == instance || nullptr == instance->mCout) EXCEPTION(EPROGFAIL);
    if (isMainThread()) return *pCout();
    const QString&  n = currentThreadName();
    if (instance->mThreadStreamsMap == nullptr) {
        instance->mThreadStreamsMapMutex = new QMutex();
        instance->mThreadStreamsMap = new QMap<QString, debugStream *>();
    }
    else if (instance->mThreadStreamsMap->contains(n)) {
        QMutexLocker locker(instance->mThreadStreamsMapMutex);
        pS = (*instance->mThreadStreamsMap)[n];
        return *pS;
    }
    // Létre kell hozni a Thread-hez a debug stream-et
    QMutexLocker locker(instance->mThreadStreamsMapMutex);
    pS = new debugStream(instance->mCout, QThread::currentThread());
    (*instance->mThreadStreamsMap)[n] = pS;
    if (cDebug::__pDeb(cDebug::INFO | cDebug::LV2)) *pS << QObject::tr("Create debugStream to %1 thread..").arg(n) << endl;
    return *pS;
}


QString cDebug::maskName(qlonglong __msk)
{
    QString n;
    if (__msk & cDebug::LV2)        n += _sLV2        + QChar(',');
    if (__msk & cDebug::LV2G)       n += _sLV2G       + QChar(',');
    if (__msk & cDebug::PARSER)     n += _sPARSER     + QChar(',');
    if (__msk & cDebug::APP)        n += _sAPP        + QChar(',');
    if (__msk & cDebug::EXCEPT)     n += _sEXCEPT     + QChar(',');
    if (__msk & cDebug::DERROR)     n += _sERROR      + QChar(',');
    if (__msk & cDebug::WARNING)    n += _sWARNING    + QChar(',');
    if (__msk & cDebug::INFO)       n += _sINFO       + QChar(',');
    if (__msk & cDebug::VERBOSE)    n += _sVERBOSE    + QChar(',');
    if (__msk & cDebug::VVERBOSE)   n += _sVVERBOSE   + QChar(',');
    if (__msk & cDebug::ENTERLEAVE) n += _sENTERLEAVE + QChar(',');
    if (__msk & cDebug::OBJECT)     n += _sOBJECT     + QChar(',');
    if (__msk & cDebug::PARSEARG)   n += _sPARSEARG   + QChar(',');
    if (__msk & cDebug::SQL)        n += _sSQL        + QChar(',');
    if (__msk & cDebug::OBJECT)     n += _sOBJECT     + QChar(',');
    if (__msk & cDebug::ADDRESS)    n += _sADDRESS    + QChar(',');
    if (n.size()) n.chop(1);
    return QChar('[') + n + QChar(']') + QChar(' ');
}

void cDebug::setGui(bool __f)
{
    if (instance == nullptr) EXCEPTION(EPROGFAIL);
    if (__f) {  // Set GUI mode
        if (instance->mMsgQueue == nullptr) {
            instance->mMsgQueue      = new QQueue<QString>();
            instance->mMsgQueueMutex = new QMutex();
        }
    }
    else {      // Unset Gui mode.
        if (instance->mMsgQueue != nullptr) {
            delete instance->mMsgQueue;
            delete instance->mMsgQueueMutex;
            instance->mMsgQueue      = nullptr;
            instance->mMsgQueueMutex = nullptr;
        }
    }
}

QString cDebug::dequeue()
{
    if (instance == nullptr)                   EXCEPTION(EPROGFAIL, -1, QObject::tr("cDebug::instance is NULL pointer"));
    if (instance->mMsgQueue == nullptr)        EXCEPTION(EPROGFAIL, -1, QObject::tr("cDebug::instance->mMsgQueue is NULL pointer"));
    if (instance->mMsgQueueMutex == nullptr)   EXCEPTION(EPROGFAIL, -1, QObject::tr("cDebug::instance->mMsgQueueMutex is NULL pointer"));
    QMutexLocker locker(instance->mMsgQueueMutex);
    if (instance->mMsgQueue->isEmpty())     return QObject::tr("***** *cDebug::instance->mMsgQueue is empty *****");
    QString r = instance->mMsgQueue->dequeue();
    return r;
}

void cDebug::chk()
{
    if (instance == nullptr) EXCEPTION(EPROGFAIL);
}

QString cDebug::fNameCnv(const QString& _fn)
{
    if   (_fn == QChar('-') || _fn.toLower() == _sStdOut) {
        return _sStdOut;
    }
    if (_fn.isEmpty()  || _fn.toLower() == _sStdErr) {
        return _sStdErr;
    }
    return _fn;
}

void cDebug::flushAll()
{
    if (cDebug::disabled
     || !isMainThread()
     || instance == nullptr
     || instance->mThreadMsgQueueMutex == nullptr
     || instance->mThreadMsgQueue == nullptr
     || instance->mThreadMsgQueue->isEmpty()
     || nullptr == instance->mCout)
        return;
    *instance->mCout << QObject::tr("Flush thread messages :") << endl;
    instance->mThreadMsgQueueMutex->lock();
    foreach (QString msg, *instance->mThreadMsgQueue) {
        *instance->mCout << msg;
    }
    instance->mThreadMsgQueueMutex->unlock();
    *instance->mCout << QObject::tr("Flush thread messages end.") << endl;
}

/* **************************************************************************************** */
debugStream *debugStream::mainInstance = nullptr;

debugStream::debugStream(debugStream *pMain, QObject *pThread)
    : QObject(), stream(), buff()
{
    pFILE = nullptr;
    oFile = nullptr;
    stream.setString(&buff, QIODevice::WriteOnly);
    if (!connect(this,    SIGNAL(redyLineFromThread()), pMain, SLOT(sRedyLineFromThread()), Qt::QueuedConnection)
     || !connect(pThread, SIGNAL(destroyed(QObject*)),  this,  SLOT(sDestroyThread(QObject*)))) {
        EXCEPTION(EPROGFAIL);
    }
}

debugStream::debugStream(QFile *__f)
    : QObject(), stream(), buff()
{
    pFILE = nullptr;
    oFile = __f;
    stream.setString(&buff, QIODevice::WriteOnly);
    mainInstance = this;
}

debugStream::debugStream(FILE *__f)
    : QObject(), stream(), buff()
{
    pFILE = __f;
    oFile = new QFile();
    oFile->open(__f, QIODevice::WriteOnly);
    stream.setString(&buff, QIODevice::WriteOnly);
    mainInstance = this;
}

debugStream::~debugStream()
{
    // printf("Remove debugStream: %p...\n", this);
    PDEB(INFO) << QObject::tr("Remove debugStream: %1...").arg(qulonglong(this)) << endl;
    flush();
    stream.reset();
    if (isMain()) {
        if (pFILE != nullptr) {    // A konstruktor allokálta az oFile objektumot
            delete oFile;
        }
        mainInstance = nullptr;
    }
    else {
        cDebug *pD = cDebug::instance;
        // Sanity check
        if (pD                         == nullptr) EXCEPTION(EPROGFAIL, -1, tr("cDebug::instance is NULL"));
        if (pD->mThreadStreamsMapMutex == nullptr) EXCEPTION(EPROGFAIL, -1, tr("mThreadStreamsMapMutex is NULL"));
        if (pD->mThreadStreamsMap      == nullptr) EXCEPTION(EPROGFAIL, -1, tr("mThreadStreamsMap is NULL"));
        if (parent()                   == nullptr) EXCEPTION(EPROGFAIL, -1, tr("Parent is NULL"));
        // Delete debugStream from mThreadStreamsMap
        QMutexLocker locker(pD->mThreadStreamsMapMutex);
        QMap<QString, debugStream *>::iterator i = pD->mThreadStreamsMap->find(parent()->objectName());
        if (i == pD->mThreadStreamsMap->end()) EXCEPTION(EPROGFAIL, -1, tr("mThreadStreamsMap is corrupt, thread name not found."));
        if (i.value() != this)                 EXCEPTION(EPROGFAIL, -1, tr("mThreadStreamsMap is corrupt, or thread name is not unique."));
        pD->mThreadStreamsMap->erase(i);
    }
}

void debugStream::sDestroyThread(QObject*)
{
    delete this;
}

#define __DEBUG__QUEUE 0

debugStream& debugStream::flush()
{
    if (cDebug::disabled || buff.isEmpty()) return *this;
    cDebug *pD = cDebug::instance;
    if (pD == nullptr) EXCEPTION(EPROGFAIL, -1, QObject::tr("cDebug::instance is NULL pointer"));
    if (isMain()) {
        // Main thread
        if (pD->mMaxLogSize) writeRollLog(*oFile, buff.toUtf8(), pD->mMaxLogSize, pD->mArcNum);
        else                 oFile->write(buff.toUtf8());
        if (pD->mMsgQueue != nullptr) {    // GUI?
            if (pD->mMsgQueueMutex == nullptr) EXCEPTION(EPROGFAIL, -1, QObject::tr("cDebug::instance->mMsgQueueMutex is NULL pointer"));
#if __DEBUG__QUEUE
            QTextStream(stderr) << "Queued to GUI : " << quotedString(buff) << endl;
#endif
            pD->mMsgQueueMutex->lock();
            pD->mMsgQueue->enqueue(buff);
            pD->mMsgQueueMutex->unlock();
            readyDebugLine();
#if __DEBUG__QUEUE
            QTextStream(stderr) << "Queued to GUI, ready." << endl;
#endif
        }
        buff.clear();
        oFile->flush();
    }
    else {
        // Thread
        if (pD->mThreadMsgQueueMutex == nullptr) {
            pD->mThreadMsgQueueMutex = new QMutex();
            pD->mThreadMsgQueue = new QQueue<QString>();
        }
#if __DEBUG__QUEUE
        QTextStream(stderr) << "Queued from thread : " << quotedString(buff) << endl;
#endif
        pD->mThreadMsgQueueMutex->lock();
        pD->mThreadMsgQueue->enqueue(buff);
        pD->mThreadMsgQueueMutex->unlock();
#if __DEBUG__QUEUE
        QTextStream(stderr) << "Queued from thread, ready." << endl;
#endif
        buff.clear();
        emit redyLineFromThread();
    }
    return *this;
}

void debugStream::sRedyLineFromThread()
{
    if (cDebug::disabled) return;
    if (isMainThread() == false) {
        QTextStream(stderr) << QObject::tr("ERROR void debugStream::sRedyLineFromThread() : This is not main thread.") << endl;
        return;
    }
    QString m;
    cDebug *pD = cDebug::instance;
    pD->mThreadMsgQueueMutex->lock();
    m = pD->mThreadMsgQueue->dequeue();
    pD->mThreadMsgQueueMutex->unlock();
    (*this) << m;
    flush();
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
#define HEAD_DT_FORMAT Qt::ISODateWithMs
#else
#define HEAD_DT_FORMAT "yyyy-MM-ddTHH:mm:ss.zzz"
#endif

debugStream &  head(debugStream & __ds)
{
    __ds << QString("%1 ").arg(__ds.lastMask, 8, 16)    // Az utolsó (aktuális) maszk hexában
         << QDateTime::currentDateTime().toString(HEAD_DT_FORMAT) << QChar(' ')
         << QCoreApplication::applicationName()
         << QChar('[')  << QString::number(QCoreApplication::applicationPid());
    if (__ds.isMain() == false) {
        __ds << QChar(',') << QThread::currentThread()->objectName();
    }
    __ds << QString("] ");
    return __ds;
}

QString list2string(const QVariantList& __vl)
{
    QString r = QChar('{');
    foreach (QVariant v, __vl) {
        if (r.size() > 1) r += _sCommaSp;
        switch (v.type()) {
        case QVariant::ByteArray:
        case QVariant::String:      r += quotedString(v.toString());    break;
        case QVariant::StringList:  r += list2string(v.toStringList()); break;
        case QVariant::List:        r += list2string(v.toList());       break;
        case QVariant::BitArray:    r += list2string(v.toBitArray());   break;
        default:                    r += v.toString() + QChar('/') + v.typeName(); break;
        }
    }
    return r + QChar('}');
}

QString list2string(const QBitArray& __vl)
{
    QString r = QChar('[');
    for (int i = 0; i < __vl.size(); ++i) {
        r += (__vl[i]) ? "1" : "0";
    }
    return r + QChar(']');
}

QString list2string(const QStringList& __vl)
{
    QString r = QChar('{');
    foreach (QString v, __vl) {
        if (r.size() > 1) r += _sCommaSp;
        r += quotedString(v);
    }
    return r + QChar('}');
}

// lv2types.h -ban
EXT_ QString QVariantToString(const QVariant& _v, bool *pOk);

QString debVariantToString(const QVariant& v)
{
    if (v.isNull()) return "[NULL]";
    if (!v.isValid()) return "[Invalid]";
    const char * tn = v.typeName();
    bool ok;
    QString s = QVariantToString(v, &ok);
    if (!ok) {
        s = "[?]";
        switch (v.userType()) {
        case QMetaType::QVariantMap:
          {
            QVariantMap vm = v.value<QVariantMap>();
            s = " { " ;
            foreach (QString key, vm.keys()) {
                s += quotedString(key) + " = " + debVariantToString(vm[key]) + ", ";
            }
            s.chop(2);
            s += " }";
            break;
          }
        case QMetaType::QVariantList:
          {
            QVariantList vl = v.value<QVariantList>();
            s = " [ " ;
            foreach (QVariant v, vl) {
                s += debVariantToString(v) + ", ";
            }
            s.chop(2);
            s += " ]";
            break;
          }
        }
    }
    return quotedString(s) + "::" + (tn == nullptr ? _sNULL : QString(tn));
}

