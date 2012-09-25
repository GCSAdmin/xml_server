#ifndef _TSK_H__
#define _TSK_H__

#include <glib.h>
#include "ossema.h"

typedef struct tsk_sys_struct   tsk_sys_t;


struct tsk_sys_struct
{
    GMutex*         mutex;
    guint           n_idle_work_thread;
    guint           n_work_thread;
    guint           n_work_thread_max;
    
    os_semaphore_t  sema;

    GPtrArray*      tsk_arr;
};

void
tsk_sys_init(
    int         n_workers
);

void
tsk_sys_deinit();

void
tsk_enqueue(
    void*               tsk_arg
);

void*
tsk_dequeue();

gint
tsk_n_idle_inc();

gint
tsk_n_idle_dec();

gboolean
tsk_queue_is_empty();

void
tsk_wake_all_thread_to_exit();



#endif

