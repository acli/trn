/* nntpinit.c
*/
/* This software is copyrighted as detailed in the LICENSE file. */


/*#define DECNET	*//* If you want decnet support */
/*#define EXCELAN	*//* Excelan EXOS 205 support */
/*#define NONETD	*//* Define if you're missing netdb.h */

#include "EXTERN.h"
#include "common.h"
#include "nntpclient.h"
#include "INTERN.h"
#include "nntpinit.h"
#include "nntpinit.ih"

#ifdef SUPPORT_NNTP

#ifdef WINSOCK
#include <winsock.h>
WSADATA wsaData;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef NONETDB
# define IPPORT_NNTP	((unsigned short) 119)
#else
# include <netdb.h>
#endif
#endif

#ifdef EXCELAN
int connect _((int, struct sockaddr*));
unsigned short htons _((unsigned short));
unsigned long rhost _((char**));
int rresvport p((int));
int socket _((int, struct sockproto *, struct sockaddr_in *, int));
#endif /* EXCELAN */

#ifdef DECNET
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#endif /* DECNET */

#ifndef WINSOCK
unsigned long inet_addr _((char*));
#ifndef NONETDB
struct servent* getservbyname();
struct hostent* gethostbyname();
#endif
#endif

int
init_nntp()
{
#ifdef WINSOCK
    if (WSAStartup(0x0101,&wsaData) == 0) {
	if (wsaData.wVersion == 0x0101)
	    return 1;
	WSACleanup();
    }
    fprintf(stderr,"Unable to initialize WinSock DLL.\n");
    return -1;
#else
    return 1;
#endif
}

int
server_init(machine)
char* machine;
{
    int sockt_rd, sockt_wr;
#ifdef DECNET
    char* cp;
#endif

#ifdef DECNET
    cp = index(machine, ':');

    if (cp && cp[1] == ':') {
	*cp = '\0';
	sockt_rd = get_dnet_socket(machine);
    } else
	sockt_rd = get_tcp_socket(machine, nntplink.port_number, "nntp");
#else /* !DECNET */
    sockt_rd = get_tcp_socket(machine, nntplink.port_number, "nntp");
#endif

    if (sockt_rd < 0)
	return -1;

    sockt_wr = dup(sockt_rd);

    /* Now we'll make file pointers (i.e., buffered I/O) out of
    ** the socket file descriptor.  Note that we can't just
    ** open a fp for reading and writing -- we have to open
    ** up two separate fp's, one for reading, one for writing. */
    if ((nntplink.rd_fp = fdopen(sockt_rd, "r")) == NULL) {
	perror("server_init: fdopen #1");
	return -1;
    }
    if ((nntplink.wr_fp = fdopen(sockt_wr, "w")) == NULL) {
	perror("server_init: fdopen #2");
	nntplink.rd_fp = NULL;
	return -1;
    }

    /* Now get the server's signon message */
    nntp_check();

    if (*ser_line == NNTP_CLASS_OK) {
	char save_line[NNTP_STRLEN];
	strcpy(save_line, ser_line);
	/* Try MODE READER just in case we're talking to innd.
	** If it is not an invalid command, use the new reply. */
	if (nntp_command("MODE READER") <= 0)
	    sprintf(ser_line, "%d failed to send MODE READER\n", NNTP_ACCESS_VAL);
	else if (nntp_check() <= 0 && atoi(ser_line) == NNTP_BAD_COMMAND_VAL)
	    strcpy(ser_line, save_line);
    }
    return atoi(ser_line);
}

void
cleanup_nntp()
{
#ifdef WINSOCK
    WSACleanup();
#endif
}

