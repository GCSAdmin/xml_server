/********************************************************************
file:
    ossema.h
purpose:
    semaphores operaions
interface:
    os_semaphore_create
	os_semaphore_free
	os_semaphore_p
	os_semaphore_v
	os_semaphore_set_value
	os_semaphore_get_value
based module:
    None
history
    Date        Who         RefDoc       Memo
    2002-03-30  Joe Han                  Created
***********************************************************************/
#ifndef ossema_h
#define ossema_h


#ifndef WIN32
#ifndef VITUAL_SEMA
#define VITUAL_SEMA
#endif
#include <semaphore.h>
#else
#include <windows.h>
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* user need not know the detail of this structure */
typedef struct os_semaphore_struct os_semaphore_t;
/**<p> os_semaphore_struct: 信号结构 </p>*/
struct os_semaphore_struct {     
#ifdef WIN32
    HANDLE          sema;		/** sema: 信号灯对像句柄 <br>*/
#else
#ifndef VITUAL_SEMA
	sem_t			sema;
#else
#ifdef SOLARIS
	mutex_t			mutex;
	cond_t			cond;
#else
	pthread_mutex_t mutex;
    pthread_cond_t  cond;
#endif /* SOLARIS */
	int v;
#endif /* VITUAL_SEMA */
#endif /* WIN32 */
};
 
/******************************************************************
name:
    os_semaphore_create
purpose:
    create semaphore; the semaphore value is set to 1
return:
    TRUE if succeed else FALSE
*******************************************************************/
int
os_semaphore_create(
    os_semaphore_t*    os_semaphore     /*out: created semaphore */
);

/******************************************************************
name:
    os_semaphore_free
purpose:
    free it
return:
    TRUE if succeed else FALSE
*******************************************************************/
int
os_semaphore_free(
    os_semaphore_t*    os_semaphore     /*in: semaphore to free */
);


/******************************************************************
name:
    os_semaphore_p
purpose:
    a "P" operation 
return:
    TRUE if succeed else FALSE
*******************************************************************/
int
os_semaphore_p(
    os_semaphore_t*    os_semaphore     /*in: */
);


/******************************************************************
name:
    os_semaphore_v
purpose:
    a "V" operation 
return:
    TRUE if succeed else FALSE
*******************************************************************/
int
os_semaphore_v(
    os_semaphore_t*    os_semaphore     /*in: */
);


#endif


