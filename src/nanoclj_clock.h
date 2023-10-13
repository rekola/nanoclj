#ifndef _NANOCLJ_CLOCK_H_
#define _NANOCLJ_CLOCK_H_

#include <stdint.h>

#ifdef _WIN32

#include <Windows.h>

static inline uint64_t system_time(){
  LARGE_INTEGER time, freq;
  if (!QueryPerformanceFrequency(&freq)){
    return 0;
  }
  if (!QueryPerformanceCounter(&time)){
    return 0;
  }
  return 1000000ll * time.QuadPart / freq.QuadPart;
}

//  Posix/Linux
#else

#include <sys/time.h>
#include <time.h>

static inline uint64_t system_time(){
  struct timeval time;
  if (gettimeofday(&time,NULL)){
    //  Handle error
    return 0;
  }
  return 1000000ll * time.tv_sec + time.tv_usec;
}

#endif
#endif
