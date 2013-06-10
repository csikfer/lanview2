/***************************************************************************
 *   Copyright (C) 2007 by Csiki Ferenc   *
 *   csikfer@csikfer@gmail.com   *
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
#ifndef USIGNAL_H_INCLUDED
#define USIGNAL_H_INCLUDED

#include <QtCore>
#include <QtNetwork>

#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX)
#define MUST_USIGNAL
#endif

#ifdef MUST_USIGNAL
class cXSignal : public QObject
{
     Q_OBJECT
public:
    /// Konstruktor. Létrehozza az sigFd socket őárt, és létrehozza a szükséges QSocketNotifier objektumot, és signal-slot konnekteket.
     cXSignal(QObject *parent = 0);
    /// Destruktor. Törli a QSocketNotifier objetumot és a socket párt.
     ~cXSignal();
     /// Az Unix signal handlerből meghívandó metódus. A szignal handler függvény az alkalmazásban van, ahol elötte az objektumat létrahozzuk.
     void xHandler(int __i = 0);
     static void setupUnixSignalHandlers();
private:
    static void unixSignalHandler(int __i);
    /// Ezen a socket páron keresztűl adja át az i paramétert az xHandler() a qsignal()-nak. Ill. ez váltja ki a signált.
    int sigFd[2];
    ///
    QSocketNotifier *sn;
    static cXSignal *pInstance;
signals:
     /// Az xHandler hívásakor (közvetve) generált szignál.
     /// @param i	A kiváltó xHandler(i) azonos nevű paramétere
     void qsignal(int __i);
private slots:
     /// Az sigFd socketpáron átküldött paraméterre aktívált slot, ez fogja kiváltani a qhandle()-t.
     void qhandle(int);
};
#else   // MUST_USIGNAL
//#warning "Class cXSignal is disabled."
#endif  // MUST_USIGNAL

#endif  // USIGNAL_H_INCLUDED
