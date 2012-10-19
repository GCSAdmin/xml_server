//for memory leak test
#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif // _WIN32


#include "xml_server.h"

gchar                   g_exe_path[1024];
gchar                   g_exe_parent_path[1024];

struct global_info_stuct global_info;

const GOptionEntry arg_options[] ={
    {"--ip_addr",               'a', 0, G_OPTION_ARG_STRING,    &global_info.ip,        "listen address(default 0.0.0.0) .", NULL},
    {"--port",                  'p', 0, G_OPTION_ARG_INT,       &global_info.port,      "listen port(default 33060) .", NULL},
    {"--worker_thread_num",     'c', 0, G_OPTION_ARG_INT,       &global_info.event_thread_cnt,   "The logging level(Should be error, critical, warning, info, message, debug, Default critical) .", NULL},
    {"--log_level",             'l', 0, G_OPTION_ARG_STRING,    &global_info.log_level_string,   "number of worker thread(default 10) .", NULL},
#ifndef _WIN32
    {"--keep_alive",            'k', 0, G_OPTION_ARG_INT,       &global_info.keepalive, "Try to restart the xml server if a crash occurs(default 1) .", NULL},
#endif // _WIN32 
    {NULL,                      0,   0, 0,                      NULL,                   NULL, NULL},
}; 

gchar* help_str = 
"\nxml server startup options.\n\
    -a, --ip_addr              listen address(default 0.0.0.0),\n\
    -p, --port                 listen port(default 33060),\n\
    -c, --worker_thread_num    number of worker thread(default 10)\n\
    -l, --log_level            The logging level(Should be error, critical, warning, info, message, debug, Default critical)\n\
    -k, --keep_alive           Try to restart the xml server if a crash occurs(default 1)\n\n";

void
xml_server_global_init()
{
    g_strlcpy(global_info.ip_addr, "0.0.0.0", IP_ADDR_LEN);
    global_info.port = 33060;
    global_info.event_thread_cnt = 10;
#ifndef _WIN32
    global_info.keepalive = 1;
#endif
}

void
xml_server_global_deinit()
{
    if (global_info.ip != NULL)
        g_free(global_info.ip);

    if (global_info.log_level_string != NULL)
        g_free(global_info.log_level_string);
}

xml_server_log_t    global_log;

int 
xml_server_parse_options(
	int*            argc_p, 
    char***         argv_p
) 
{
	int ret = 0;

    GError* gerr = NULL;

    GOptionContext * opt_ctx = g_option_context_new("- XML Server");
    
    g_option_context_add_main_entries(opt_ctx, arg_options, NULL);
    
    g_option_context_set_help_enabled(opt_ctx, FALSE);

    g_option_context_set_ignore_unknown_options(opt_ctx, FALSE);

    if (FALSE == g_option_context_parse(opt_ctx, argc_p, argv_p, &gerr))
    {
        if (gerr != NULL)
        {
            g_critical("%s xml_server_parse_options fail, reason %s \n",
                        G_STRLOC, gerr->message);

            g_error_free(gerr);
        }
        
        return -1;
    }

    g_option_context_free(opt_ctx);

    if (global_info.log_level_string != NULL)
        xml_server_log_set_level(&global_log, global_info.log_level_string);
    
    if (global_info.ip != NULL)
        strcpy(global_info.ip_addr, global_info.ip);
	
    return 0;
}

int
xml_server_get_now_time_str(
    char*           buf,
    guint           len
)
{
    struct tm*      tm;
    GTimeVal        tv;
    time_t	        t;
    size_t          time_len;

    g_get_current_time(&tv);
    t = (time_t) tv.tv_sec;
    tm = localtime(&t);

    time_len = strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm);

    return time_len;
}

int
xml_server_log_open(
    xml_server_log_t *      log
) 
{
    gchar       buf[2000];

    sprintf(buf, "%s/xml_server.log", g_exe_path);
    log->log_file_fd = fopen(buf, "a+");

    return (log->log_file_fd == NULL);
}

void
xml_server_log_close(
    xml_server_log_t*       log                     
)
{
    if (log->log_file_fd != NULL)
    {
        fclose(log->log_file_fd);        
    }
    
    log->log_file_fd = NULL;
}

