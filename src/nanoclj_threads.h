#ifndef _NANOCLJ_THREADS_H_
#define _NANOCLJ_THREADS_H_

#ifdef _WIN32

#include <process.h>

typedef int nanoclj_mutex_t;
typedef int nanoclj_thread_t;

static inline void nanoclj_mutex_lock(nanoclj_mutex_t * m) {
}

static inline void nanoclj_mutex_unlock(nanoclj_mutex_t * m) {
}

static inline void nanoclj_mutex_init(nanoclj_mutex_t * m) {
}

static nanoclj_thread_t * nanoclj_start_thread(void *(*start_routine)(void *), void * arg) {
  return NULL;
}

#else

#include <pthread.h>
#include <stdlib.h>

typedef pthread_mutex_t nanoclj_mutex_t;
typedef pthread_t nanoclj_thread_t;

static inline void nanoclj_mutex_lock(nanoclj_mutex_t * m) {
  pthread_mutex_lock(m);
}

static inline void nanoclj_mutex_unlock(nanoclj_mutex_t * m) {
  pthread_mutex_unlock(m);
}

static inline void nanoclj_mutex_init(nanoclj_mutex_t * m) {
  pthread_mutex_init(m, 0);
}

static nanoclj_thread_t * nanoclj_start_thread(void *(*start_routine)(void *), void * arg) {
  pthread_t * thread = malloc(sizeof(pthread_t));
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(thread, &attr, start_routine, arg);
  return thread;
}
 
#endif

#endif
