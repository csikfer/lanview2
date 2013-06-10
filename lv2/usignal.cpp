/***************************************************************************
 *   Copyright (C) 2013 by Csiki Ferenc   *
 *   csikfer@csikfer@gmail.com   *
 *                                                                         *
 ***************************************************************************/
#include "signal.h"
#include "cdebug.h"
#include "cerror.h"

#include "usignal.h"

#ifdef MUST_USIGNAL
extern int errno;
#include <sys/types.h>
#include <sys/socket.h>

cXSignal *cXSignal::pInstance = NULL;

cXSignal::cXSignal(QObject *parent) : QObject(parent)
{
    if (pInstance != NULL) EXCEPTION(EPROGFAIL);
    pInstance = this;
    sigFd[0] = sigFd[1] = -1;
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigFd))
        EXCEPTION(EFOPEN, -1, "Couldn't create socketpair");
    sn = new QSocketNotifier(sigFd[1], QSocketNotifier::Read, this);
    connect(sn, SIGNAL(activated(int)), this, SLOT(qhandle(int)), Qt::QueuedConnection);
}
cXSignal::~cXSignal()
{
    if (sn != NULL) {
        delete sn;
//        ::close(sigFd[0]);
//        ::close(sigFd[1]);
    }
    pInstance = NULL;
}

void cXSignal::xHandler(int __i)
{
    int e = ::write(sigFd[0], &__i, sizeof(int));
    if (sizeof(int) != e) {
        fprintf(stderr, "Error in cXSignal::xHandler(%i), write returned %i, errno = %i\n", __i, e, errno);
    }
}
void cXSignal::qhandle(int)
{
    int i, e;
    sn->setEnabled(false);
    e = ::read(sigFd[1], &i, sizeof(int));
    if (sizeof(int) != e) {
        fprintf(stderr, "Error in cXSignal::qhandle(), read returned %i, errno = %i\n", e, errno);
        i = -1;
    }
    qsignal(i);
    sn->setEnabled(true);
}

void cXSignal::unixSignalHandler(int __i)
{
    if (pInstance == NULL) {
        fprintf(stderr, "unixSignalHandler(%d): instance or unixSignals pointer is NULL, signal ignored.\n", __i);
        return;
    }
    pInstance->xHandler(__i);
}

void cXSignal::setupUnixSignalHandlers()
{
    struct sigaction siga;

    siga.sa_handler = cXSignal::unixSignalHandler;
    sigemptyset(&siga.sa_mask);
    siga.sa_flags = 0;
    siga.sa_flags |= SA_RESTART;

    if (sigaction(SIGHUP, &siga, 0) > 0
     || sigaction(SIGINT, &siga, 0) > 0
     || sigaction(SIGALRM, &siga, 0) > 0)
        EXCEPTION(EPROGFAIL);
}
#endif  // MUST_USIGNAL

