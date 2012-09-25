#include "../xml_server/network.h"
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>

char*           ip_addr = "127.0.0.1";
guint           port = 12345;



gpointer
client_thread(
    void*       user_arg              
)
{
    int                     fd;
    network_address_t*      listen_addr = NULL;
    char                    buf[100];
    int                     ret;
    int                     left;
    int                     send_cnt = 0;
    int                     i = 0;

    switch (1)
    {
    case 1:
        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd == -1)
        {
            printf("socket error\n");
            break;
        }

        listen_addr = network_address_new();
        listen_addr->addr.ipv4.sin_family = AF_INET;

        listen_addr->addr.ipv4.sin_addr.s_addr = inet_addr(ip_addr);

        listen_addr->addr.ipv4.sin_port = htons(port);
        listen_addr->len = sizeof(listen_addr->addr.ipv4);

        if (-1 == connect(fd, &listen_addr->addr.common, listen_addr->len))
        {
            printf("connect error\n");
            break;
        }

        while (i++ < 1000000)
        {
            g_strlcpy(buf, "abcabcabcabcabcabcabcabcabcabc\n", 100);
            left = strlen(buf);
            ret = send(fd, buf, left, 0);
        }
       
        g_strlcpy(buf, "XML_TRANSFER_END", 100);
        left = strlen(buf);
        ret = send(fd, buf, left, 0);

        shutdown(fd, 1);
        while(recv(fd, buf, 100, 0) > 0)
        {
            printf("%s\n", buf);
        }

        closesocket(fd);
    }

    printf("exit\n");

    return NULL;
}

int
xml_client_init()
{
    //init glib and winsocket

    const gchar *check_str = NULL;

    if (!GLIB_CHECK_VERSION(2, 6, 0)) {
        g_critical("the glib header are too old, need at least 2.6.0, got: %d.%d.%d", 
            GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);

        return -1;
    }

    check_str = glib_check_version(GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);

    if (check_str) {
        g_critical("%s, got: lib=%d.%d.%d, headers=%d.%d.%d", 
            check_str,
            glib_major_version, glib_minor_version, glib_micro_version,
            GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);

        return -1;
    }

    g_thread_init(NULL);

#ifdef _WIN32
    {
        //Initialize Winsock
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (iResult != NO_ERROR)
            printf("Error at WSAStartup()\n");
    }
#endif // _WIN32

    return 0;
}

int
main(
     int         argc, 
     char        **argv
)
{
    char*           ip_addr = "";
    guint           port = 10;
    GThread*        thread;
    int             i;
    GError*         gerr = NULL;
    gchar           buf[100];

    if (xml_client_init())
        exit(-1);

    for (i = 0; i < 100; ++i)
    {
        thread = g_thread_create(client_thread, NULL, TRUE, &gerr);
        if (gerr != NULL)
        {
            printf("%s\n", gerr->message);
            g_error_free(gerr);
            gerr = NULL;
        }
    }

    fgets(buf, 100, stdin);
 
    return 0;
}