int
get_tcp_socket(machine, port, service)
char* machine;
int port;
char* service;
{
    int s;
#if INET6
    struct addrinfo hints;
    struct addrinfo* res;
    struct addrinfo* res0;
    char portstr[8];
    char* cause = NULL;
    int error;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    if (port)
	sprintf(service = portstr, "%d", port);
    error = getaddrinfo(machine, service, &hints, &res0);
    if (error) {
	fprintf(stderr, "%s", gai_strerror(error));
	return -1;
    }
    for (res = res0; res; res = res->ai_next) {
	char buf[64] = "";
	s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (s < 0) {
	    cause = "socket";
	    continue;
	}

	inet_ntop(res->ai_family, res->ai_addr, buf, sizeof buf);
	if (res != res0)
	    fprintf(stderr, "trying %s...", buf);

	if (connect(s, res->ai_addr, res->ai_addrlen) >= 0)
	    break;  /* okay we got one */

	fprintf(stderr, "connection to %s: ", buf);
	perror("");
	cause = "connect";
	close(s);
	s = -1;
    }
    if (s < 0) {
	fprintf(stderr, "giving up... ");
	perror(cause);
    }
    freeaddrinfo(res0);
#else	/* !INET6 */
    struct sockaddr_in sin;
#ifdef __hpux
    int socksize = 0;
    int socksizelen = sizeof socksize;
#endif
#ifdef NONETDB
    bzero((char*)&sin, sizeof sin);
    sin.sin_family = AF_INET;
#else
    struct hostent* hp;
#ifdef h_addr
    int x = 0;
    register char** cp;
    static char* alist[1];
#endif /* h_addr */
    static struct hostent def;
    static struct in_addr defaddr;
    static char namebuf[256];

    bzero((char*)&sin, sizeof sin);

    if (port)
	sin.sin_port = htons(port);
    else {
	struct servent* sp;
	if ((sp = getservbyname(service, "tcp")) == NULL) {
	    fprintf(stderr, "%s/tcp: Unknown service.\n", service);
	    return -1;
	}
	sin.sin_port = sp->s_port;
    }
    /* If not a raw ip address, try nameserver */
    if (!isdigit(*machine)
#ifdef INADDR_NONE
     || (defaddr.s_addr = inet_addr(machine)) == INADDR_NONE)
#else
     || (long)(defaddr.s_addr = inet_addr(machine)) == -1)
#endif
	hp = gethostbyname(machine);
    else {
	/* Raw ip address, fake  */
	(void) strcpy(namebuf, machine);
	def.h_name = namebuf;
#ifdef h_addr
	def.h_addr_list = alist;
#endif
	def.h_addr = (char*)&defaddr;
	def.h_length = sizeof (struct in_addr);
	def.h_addrtype = AF_INET;
	def.h_aliases = 0;
	hp = &def;
    }
    if (hp == NULL) {
	fprintf(stderr, "%s: Unknown host.\n", machine);
	return -1;
    }

    sin.sin_family = hp->h_addrtype;
#endif /* !NONETDB */

    /* The following is kinda gross.  The name server under 4.3
    ** returns a list of addresses, each of which should be tried
    ** in turn if the previous one fails.  However, 4.2 hostent
    ** structure doesn't have this list of addresses.
    ** Under 4.3, h_addr is a #define to h_addr_list[0].
    ** We use this to figure out whether to include the NS specific
    ** code... */
#ifdef h_addr
    /* get a socket and initiate connection -- use multiple addresses */
    for (cp = hp->h_addr_list; cp && *cp; cp++) {
	extern char* inet_ntoa _((const struct in_addr));
	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s < 0) {
	    perror("socket");
	    return -1;
	}
        bcopy(*cp, (char*)&sin.sin_addr, hp->h_length);
		
	if (x < 0)
	    fprintf(stderr, "trying %s\n", inet_ntoa(sin.sin_addr));
	x = connect(s, (struct sockaddr*)&sin, sizeof (sin));
	if (x == 0)
	    break;
        fprintf(stderr, "connection to %s: ", inet_ntoa(sin.sin_addr));
	perror("");
	(void) close(s);
    }
    if (x < 0) {
	fprintf(stderr, "giving up...\n");
	return -1;
    }
#else /* no name server */
#ifdef EXCELAN
    s = socket(SOCK_STREAM, (struct sockproto*)NULL, &sin, SO_KEEPALIVE);
    if (s < 0) {
	/* Get the socket */
	perror("socket");
	return -1;
    }
    bzero((char*)&sin, sizeof sin);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port? port : *service == 'n'? IPPORT_NNTP : 80);

    /* set up addr for the connect */
    if ((sin.sin_addr.s_addr = rhost(&machine)) == -1) {
	fprintf(stderr, "%s: Unknown host.\n", machine);
	return -1;
    }

    /* And then connect */
    if (connect(s, (struct sockaddr*)&sin) < 0) {
	perror("connect");
	(void) close(s);
	return -1;
    }
#else /* !EXCELAN */
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("socket");
	return -1;
    }

    /* And then connect */

    bcopy(hp->h_addr, (char*)&sin.sin_addr, hp->h_length);
    if (connect(s, (struct sockaddr*)&sin, sizeof sin) < 0) {
	perror("connect");
	(void) close(s);
	return -1;
    }

