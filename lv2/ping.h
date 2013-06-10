#ifndef __PING_HPP__
#define __PING_HPP__

#include "lv2_global.h"
#include <QtNetwork>
#include <QString>

/// Ping hiba kódok.
enum PingResult
{
    PING_SUCCESS,   ///< Volt válasz
    PING_ERROR,     ///< hiba történt
    PING_SILENCE    ///< nincs válasz
};


// --------------------------------------------------------------------------------
//
// This class operates in the 3-rd (Network) level of OSI
//
// --------------------------------------------------------------------------------

class  LV2SHARED_EXPORT Pinger
{
public:
    Pinger() { mElapsed = mBytes = -1; }
    PingResult ping(const char* address, unsigned time_limit = 0);
    PingResult ping(const QString& address, unsigned time_limit = 0)
    {
        return ping(address.toStdString().c_str(), time_limit);
    }
    PingResult ping(const QHostAddress& address, unsigned time_limit = 0)
    {
        return ping(address.toString(), time_limit);
    }

    long long getElapsedTime() { return mElapsed; }
    long long getBytes() { return mBytes; }

    static int pings(const QString &address, int n = 4, unsigned time_limit = 0);

    static int DefaultTimeLimit; ///< ping max time, in seconds
private:
    long long mElapsed;
    long long mBytes;

};

#endif  //__PING_HPP__
