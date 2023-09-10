#ifndef _NANOCLJ_CURL_H_
#define _NANOCLJ_CURL_H_

#include <curl/curl.h>

static size_t curl_write_data_func(void * buffer, size_t size, size_t nmemb, void * userp) {
  size_t s = size * nmemb;
  http_load_t * context = (http_load_t *)userp;
  return write(context->fd, buffer, s);
}

static inline void * http_load(void *ptr) {
  http_load_t * d = (http_load_t*)ptr;
  CURL *curl = curl_easy_init();
  
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, d->url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, d->useragent);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, d);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_func);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    
    curl_easy_cleanup(curl);
  }
  
  free(d->url);
  free(d->useragent);
  close(d->fd);
  free(d);
  
  return 0;
}

static inline void http_init() {
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

#endif