void
xml_server_log_init()               
{
    if (xml_server_log_open(&global_log))
    {
        g_error("can't open log file %s/xml_server.log\n", g_exe_path);
    }
    global_log.log_ts_str   = g_string_new_len(NULL, 100);
    global_log.min_lvl      = G_LOG_LEVEL_CRITICAL;

    g_log_set_default_handler(xml_server_log_func, &global_log);
}

void
xml_server_log_deinit()
{
    xml_server_log_close(&global_log);
    g_string_free(global_log.log_ts_str, TRUE);
}

const struct {
    char *name;
    GLogLevelFlags lvl;
} log_lvl_map[] = {	/* syslog levels are different to the glib ones */
    { "error",      G_LOG_LEVEL_ERROR},
    { "critical",   G_LOG_LEVEL_CRITICAL},
    { "warning",    G_LOG_LEVEL_WARNING},
    { "message",    G_LOG_LEVEL_MESSAGE},
    { "info",       G_LOG_LEVEL_INFO},
    { "debug",      G_LOG_LEVEL_DEBUG},

    { NULL, 0 }
};

int
xml_server_log_set_level(
    xml_server_log_t*       log,
    gchar*                  level_str
)
{
    gint i;

    for (i = 0; log_lvl_map[i].name; i++) {
        if (0 == g_strcasecmp(log_lvl_map[i].name, level_str)) {
            log->min_lvl = log_lvl_map[i].lvl;
            return 0;
        }
    }

    return -1;
}

gchar*
xml_server_log_get_level_str(
    GLogLevelFlags          level
)
{
    gint i;

    for (i = 0; log_lvl_map[i].name; i++) {
        if (level == log_lvl_map[i].lvl) {
            return log_lvl_map[i].name;
        }
    }

    return NULL;
}

void 
xml_server_log_func(
    const gchar*            log_domain, 
    GLogLevelFlags          log_level, 
    const gchar *           message, 
    gpointer                user_data    
) 
{
	xml_server_log_t*   log = user_data;
    gchar           time_str_buf[1000];    
    guint           retry_cnt = 0;

	/**
	 * make sure we syncronize the order of the write-statements 
	 */
	static GStaticMutex log_mutex = G_STATIC_MUTEX_INIT;

	/* ignore the verbose log-levels */
	if (log_level > log->min_lvl) return;

	g_static_mutex_lock(&log_mutex);

    xml_server_get_now_time_str(time_str_buf, 1000);

    g_string_sprintf(log->log_ts_str, "(%s) <%s> %s", time_str_buf, xml_server_log_get_level_str(log_level), message);

write_again:
    if (-1 == fwrite(log->log_ts_str->str, log->log_ts_str->len, 1, log->log_file_fd)) 
    {
        /* writing to the file failed (Disk Full, what ever ... */
        
        //重试10次
        for (; retry_cnt < 10; ++retry_cnt)
        {
            xml_server_log_close(log);
            if (xml_server_log_open(log))
            {
                //open success
                goto write_again;
            }
            g_usleep(100);
        }

        //maybe Disk full or fail!
        abort();
    } 
    else 
    {
        fwrite("\n", 1, 1, log->log_file_fd);
    }

    fflush(log->log_file_fd);

	g_static_mutex_unlock(&log_mutex);
}

xml_server_thread_pool_t     global_thread_pool;

int
xml_server_thread_pool_init(
    guint               thread_cnt
)
{
    guint       i;

    global_thread_pool.mutex = g_mutex_new();

    global_thread_pool.work_thread_arr = g_ptr_array_new();

    for (i = 0; i < thread_cnt; ++i)
    {
        if (xml_server_new_worker_thread() < 0)
        {
            g_error("%s, xml_sever_new_worker_thread() error", G_STRLOC);
        }
    }

    return 0;
}

