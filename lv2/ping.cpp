#include <stdio.h>
#include "ping.h"
#include "lanview.h"

#ifdef Q_OS_LINUX
#include <iostream>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <sys/time.h>

#define BIG_HDR(s)  s

using namespace std;

static const unsigned  BYTES_TO_SEND = 60000;
static const unsigned  IPLEN = 20;
static const unsigned  MAXIPLEN = 60;
static const unsigned  ICMP_MAXLEN = 65536 - MAXIPLEN - ICMP_MINLEN;

static const int SEQUENCE = 12345;
int Pinger::DefaultTimeLimit = 1; // ping max time, in seconds

// --------------------------------------------------------------------------------
//
//  Checksum
//
// --------------------------------------------------------------------------------

static uint16_t in_cksum(uint16_t *addr, unsigned len)
{
    uint16_t answer = 0;

    // --------------------------------------------------------------------------------
    // Our algorithm is simple, using a 32 bit accumulator (sum), we add
    // sequential 16 bit words to it, and at the end, fold back all the
    // carry bits from the top 16 bits into the lower 16 bits.
    // --------------------------------------------------------------------------------
    uint32_t sum = 0;
    while (len > 1)  {
        sum += *addr++;
        len -= 2;
    }
    // --------------------------------------------------------------------------------
    // mop up an odd byte, if necessary
    // --------------------------------------------------------------------------------
    if (len == 1) {
        *(unsigned char *)&answer = *(unsigned char *)addr ;
        sum += answer;
    }

    // --------------------------------------------------------------------------------
    // add back carry outs from top 16 bits to low 16 bits
    // --------------------------------------------------------------------------------
    sum = (sum >> 16) + (sum & 0xffff); // add high 16 to low 16
    sum += (sum >> 16); // add carry
    answer = ~sum; // truncate to 16 bits
    return answer;
}
#endif
// --------------------------------------------------------------------------------
//
//  Ping (send ICMP-packet and catch echo reply)
//
// --------------------------------------------------------------------------------