#endif /* !EXCELAN */
#endif /* !h_addr */
#endif /* !INET6 */
#ifdef __hpux	/* recommended by raj@cup.hp.com */
#define	HPSOCKSIZE 0x8000
    getsockopt(s, SOL_SOCKET, SO_SNDBUF, (caddr_t)&socksize, (caddr_t)&socksizelen);
    if (socksize < HPSOCKSIZE) {
	socksize = HPSOCKSIZE;
	setsockopt(s, SOL_SOCKET, SO_SNDBUF, (caddr_t)&socksize, sizeof (socksize));
    }
    socksize = 0;
    socksizelen = sizeof (socksize);
    getsockopt(s, SOL_SOCKET, SO_RCVBUF, (caddr_t)&socksize, (caddr_t)&socksizelen);
    if (socksize < HPSOCKSIZE) {
	socksize = HPSOCKSIZE;
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, (caddr_t)&socksize, sizeof (socksize));
    }
#endif
    return s;
}

#ifdef DECNET
static int
get_dnet_socket(machine)
char* machine;
{
    int s, area, node;
    struct sockaddr_dn sdn;
    struct nodeent *getnodebyname(), *np;

    bzero((char*)&sdn, sizeof sdn);

    switch (s = sscanf(machine, "%d%*[.]%d", &area, &node)) {
    case 1: 
	node = area;
	area = 0;
    case 2: 
	node += area*1024;
	sdn.sdn_add.a_len = 2;
	sdn.sdn_family = AF_DECnet;
	sdn.sdn_add.a_addr[0] = node % 256;
	sdn.sdn_add.a_addr[1] = node / 256;
	break;
    default:
	if ((np = getnodebyname(machine)) == NULL) {
	    fprintf(stderr, "%s: Unknown host.\n", machine);
	    return -1;
	}
	bcopy(np->n_addr, (char*)sdn.sdn_add.a_addr, np->n_length);
	sdn.sdn_add.a_len = np->n_length;
	sdn.sdn_family = np->n_addrtype;
	break;
    }
    sdn.sdn_objnum = 0;
    sdn.sdn_flags = 0;
    sdn.sdn_objnamel = strlen("NNTP");
    bcopy("NNTP", &sdn.sdn_objname[0], sdn.sdn_objnamel);

    if ((s = socket(AF_DECnet, SOCK_STREAM, 0)) < 0) {
	nerror("socket");
	return -1;
    }

    /* And then connect */
    if (connect(s, (struct sockaddr*)&sdn, sizeof sdn) < 0) {
	nerror("connect");
	close(s);
	return -1;
    }
    return s;
}
#endif /* DECNET */

/*
 * inet_addr for EXCELAN (which does not have it!)
 */
#ifdef NONETDB
unsigned long
inet_addr(cp)
register char* cp;
{
    unsigned long val, base, n;
    register char c;
    unsigned long octet[4], *octetptr = octet;
#ifndef htonl
    extern  unsigned long   htonl();
#endif  /* htonl */
again:
    /* Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, other=decimal. */
    val = 0; base = 10;
    if (*cp == '0')
	base = 8, cp++;
    if (*cp == 'x' || *cp == 'X')
	base = 16, cp++;
    while (c = *cp) {
	if (isdigit(c)) {
	    val = (val * base) + (c - '0');
	    cp++;
	    continue;
	}
	if (base == 16 && isxdigit(c)) {
	    val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
	    cp++;
	    continue;
	}
	break;
    }
    if (*cp == '.') {
	/* Internet format:
	 *      a.b.c.d
	 *      a.b.c   (with c treated as 16-bits)
	 *      a.b     (with b treated as 24 bits) */
	if (octetptr >= octet + 4)
	    return -1;
	*octetptr++ = val, cp++;
	goto again;
    }
    /* Check for trailing characters. */
    if (*cp && !isspace(*cp))
	return -1;
    *octetptr++ = val;
    /* Concoct the address according to the number of octet specified. */
    n = octetptr - octet;
    switch (n) {
    case 1:    			/* a -- 32 bits */
	val = octet[0];
	break;
    case 2:			/* a.b -- 8.24 bits */
	val = (octet[0] << 24) | (octet[1] & 0xffffff);
	break;
    case 3:			/* a.b.c -- 8.8.16 bits */
	val = (octet[0] << 24) | ((octet[1] & 0xff) << 16) |
	    (octet[2] & 0xffff);
	break;
    case 4:			/* a.b.c.d -- 8.8.8.8 bits */
	val = (octet[0] << 24) | ((octet[1] & 0xff) << 16) |
	    ((octet[2] & 0xff) << 8) | (octet[3] & 0xff);
	break;
    default:
	return -1;
    }
    val = htonl(val);
    return val;
}
#endif /* NONETDB */

#endif /* SUPPORT_NNTP */
