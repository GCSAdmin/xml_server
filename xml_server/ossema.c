#include <errno.h>
// #include "dm.h"
// #include "os_pub.h"
#include "ossema.h"
#include <stdlib.h>


/**
 * <p>功能说明：创建信号量，windows初始计数为0，最大计数为10000，其他以后分析</p>
 * <p>算法描述：无</p>
 * <p>参数列表：<br><ul>
 * <li>os_semaphore_t* os_semaphore：输入输出参数，指向该函数创建的信号量的指针，不可能为空</li>
 * </ul></p>
 * <p>返回值  ：创建成功返回TRUE，否则返回FALSE</p>
 */
int
os_semaphore_create(
    os_semaphore_t*    os_semaphore     /*out: created semaphore */
)
{
#ifdef WIN32
    os_semaphore->sema  = CreateSemaphore(NULL, 0, 10000, NULL);

   if (os_semaphore->sema == NULL)
       return FALSE;
   else
       return TRUE;

#else
#ifndef VITUAL_SEMA
	if (-1 == sem_init(&os_semaphore->sema, 0, 0))
	{
		exit(-1)
	}
#else    
   os_semaphore->v = 0;

#ifdef SOLARIS 
	if (-1 == mutex_init(&(os_semaphore->mutex), USYNC_THREAD | LOCK_ERRORCHECK, NULL))
#else
	if (-1 == pthread_mutex_init(&(os_semaphore->mutex), NULL))
#endif
		exit(-1);
 
#ifdef SOLARIS
	if (-1 == cond_init(&(os_semaphore->cond), USYNC_THREAD, NULL))
#else
	if (-1 == pthread_cond_init(&(os_semaphore->cond), NULL))
#endif
		exit(-1);
#endif
	return TRUE;
#endif
}

/**
 * <p>功能说明：释放信号量</p>
 * <p>算法描述：无</p>
 * <p>参数列表：<br><ul>
 * <li>os_semaphore_t* os_semaphore：输入参数，指向信号量的指针，不可能为空</li>
 * </ul></p>
 * <p>返回值  ：释放成功返回TRUE，否则返回FALSE</p>
 */
int
os_semaphore_free(
    os_semaphore_t*    os_semaphore     /*in: semaphore to free */
)
{

#ifdef WIN32
    if (!CloseHandle(os_semaphore->sema))
		exit(-1);
        
	return TRUE;
#else
#ifndef VITUAL_SEMA
	while (-1 == sem_destroy(&(os_semaphore->sema)))
	{
		if (errno != EBUSY)
		{
			exit(-1);
		}
		os_thread_sleep(1);
	}
#else
#ifdef SOLARIS
	if (-1 == mutex_destroy(&(os_semaphore->mutex)))
#else
	if (-1 == pthread_mutex_destroy(&(os_semaphore->mutex)))
#endif
        //return FALSE;
		exit(-1);

#ifdef SOLARIS
	if (-1 == cond_destroy(&(os_semaphore->cond)))
#else
	if (-1 == pthread_cond_destroy(&(os_semaphore->cond)))
#endif
        //return FALSE;
		exit(-1);
#endif
	return TRUE;
#endif

};


/**
 * <p>功能说明：信号量的p操作</p>
 * <p>算法描述：无</p>
 * <p>参数列表：<br><ul>
 * <li>os_semaphore_t* os_semaphore：输入参数，指向信号量的指针，不可能为空</li>
 * </ul></p>
 * <p>返回值  ：操作成功返回TRUE，否则返回FALSE</p>
 */
int
os_semaphore_p(
    os_semaphore_t*    os_semaphore     /*in: */
)
{
#ifdef WIN32
    DWORD ret;

    ret = WaitForSingleObject(os_semaphore->sema, INFINITE);
    if (ret ==  WAIT_FAILED)
		exit(-1);
    
    return TRUE;
#else
#ifndef VITUAL_SEMA
	if (-1 == sem_wait(&os_semaphore->sema))
	{
		exit(-1);
	}
#else
#ifdef SOLARIS
	if (-1 == mutex_lock(&(os_semaphore->mutex)))
#else
	if (-1 == pthread_mutex_lock(&(os_semaphore->mutex)))
#endif
        //return FALSE;
		exit(-1);

    while (os_semaphore->v <= 0)
    {
#ifdef SOLARIS
		if (-1 == cond_wait(&(os_semaphore->cond), &(os_semaphore->mutex)))
#else
		if (-1 == pthread_cond_wait(&(os_semaphore->cond), &(os_semaphore->mutex)))
#endif
          //return FALSE;
		  exit(-1);
    }
    
    (os_semaphore->v)--;

    
#ifdef SOLARIS
	if (-1 == mutex_unlock(&(os_semaphore->mutex)))
#else
	if (-1 == pthread_mutex_unlock(&(os_semaphore->mutex)))
#endif
        //return FALSE;
		exit(-1);
#endif
	return TRUE;
#endif

}


/**
 * <p>功能说明：信号量的v操作</p>
 * <p>算法描述：无</p>
 * <p>参数列表：<br><ul>
 * <li>os_semaphore_t* os_semaphore：输入参数，指向信号量的指针，不可能为空</li>
 * </ul></p>
 * <p>返回值  操作成功返回TRUE，否则返回FALSE</p>
 */
int
os_semaphore_v(
    os_semaphore_t*    os_semaphore     /*in: */
)
{
#ifdef WIN32
    if (!ReleaseSemaphore(os_semaphore->sema, 1, NULL))
		exit(-1);
        
	return TRUE;
#else
#ifndef VITUAL_SEMA
	if (-1 == sem_post(&os_semaphore->sema))
	{
		//perror("sem_post error!");
		//return FALSE;
		exit(-1);
	}
#else
#ifdef SOLARIS
	if (-1 == mutex_lock(&(os_semaphore->mutex)))
#else
	if (-1 == pthread_mutex_lock(&(os_semaphore->mutex)))
#endif
        //return FALSE;
		exit(-1);

    (os_semaphore->v)++;
    
#ifdef SOLARIS
	if (-1 == mutex_unlock(&(os_semaphore->mutex)))
#else
	if (-1 == pthread_mutex_unlock(&(os_semaphore->mutex)))
#endif
        //return FALSE;
		exit(-1);

#ifdef SOLARIS
	if (-1 == cond_signal(&(os_semaphore->cond)))
#else
	if (-1 == pthread_cond_signal(&(os_semaphore->cond)))
#endif
        //return FALSE;
		exit(-1);
#endif
	return TRUE;
#endif
}

