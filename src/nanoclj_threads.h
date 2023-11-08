#ifndef _NANOCLJ_THREADS_H_
#define _NANOCLJ_THREADS_H_

#ifdef _WIN32

#include <Windows.h>
#include <process.h>
#include <synchapi.h>

typedef HANDLE nanoclj_mutex_t;

#define NANOCLJ_THREAD_SIG unsigned __stdcall

static inline void nanoclj_mutex_lock(nanoclj_mutex_t * m) {
  WaitForSingleObject(*m, INFINITE);
}

static inline void nanoclj_mutex_unlock(nanoclj_mutex_t * m) {
  ReleaseMutex(*m);
}

static inline nanoclj_mutex_t nanoclj_mutex_create() {
  return CreateMutexA(NULL, TRUE, NULL);  
}

static inline void nanoclj_mutex_destroy(nanoclj_mutex_t * m) {
  CloseHandle(*m);  
}

static inline void nanoclj_start_thread(unsigned (__stdcall *start_routine)(void *), void * arg) {
  unsigned threadID;
  uintptr_t h = _beginthreadex( NULL, 0, start_routine, arg, 0, &threadID );
  CloseHandle(h);
}

static inline void nanoclj_sleep(long long ms) {
  Sleep(ms);
}

#else

#include <pthread.h>
#include <stdlib.h>

typedef pthread_mutex_t nanoclj_mutex_t;
typedef pthread_t nanoclj_thread_t;

#define NANOCLJ_THREAD_SIG void *

static inline void nanoclj_mutex_lock(nanoclj_mutex_t * m) {
  pthread_mutex_lock(m);
}

static inline void nanoclj_mutex_unlock(nanoclj_mutex_t * m) {
  pthread_mutex_unlock(m);
}

static inline nanoclj_mutex_t nanoclj_mutex_create() {
  pthread_mutex_t m;
  pthread_mutex_init(&m, NULL);
  return m;
}

static inline void nanoclj_mutex_destroy(nanoclj_mutex_t * m) {
  pthread_mutex_destroy(m);
}

static inline void nanoclj_start_thread(NANOCLJ_THREAD_SIG (*start_routine)(void *), void * arg) {
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&thread, &attr, start_routine, arg);
}

static inline void nanoclj_sleep(long long ms) {
  struct timespec t, t2;
  t.tv_sec = ms / 1000;
  t.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&t, &t2);
}

#endif

#endif