void
xml_server_thread_pool_deinit()
{
#ifdef _DEBUG
    g_message("waiting task queue empty\n");
#endif // _DEBUG

    while(1)
    {
        if (tsk_queue_is_empty() == TRUE)
        {
            break;
        }

        g_usleep(100); //
    }

#ifdef _DEBUG
    g_message("waiting all worker thread to exit\n");
#endif // _DEBUG

    tsk_wake_all_thread_to_exit();

    while(1)
    {
        GThread*        work_thread;

        if (global_thread_pool.work_thread_arr->len == 0)
            break;

        work_thread = g_ptr_array_remove_index(global_thread_pool.work_thread_arr, 0);

        g_thread_join(work_thread);
    }

#ifdef _DEBUG
    g_message("exit!\n");
#endif // _DEBUG

    g_ptr_array_free(global_thread_pool.work_thread_arr, TRUE);

    g_mutex_free(global_thread_pool.mutex);
}

#define XML_SERVER_WORKER_STACK_SIZE (1 << 20)

int
xml_server_new_worker_thread()
{
    GThread*            work_thread;
    GError*             gerr = NULL;

    work_thread = g_thread_create_full(xml_server_worker_thread, NULL, XML_SERVER_WORKER_STACK_SIZE, TRUE, FALSE, G_THREAD_PRIORITY_NORMAL, &gerr);
    if (gerr != NULL)
    {
        g_critical("%s: %s", G_STRLOC, gerr->message);
        g_error_free(gerr);
        gerr = NULL;
        return -1;
    }

    g_mutex_lock(global_thread_pool.mutex);
    g_ptr_array_add(global_thread_pool.work_thread_arr, work_thread);
    g_mutex_unlock(global_thread_pool.mutex);

    return 0;
}

int
xml_server_init()
{
    //init glib and winsocket

    const gchar *check_str = NULL;

#ifdef _WIN32
    {
        //Initialize Winsock
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (iResult != NO_ERROR)
            printf("Error at WSAStartup()\n");

        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }
#endif // _WIN32

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

    xml_server_global_init();

    xml_server_log_init();

#ifndef _WIN32
    (void) signal(SIGPIPE, SIG_IGN);
#endif

    return 0;
}

network_socket_t*
xml_server_new_listen_socket(
    gchar*          ip_addr,
    guint           port
)
{
    gint                fail_flag = 0;
    network_address_t*  listen_addr = NULL;
    network_socket_t*   listen_socket = NULL;
#ifdef WIN32
    char val = 1;	/* Win32 setsockopt wants a const char* instead of the UNIX void*...*/
#else
    int val = 1;
#endif

    if (port > 65535)
    {
        g_critical("%s: sever port is invalid: (%d)", 
            G_STRLOC,
            port);
        
        return NULL;
    }

    listen_socket        = network_socket_new();
    listen_socket->socket_type = SOCK_STREAM;

    switch (1)
    {
    case 1:
        listen_socket->fd = socket(AF_INET, listen_socket->socket_type, IPPROTO_TCP);
        if (listen_socket->fd == -1) 
        {
            g_critical("%s: socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) failed: %s (%d)", 
                G_STRLOC,
                g_strerror(errno), errno);

            fail_flag = 1;
            break;
        }

#ifndef _WIN32
        /*
          We should not use SO_REUSEADDR on windows as this would enable a
          user to open two servers with the same TCP/IP port.
        */
        if (0 != setsockopt(listen_socket->fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))) 
        {
            g_critical("%s: setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR) failed: %s (%d)", 
                G_STRLOC,
                g_strerror(errno), errno);
            return NULL;
        }
#endif
        listen_addr = network_address_new();
        listen_socket->addr = listen_addr;
        listen_socket->addr_type = SOCKET_ADDR_TYPE_LOCAL;
        if (network_address_set_address_ip(listen_addr, ip_addr, port))
        {
            g_critical("%s: sever address is invalid: (%s:%d)", 
                G_STRLOC,
                ip_addr,
                port);

            return NULL;
        }

        if (bind(listen_socket->fd, 
            (struct sockaddr*) &listen_addr->addr.ipv4, 
            listen_addr->len) == -1) 
        {
            g_critical("%s: bind failed: %s (%d)", 
                G_STRLOC,
                g_strerror(errno), errno);

            fail_flag = 1;

            break;
        }

        //----------------------
        // Listen for incoming connection requests 
        // on the created socket
        if (listen(listen_socket->fd, SOMAXCONN) == -1)
        {
            g_critical("%s: listen failed: %s (%d)", 
                G_STRLOC,
                g_strerror(errno), errno);

            fail_flag = 1;

            break;
        }
    }

    if (fail_flag == 1)
    {  
        network_socket_free(listen_socket);
        return NULL;
    }

    return listen_socket;
}

