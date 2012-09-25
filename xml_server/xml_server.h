#ifndef _XML_SERVER_H
#define _XML_SERVER_H

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "network.h"
#include "tsk.h"
#include "unix-daemon.h"



#define IP_ADDR_LEN 100

struct global_info_stuct 
{
    guint               port;
    gchar               ip_addr[IP_ADDR_LEN + 1];
    gchar*              ip;
    gint                event_thread_cnt;

    gchar*              log_level_string;

    gchar               log_filename[100];

    gint                exit_flag;


#ifndef _WIN32
    gint                keepalive;   
#endif
};

typedef struct xml_server_log_struct xml_server_log_t;
struct xml_server_log_struct
{
    GLogLevelFlags  min_lvl;

    GString*        log_ts_str;

    FILE*           log_file_fd;

};

typedef struct xml_server_worker_thread_pool_struct xml_server_thread_pool_t;
struct xml_server_worker_thread_pool_struct 
{
    GMutex*     mutex;

    GPtrArray*  work_thread_arr;
};

int
xml_server_log_set_level(
    xml_server_log_t*       log,
    gchar*                  level_str
);

void 
xml_server_log_func(
    const gchar*            log_domain, 
    GLogLevelFlags          log_level, 
    const gchar *           message, 
    gpointer                user_data    
);

int
xml_server_new_worker_thread();

int
xml_server_thread_pool_init(
    guint               thread_cnt
);

void
xml_server_thread_pool_deinit();

gpointer
xml_server_worker_thread(
    void*       user_arg
);

#endif // _XML_SERVER_H