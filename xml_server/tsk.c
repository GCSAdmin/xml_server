#include "tsk.h"
#include "stdlib.h"
#include "ossema.h"
#include "xml_server.h"

tsk_sys_t global_tsk_sys;

void
tsk_sys_init(
    int         n_workers
)
{
    int         real_n_workers;

    global_tsk_sys.mutex = g_mutex_new();
    if (os_semaphore_create(&global_tsk_sys.sema) == FALSE)
        exit(-1);

    global_tsk_sys.n_work_thread_max   = n_workers;

    real_n_workers = n_workers / 10;
    if (real_n_workers < 10)
        real_n_workers = (n_workers > 10) ? 10 : n_workers;
    
    global_tsk_sys.n_idle_work_thread  = real_n_workers;
    global_tsk_sys.n_work_thread       = real_n_workers;
    global_tsk_sys.tsk_arr             = g_ptr_array_new();

    xml_server_thread_pool_init(global_tsk_sys.n_work_thread);
}

void
tsk_sys_deinit()
{
    xml_server_thread_pool_deinit();

    g_mutex_free(global_tsk_sys.mutex);
    os_semaphore_free(&global_tsk_sys.sema);
    //to do
    g_ptr_array_free(global_tsk_sys.tsk_arr, TRUE);
}

void
tsk_enter()
{
    g_mutex_lock(global_tsk_sys.mutex);
}

void
tsk_leave()
{
    g_mutex_unlock(global_tsk_sys.mutex);
}

void
tsk_enqueue(
    void*               tsk_arg
)
{

    tsk_enter();

    g_ptr_array_add(global_tsk_sys.tsk_arr, tsk_arg);

    if (global_tsk_sys.n_idle_work_thread == 0 &&
        global_tsk_sys.n_work_thread < global_tsk_sys.n_work_thread_max)
    {
        if (xml_server_new_worker_thread() < 0)
        {
            g_error("%s : xml_server_new_worker_thread error\n");
        }
        global_tsk_sys.n_work_thread ++;
        //g_assert(global_tsk_sys.n_work_thread == )
        tsk_n_idle_inc();
    }

    os_semaphore_v(&global_tsk_sys.sema);

    tsk_leave();
}

gboolean
tsk_queue_is_empty()
{
    return global_tsk_sys.tsk_arr->len == 0;
}

void
tsk_wake_all_thread_to_exit()
{
    guint i;
    g_assert(tsk_queue_is_empty());

    for (i = 0; i < global_tsk_sys.n_work_thread; ++i)
    {
        os_semaphore_v(&global_tsk_sys.sema);
    }
}

void*
tsk_dequeue()
{
    void*       tsk_arg;

    os_semaphore_p(&global_tsk_sys.sema);

    g_assert(global_tsk_sys.n_idle_work_thread > 0);

    if (tsk_queue_is_empty())
    {
        return NULL;
    }
    
    tsk_enter();
    g_assert(global_tsk_sys.tsk_arr->len > 0);

    tsk_arg = g_ptr_array_remove_index(global_tsk_sys.tsk_arr, 0);

    tsk_leave();

    return tsk_arg;
}

gint
tsk_n_idle_inc()
{
//     tsk_enter();
// 
//     global_tsk_sys.n_idle_work_thread++;
// 
//     tsk_leave();
    gint        i;  

    i = g_atomic_int_exchange_and_add(&global_tsk_sys.n_idle_work_thread, 1);

    return i + 1;
}

int
tsk_n_idle_dec()
{
    gint        i;

    i = g_atomic_int_exchange_and_add(&global_tsk_sys.n_idle_work_thread, -1);

    return i - 1;
}