int
xml_server_write_to_file(
    gchar*                  filename,
    char*                   buf,
    guint                   len
)
{
    FILE*           f;

#ifdef _LINUX_TEST
    return 0;
#endif // _LINUX_TEST
    
    f = fopen(filename, "a+");
    if (f == NULL)
    {
        g_critical("%s: fopen(%s) failed: %s (%d)", 
            G_STRLOC,
            filename,
            g_strerror(errno), errno);

        return -1;
    }

    if (fwrite(buf, len, 1, f) < 0)
    {
        g_critical("%s: fwrite(%s) failed: buf length is %d %s:(%d)", 
            G_STRLOC,
            filename,
            len,
            g_strerror(errno), errno);

        return -1;
    }

    fclose(f);

    return 0;
}

#define XML_TRANSFER_END "XML_TRANSFER_END"

#define XML_BUF_SIZE    65535

#define XML_WRITE_SIZE  (1 << 20)

#ifdef _WIN32
#define FN_LIBCHAR '\\'
#else
#define FN_LIBCHAR '/'
#endif

/* 返回非0表示失败 */
int my_GetModuleFileName( char* sFileName, int nSize)
{
#ifdef _WIN32
    return GetModuleFileName(NULL, sFileName, (DWORD)nSize) == 0;
#else
    int ret = -1;
    char sLine[1024] = { 0 };
    void* pSymbol = (void*)"";
    FILE *fp;
    char *pPath;


    fp = fopen ("/proc/self/maps", "r");
    if ( fp != NULL )
    {
        while (!feof (fp))
        {
            unsigned long start, end;

            if ( !fgets (sLine, sizeof (sLine), fp))
                continue;
            if ( !strstr (sLine, " r-xp ") || !strchr (sLine, '/'))
                continue;

            /* 利用存储地址判断是否同一进程 */
            sscanf (sLine, "%lx-%lx ", &start, &end);
            if (pSymbol >= (void *) start && pSymbol < (void *) end)
            {
                char *tmp;
                size_t len;

                /* Extract the filename; it is always an absolute path */
                pPath = strchr (sLine, '/');

                /* Get rid of the newline */
                tmp = strrchr (pPath, '\n');
                if (tmp) *tmp = 0;

                /* Get rid of "(deleted)" */
                //len = strlen (pPath);
                //if (len > 10 && strcmp (pPath + len - 10, " (deleted)") == 0)
                //{
                // tmp = pPath + len - 10;
                // *tmp = 0;
                //}
                ret = 0;
                strcpy( sFileName, pPath );
            }
        }
        fclose (fp);

    }
    return ret;
#endif
}


