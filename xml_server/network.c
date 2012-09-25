#include "network.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

gint 
network_address_refresh_name(
    network_address_t*      addr
) 
{
    /* resolve the peer-addr if we haven't done so yet */
    if (addr->name->len > 0) 
        return 0;

    switch (addr->addr.common.sa_family) 
    {
    case AF_INET:
        g_string_printf(addr->name, "%s_%d", 
            inet_ntoa(addr->addr.ipv4.sin_addr),
            ntohs(addr->addr.ipv4.sin_port));
        break;
#ifdef HAVE_SYS_UN_H
    case AF_UNIX:
        g_string_assign(addr->name, addr->addr.un.sun_path);
        break;
#endif
    default:
        if (addr->addr.common.sa_family > AF_MAX)
            g_debug("%s.%d: ignoring invalid sa_family %d", __FILE__, __LINE__, addr->addr.common.sa_family);
        else
            g_warning("%s.%d: can't convert addr-type %d into a string",
            __FILE__, __LINE__, 
            addr->addr.common.sa_family);
        return -1;
    }

    return 0;
}

network_address_t*
network_address_new()
{
    network_address_t*      addr;

    addr = g_new0(network_address_t, 1);

    addr->len = sizeof(addr->addr.common);
    addr->name = g_string_new(NULL);

    return addr;
}

void
network_address_free(
    network_address_t*      addr               
)
{
    if (addr == NULL)
        return;

    g_string_free(addr->name, TRUE);

    g_free(addr);

    return;
}

gint 
network_address_set_address_ip(
    network_address_t*      addr, 
    const gchar*            address, 
    guint                   port
) 
{
	g_return_val_if_fail(addr, -1);

	if (port > 65535) {
		g_critical("%s: illegal value %u for port, only 1 ... 65535 allowed",
				G_STRLOC, port);
		return -1;
	}

	memset(&addr->addr.ipv4, 0, sizeof(struct sockaddr_in));

	if (NULL == address ||
	    strlen(address) == 0 || 
	    0 == strcmp("0.0.0.0", address)) 
    {
		/* no ip */
		addr->addr.ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
		addr->addr.ipv4.sin_family = AF_INET; /* "default" family */
	} 
    else 
    {
#ifdef HAVE_GETADDRINFO
		struct addrinfo *ai = NULL, hint;
		int				rc, family;
		
		memset(&hint, 0, sizeof (hint));
		/*
		 * FIXME: when we add support for IPv6, we'll have to do one
		 * PF_INET* after the other
		 */
		hint.ai_family = PF_INET;
		if ((rc = getaddrinfo(address, NULL, &hint, &ai)) != 0) 
        {
			g_critical("getaddrinfo(%s) failed with %s", address, 
					   gai_strerror(rc));
			return -1;
		}

		do {
			family = ai->ai_family;
#if 0 /* keep this for when we do IPv6 */
			if (family == PF_INET6) {
				memcpy(&addr->addr.ipv6,
						(struct sockaddr_in6 *) ai->ai_addr,
						sizeof (addr->addr.ipv6));
				break;
			} 
#endif /* 0 */
			if (family == PF_INET) 
            {
				memcpy(&addr->addr.ipv4,
						(struct sockaddr_in *) ai->ai_addr, 
						sizeof (addr->addr.ipv4));
				break;
			}
			ai = ai->ai_next;
		} while (NULL != ai);

		if (ai == NULL) 
        {
			/* the family we print here is the *last* ai's */
			g_critical("address %s doesn't resolve to a valid/supported "
					   "address family: %d expected, last found %d", address,
					   PF_INET, family);
			freeaddrinfo(ai);
			return -1;
		}

		freeaddrinfo(ai);
#else 
		struct hostent	*he;
		static GStaticMutex gh_mutex = G_STATIC_MUTEX_INIT;

		g_static_mutex_lock(&gh_mutex);

		he = gethostbyname(address);
		if (NULL == he) {
			g_static_mutex_unlock(&gh_mutex);
			return -1;
		}

		g_assert(he->h_addrtype == AF_INET);
		g_assert(he->h_length == sizeof(struct in_addr));

		memcpy(&(addr->addr.ipv4.sin_addr.s_addr), he->h_addr_list[0], he->h_length);
		g_static_mutex_unlock(&gh_mutex);
		addr->addr.ipv4.sin_family = AF_INET;
#endif /* HAVE_GETADDRINFO */
	}

	addr->addr.ipv4.sin_port = htons(port);
	addr->len = sizeof(struct sockaddr_in);

	(void) network_address_refresh_name(addr);

	return 0;
}

network_socket_t*
network_socket_new()
{
    network_socket_t*   socket = g_new0(network_socket_t, 1);

    socket->fd = -1;
    socket->addr_type = SOCKET_ADDR_TYPE_LOCAL;

    return socket;
}

void
network_socket_free(
    network_socket_t*       socket                    
)
{
    if (socket == NULL)
        return;

    if (socket->fd != -1)
    {
        shutdown(socket->fd, SHUT_RDWR);
        closesocket(socket->fd);
    }

    network_address_free(socket->addr);
    
    g_free(socket);
}

