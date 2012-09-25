#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <glib.h>
#include <errno.h>

#define NETWORK_SUCCESS         0

#define NETWORK_ERROR           -1
#define NETWORK_ADDR_ERROR      -2
#define NETWORK_SOCKET_ERROR    -3 

#ifndef _WIN32
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>     /** struct sockaddr_in */
#endif
#include <netinet/tcp.h>
#include <netdb.h>

#define closesocket(x) close(x)

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>         /** struct sockaddr_un */
#endif
#include <sys/socket.h>     /** struct sockaddr (freebsd and hp/ux need it) */
#else
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <errno.h>
#ifndef SHUT_RDWR
#define SHUT_RDWR 0x02
#endif

#endif

#ifdef __hpux
/* see http://curl.haxx.se/mail/lib-2009-04/0287.html */
typedef int network_socklen_t;
#else
typedef socklen_t network_socklen_t;
#endif

typedef struct network_address_struct   network_address_t;
typedef struct network_socket_struct    network_socket_t;

struct network_address_struct
{
	union 
    {
		struct sockaddr_in ipv4;
		struct sockaddr_in6 ipv6;
#ifdef HAVE_SYS_UN_H
		struct sockaddr_un un;
#endif
		struct sockaddr common;
	} addr;

 	GString*            name; 
 	network_socklen_t   len;	
// 	gboolean can_unlink_socket; /* set TRUE *only* after successful bind */
};

#define SOCKET_ADDR_TYPE_LOCAL   0
#define SOCKET_ADDR_TYPE_REMOTE  1

struct network_socket_struct{
	int                 fd;         /**< socket-fd */
	
    //struct event        event;      /**< events for this fd */

// 	network_address_t*  src;        /**< getsockname() */
// 	network_address_t*  dst;        /**< getpeername() */

    network_address_t*  addr;
    int                 addr_type;

	int                 socket_type; /**< SOCK_STREAM or SOCK_DGRAM for now */
};

network_address_t*
network_address_new();

void
network_address_free(
    network_address_t*      addr               
);

gint 
network_address_set_address_ip(
    network_address_t*      addr, 
    const gchar*            address, 
    guint                   port
);

network_socket_t*
network_socket_new();

void
network_socket_free(
    network_socket_t*      socket 
);

gint 
network_socket_bind(
    network_socket_t*         con
);

gint
network_socket_read(
    network_socket_t*       socket,
    void*                   buf,
    guint                   len
);

gint
network_socket_write(
    network_socket_t*       socket,
    void*                   buf,
    guint                   len
);

gint 
network_address_refresh_name(
    network_address_t*      addr
);

#endif