int
xml_server_work(
    void*       user_arg                         
)
{
    network_socket_t*       socket;
    gchar                   buf[XML_BUF_SIZE + 1];
    guint                   len;
    gchar*                  ret = NULL;
    guint                   ret_size = 0;
    guint                   ret_len = 0;
    gint                    has_end = 0;
    guint                   end_len = strlen(XML_TRANSFER_END);
    gchar                   tmp_filename[100];
    gchar                   filename[100];

    socket  = (network_socket_t*)user_arg;

    sprintf(tmp_filename, "%s/xml_tmp/%s_%d.log", g_exe_path, socket->addr->name->str, (unsigned long)time(NULL));
    sprintf(filename, "%s/xml_tmp/%s_%d.log", g_exe_parent_path, socket->addr->name->str, (unsigned long)time(NULL));

    ret_size = XML_WRITE_SIZE;
    ret = g_new(gchar, ret_size + 1);
    if (ret == NULL)
    {
        g_critical("%s, g_new error \n", G_STRLOC);

        goto fail_flag;
    }

    while(1)
    {
        /* 1次读入16k */
        len = recv(socket->fd, buf, XML_BUF_SIZE, 0);
        if (len <= 0)
            break;

        buf[len] = '\0';
        //消息拼在一起

        if (ret_size - ret_len < len)
        {
            /* 每读到1M写一次临时文件 */
            if (xml_server_write_to_file(tmp_filename, ret, ret_len) < 0)
            {
                g_free(ret);
                goto fail_flag;
            }

            ret_len = 0; 
        }

        memcpy(&ret[ret_len], buf, len);

        ret_len += len;
        ret[ret_len] = '\0';
    }

    if (ret_len > 0)
    {
        if (strcmp(&ret[ret_len - end_len], XML_TRANSFER_END) == 0)
        {
            ret_len -= end_len;
            ret[ret_len] = '\0';
        }
        else
        {
            g_critical("%s, client message should has XML_TRANSFER_END, but has not \n", G_STRLOC);
            
            g_free(ret);
            goto fail_flag;
        }

        if (xml_server_write_to_file(tmp_filename, ret, ret_len) < 0)
        {
            g_free(ret);
            goto fail_flag;
        }

#ifndef _LINUX_TEST
        if (rename(tmp_filename, filename) < 0) 
        {
            g_free(ret);
            goto fail_flag;
        }
#endif // _LINUX_TEST
    }

    network_socket_write(socket, "XML_RECEIVE_SUCCESS", sizeof("XML_RECEIVE_SUCCESS"));

    network_socket_free(socket);

    g_free(ret);

    return 0;

fail_flag:
    network_socket_write(socket, "XML_RECEIVE_FAIL", sizeof("XML_RECEIVE_FAIL"));

    network_socket_free(socket);

    return -1;
}

gpointer
xml_server_worker_thread(
    void*       user_arg
)
{
    network_socket_t*       socket;
    while(1)
    {
        if (global_info.exit_flag)
            break;

        socket = (network_socket_t*)tsk_dequeue();

        //should exit
        if (socket == NULL)
            continue;
        
        tsk_n_idle_dec();
        xml_server_work(socket);
        tsk_n_idle_inc();

        //如果user_arg不为null，表示负载高产生的新线程，处理完线程即可以退出
        if (user_arg != NULL)
            break;
    }

    return NULL;
}

extern tsk_sys_t global_tsk_sys;

gpointer
xml_server_listen_thread(
    void*       user_arg
)
{
    network_socket_t*   listen_socket;
    gint                fail_flag = 0;
    network_address_t*  accept_addr;
    GThread*            gthread = NULL;
    guint               i = 0;

    listen_socket     = (network_socket_t*)user_arg;

    while (1)
    {
        network_socket_t*       accept_socket;

        if (global_info.exit_flag == 1)
        {
            break;
        }

        accept_socket = network_socket_new();
        accept_addr = network_address_new();
        accept_socket->addr         = accept_addr;
        accept_socket->socket_type  = SOCK_STREAM;
        accept_socket->addr_type    = SOCKET_ADDR_TYPE_REMOTE;

        if (-1 == (accept_socket->fd = accept(listen_socket->fd, &accept_addr->addr.common, &(accept_addr->len))))
        {
            if (global_info.exit_flag != 1)
            {
                g_critical("%s: accept failed: %s (%d)", 
                    G_STRLOC,
                    g_strerror(errno), errno);
            }

            network_socket_free(accept_socket);
        }
        else
        {
            if (i ++ == 1000)
            {
                g_critical("listen thread log, thread count : %d, n_idle_thread %d, tsk count %d, \n",
                            global_thread_pool.work_thread_arr->len, global_tsk_sys.n_idle_work_thread, global_tsk_sys.tsk_arr->len);
                i = 0;
            }

            network_address_refresh_name(accept_socket->addr);

            //添加到任务队列
            tsk_enqueue(accept_socket);
        }
    }

#ifdef _WIN32
    g_assert(global_info.exit_flag == 1);
#endif // _WIN32

    network_socket_free(listen_socket);

    return NULL;
}

void
xml_sever_cleanup()
{
    global_info.exit_flag = 1;

    tsk_sys_deinit();

    xml_server_global_deinit();

    xml_server_log_deinit();

#ifdef _WIN32
    WSACleanup();
#endif // _WIN32
    
}