gint
network_socket_write(
    network_socket_t*       socket,
    void*                   buf,
    guint                   len
)
{
    guint           n_left;
    guint           n_write;
    char*           ptr;

    n_left  = len;
    ptr     = buf;

    while(n_left > 0)
    {
        if ((n_write = send(socket->fd, ptr, n_left, 0)) <= 0)
        {
            if(n_write < 0 && errno == EINTR)
                n_write = 0; /*若出错返回－1是因为被信号中断，继续写 */
            else
                return(-1);  /*若不是因为被信号中断而出错返回－1或者返回0时，则不再写writen返回－1 */
        }

        n_left -= n_write;//当write返回值>0但不等于n时，继续写

        ptr += n_write;
    }
    
    return len;
}

gint
network_socket_read(
    network_socket_t*       socket,
    void*                   buf,
    guint                   len
)
{
    guint   nleft;
    guint   nread;
    char*   ptr;

    ptr = buf;
    nleft = len;
    while(nleft > 0){
        if((nread = recv(socket->fd, ptr, nleft, 0))<0)
        {
            //当read返回－1，
            if(errno==EINTR)
                nread=0;/* 若出错是因为被信号中断，继续读 */
            else
                return(-1); //若不是因为被信号中断而出错，则不再读readn返回－1
        } 
        else if (nread == 0) //当read返回0时，不再读下去，readn返回已经读的字节数
            break; /* EOF */

        nleft -= nread;//当read返回值>0但不等于n时，继续读

        ptr += nread;
    }
    return (len-nleft);/* return >= 0 */
}



// gint 
// network_socket_bind(
//     network_socket_t*         con
// ) 
// {
// #ifdef WIN32
//     char val = 1;	/* Win32 setsockopt wants a const char* instead of the UNIX void*...*/
// #else
//     int val = 1;
// #endif
//     g_return_val_if_fail(con->fd < 0, NETWORK_SOCKET_ERROR); /* socket is already bound */
//     g_return_val_if_fail((con->socket_type == SOCK_DGRAM) || (con->socket_type == SOCK_STREAM), NETWORK_SOCKET_ERROR);
// 
//     if (con->socket_type == SOCK_STREAM) 
//     {
//         g_return_val_if_fail(con->dst, NETWORK_SOCKET_ERROR);
//         //g_return_val_if_fail(con->dst->name->len > 0, NETWORK_SOCKET_ERROR);
// 
//         if (-1 == (con->fd = socket(con->dst->addr.common.sa_family, con->socket_type, 0))) 
//         {
//             g_critical("%s: socket(%s) failed: %s (%d)", 
//                 G_STRLOC,
//                 con->dst->name->str,
//                 g_strerror(errno), errno);
//             return NETWORK_SOCKET_ERROR;
//         }
// 
//         if (con->dst->addr.common.sa_family == AF_INET || 
//             con->dst->addr.common.sa_family == AF_INET6) 
//         {
//                 if (0 != setsockopt(con->fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val))) 
//                 {
//                     g_critical("%s: setsockopt(%s, IPPROTO_TCP, TCP_NODELAY) failed: %s (%d)", 
//                         G_STRLOC,
//                         con->dst->name->str,
//                         g_strerror(errno), errno);
//                     return NETWORK_SOCKET_ERROR;
//                 }
// 
//                 if (0 != setsockopt(con->fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))) 
//                 {
//                     g_critical("%s: setsockopt(%s, SOL_SOCKET, SO_REUSEADDR) failed: %s (%d)", 
//                         G_STRLOC,
//                         con->dst->name->str,
//                         g_strerror(errno), errno);
//                     return NETWORK_SOCKET_ERROR;
//                 }
//         }
// 
//         if (-1 == bind(con->fd, &con->dst->addr.common, con->dst->len)) 
//         {
//             g_critical("%s: bind(%s) failed: %s (%d)", 
//                 G_STRLOC,
//                 con->dst->name->str,
//                 g_strerror(errno), errno);
//             return NETWORK_SOCKET_ERROR;
//         }
// 
//         if (-1 == listen(con->fd, 128)) 
//         {
//             g_critical("%s: listen(%s, 128) failed: %s (%d)",
//                 G_STRLOC,
//                 con->dst->name->str,
//                 g_strerror(errno), errno);
//             return NETWORK_SOCKET_ERROR;
//         }
//     } 
//     else 
//     {
//         /* UDP sockets bind the ->src address */
//         g_return_val_if_fail(con->src, NETWORK_SOCKET_ERROR);
//         g_return_val_if_fail(con->src->name->len > 0, NETWORK_SOCKET_ERROR);
// 
//         if (-1 == (con->fd = socket(con->src->addr.common.sa_family, con->socket_type, 0))) 
//         {
//             g_critical("%s: socket(%s) failed: %s (%d)", 
//                 G_STRLOC,
//                 con->src->name->str,
//                 g_strerror(errno), errno);
//             return NETWORK_SOCKET_ERROR;
//         }
// 
//         if (-1 == bind(con->fd, &con->src->addr.common, con->src->len)) 
//         {
//             g_critical("%s: bind(%s) failed: %s (%d)", 
//                 G_STRLOC,
//                 con->src->name->str,
//                 g_strerror(errno), errno);
//             return NETWORK_SOCKET_ERROR;
//         }
//     }
// 
//     return NETWORK_SUCCESS;
// }