#ifndef Q_OS_WIN
PingResult Pinger::ping(const char* __address, unsigned time_limit)
#else
PingResult Pinger::ping(const char*, unsigned)
#endif
{
#ifndef Q_OS_WIN
#define ADDR_SIZE 39
    static char address[ADDR_SIZE +1];
    _DBGFN() << "@(" << address << _sComma << time_limit << ")" << endl;
    if (isalpha(address[0])) {
        QHostInfo hi = QHostInfo::fromName(QString(address));
        if (hi.addresses().isEmpty()) {
            DERR() << "Invalis address : " << __address << endl;
            return PING_ERROR;
        }
        strncpy(address, hi.addresses().first().toString().toStdString().c_str(), ADDR_SIZE);
    }
    else strncpy(address, __address, ADDR_SIZE);
    if(!time_limit)
        time_limit = DefaultTimeLimit;

    // --------------------------------------------------------------------------------
    // Open raw socket
    // --------------------------------------------------------------------------------
    int socket_id = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (socket_id < 0)
    {
        DERR() << "Error calling function 'socket'" << endl;      /* probably not running as superuser */
        return PING_ERROR;
    }
    // --------------------------------------------------------------------------------
    // Create ICMP packet
    // --------------------------------------------------------------------------------
    char packet[ICMP_MINLEN] = {'\0'}; // empty packet

    icmp* icmp_header = reinterpret_cast<icmp*>(packet);   // 64 bits
    icmp_header->icmp_type = ICMP_ECHO; //  0 - 7 bits
    icmp_header->icmp_code = 0;         //  8 - 15 bits
    icmp_header->icmp_cksum = 0;        //  16 - 31 bits
    icmp_header->icmp_id = getpid();    //  32 - 47 bits
    icmp_header->icmp_seq = SEQUENCE;   //  48 - 63 bits

    // after filling all the fields, calculate checksum
    icmp_header->icmp_cksum = in_cksum(reinterpret_cast<uint16_t*>(packet),
                                       sizeof(packet));

    // --------------------------------------------------------------------------------
    // "To" (set recipient address)
    // --------------------------------------------------------------------------------
    sockaddr_in to;
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = inet_addr(address);
    /* warning: comparison of unsigned expression < 0 is always false
    if (to.sin_addr.s_addr < 0)
    {
        DERR() << BIG_HDR("")<< "Error calling function 'inet_addr'" << endl;
        return PING_ERROR;
    }*/

    // --------------------------------------------------------------------------------
    // Start timer
    // --------------------------------------------------------------------------------
    timeval start;
    gettimeofday(&start, NULL);

    // --------------------------------------------------------------------------------
    // Send packet
    // --------------------------------------------------------------------------------
    int bytes_sent = sendto(socket_id, packet, sizeof(packet) , 0,
                            reinterpret_cast<const sockaddr*>(&to), sizeof(to));
    if (bytes_sent < 0)
    {
        DERR() <<"Error calling function 'sendto'" << endl;
        return PING_ERROR;
    }
    PDEB(INFO) << BIG_HDR("") << "sent " <<  bytes_sent*8 << " bits " << "\n";
    mBytes = bytes_sent;


    PingResult res = PING_SILENCE;
    // --------------------------------------------------------------------------------
    // Listen to echo (several seconds)
    // --------------------------------------------------------------------------------
    for(unsigned i=0; i<time_limit ;i++)
    {
        // ---------------------------------------------------------------------------
        // Use 'select' to define if socket is ready
        // ---------------------------------------------------------------------------
        fd_set rfds; // this descriptor needs to be verified if it's ready for reading
        FD_ZERO(&rfds);
        FD_SET(socket_id, &rfds);
        timeval tv; // time interval of the verifying = 1 sec.
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ready = select(socket_id + 1, &rfds, NULL, NULL, &tv);

        // ---------------------------------------------------------------------------
        // 'select' error
        // ---------------------------------------------------------------------------
        if (ready < 0)
        {
            DERR() << "Error calling function 'select'" << endl;
            return PING_ERROR;
        }

        // ---------------------------------------------------------------------------
        // socket not ready
        // ---------------------------------------------------------------------------
        else if (ready == 0)
        {
            continue;
        }

        // ---------------------------------------------------------------------------
        // socket ready
        // ---------------------------------------------------------------------------
        else if (ready > 0)
        {
            PDEB(INFO) << BIG_HDR("") << "select: " << ready << "\n";

            // ---------------------------------------------------------------------------
            // Read a packet
            // ---------------------------------------------------------------------------
            char packet[ICMP_MAXLEN]  = {'\0'};
            sockaddr_in from;
            socklen_t fromlen = 0;

            int bytes_read = recvfrom(socket_id, packet, ICMP_MAXLEN, 0,
                                      reinterpret_cast<sockaddr*>(&from), &fromlen);

            if(bytes_read < 0)
            {
                DERR() << "Error calling function 'recvfrom'" << endl;
                return PING_ERROR;
            }
            PDEB(INFO) << BIG_HDR("") << "read " <<  bytes_read*8 << " bits \n";

            // --------------------------------------------------------------------------------
            // Check the IP header
            // --------------------------------------------------------------------------------
            ip* ip_header = reinterpret_cast<ip*>(&packet);

            if (static_cast<unsigned>(bytes_read) < (sizeof(ip_header) + ICMP_MINLEN))
            {
                DERR() << "packet too short (" << bytes_read*8  << " bits) from " << address << endl;
                return PING_ERROR;
            }

            mBytes += bytes_read;
            // --------------------------------------------------------------------------------
            // Now the ICMP part
            // --------------------------------------------------------------------------------
            icmp* icmp_header = reinterpret_cast<icmp*>(packet + IPLEN);
            if (icmp_header->icmp_type == ICMP_ECHOREPLY)
            {
                if (icmp_header->icmp_seq != SEQUENCE)
                {
                    PDEB(INFO) << BIG_HDR("") << "ERR: received sequence # " << icmp_header->icmp_seq << "\n";
                    continue;
                }
                if (icmp_header->icmp_id != getpid())
                {
                    PDEB(INFO) << BIG_HDR("") << "ERR: received id " << icmp_header->icmp_id << "\n";
                    continue;
                }
                res = PING_SUCCESS;
                break;
            }
            else
            {
                PDEB(INFO) << BIG_HDR("") << "ERR: not an echo reply" << "\n";
                continue;
            }
        }
    }
    // --------------------------------------------------------------------------------
    // Close socket
    // --------------------------------------------------------------------------------
    close(socket_id);

    // --------------------------------------------------------------------------------
    // Stop timer
    // --------------------------------------------------------------------------------
    timeval end;
    gettimeofday(&end, NULL);

    mElapsed = 1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
    if(mElapsed < 1)
        mElapsed = 1;
//#warning "Compile to linux."
#else
    DWAR() << "Not supported" << endl;
    PingResult res = PING_ERROR;
//#warning "Compile to not linux."
#endif
    return res;
}

int Pinger::pings(const QString& address, int n, unsigned time_limit)
{
    Pinger p;
    int r = 0;
    for (int i = 0; i < n; i++) {
        switch (p.ping(address, time_limit)) {
        case PING_SUCCESS:  ++r;    break;
        case PING_ERROR:    return -1;
        case PING_SILENCE:  break;
        default:            EXCEPTION(EPROGFAIL);
        }
    }
    return r;
}