int 
main(
    int         argc, 
    char        **argv
)
{
    gchar               buf[1024];
    GError*             gerr = NULL;
    GThread*            listen_thread;
    network_socket_t*   listen_socket = NULL;
    //gint                i;

    /* 获得进程所在位置 */
    if (!my_GetModuleFileName(g_exe_path, 1024))
    {
        //exe_name
        char *last= NULL;
        char *end;

        strcpy(g_exe_parent_path, g_exe_path);

        *(strrchr(g_exe_path, FN_LIBCHAR)) = 0;


        end= g_exe_parent_path + strlen(g_exe_parent_path) - 1;

        /*
        Look for the second-to-last \ in the filename, but hang on
        to a pointer after the last \ in case we're in the root of
        a drive.
        */
        for ( ; end > g_exe_parent_path; end--)
        {
            if (*end == FN_LIBCHAR)
            {
                if (last)
                {
                    /* Keep the last '\' as this works both with D:\ and a directory */
                    end[0]= 0;
                    break;
                }
                last= end;
            }
        }

    }
    else
    {
        g_error("%s, can't get module file name", G_STRLOC);
        
        strcpy(g_exe_path, "./");
        strcpy(g_exe_parent_path, "../");
    }

    fprintf(stdout, "xml_server start dir %s, parent dir %s\n", g_exe_path, g_exe_parent_path);

    if (xml_server_init() == -1)
    {
        exit(-1);
    }

    /*** 读取配置信息 ***/
    if (argc == 2 &&
        (strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0))
    {
        printf("%s", help_str);
        exit(-1);
    }

    if (xml_server_parse_options(&argc, &argv))
    {
        exit(-1);
    }

#ifndef _WIN32
    if (global_info.keepalive)
    {
        int child_exit_status = EXIT_SUCCESS; /* forward the exit-status of the child */
        int ret = unix_proc_keepalive(&child_exit_status);

        if (ret > 0) 
        {
            /* the agent stopped */
            exit(child_exit_status);
        } 
        else if (ret < 0) {
            exit(EXIT_FAILURE);
        } 
        else {
            /* we are the child, go on */
        }
    }
#endif // _WIN32

    /*** 初始化任务系统 ***/
    tsk_sys_init(global_info.event_thread_cnt);

    /*** 启动监听线程 ***/
    if (NULL != (listen_socket = xml_server_new_listen_socket(global_info.ip_addr, global_info.port)))
    {
#ifdef _WIN32
        int         sock_tmp;
        /* Create listen thread */
        listen_thread = g_thread_create((GThreadFunc)xml_server_listen_thread, listen_socket, TRUE, &gerr);
        if (gerr) 
        {
            g_critical("%s: %s", G_STRLOC, gerr->message);
            g_error_free(gerr);
            gerr = NULL;
        }
        g_thread_set_priority(listen_thread, G_THREAD_PRIORITY_HIGH);

        /*** 终端接收信息 ***/
        while (1)
        {
            guint       buf_len;
            fgets(buf, 1023, stdin);

            //去掉最后的\n
            buf_len = strlen(buf);
            if (buf[buf_len - 1] == '\n')
                buf[buf_len - 1] = '\0';

            if (g_strcasecmp(buf, "exit") == 0)
            {
                global_info.exit_flag = 1;
                g_message("xml server is exiting...\n");
                break;
            }
            else
            {
                g_message("input exit to quit\n");
            }
        }

        //监听线程可能正在accept阻塞着
        sock_tmp = listen_socket->fd;
        listen_socket->fd = -1;
        shutdown(sock_tmp, SHUT_RDWR);
        closesocket(sock_tmp);

        if (listen_thread)
        {
            //wait until listen_thread over!
            g_thread_join(listen_thread);
        }
#else
        xml_server_listen_thread(listen_socket);
#endif // _WIN32
    }

    /*** 清理操作，并等待工作线程结束 ****/
    xml_sever_cleanup();

#ifdef _WIN32
    _CrtDumpMemoryLeaks();
#endif // _WIN32
    

    return 0;
}
