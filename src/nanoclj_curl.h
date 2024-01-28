#ifndef _NANOCLJ_CURL_H_
#define _NANOCLJ_CURL_H_

#define NANOCLJ_HAS_HTTP 1

#include <curl/curl.h>

static inline void free_uri(nanoclj_uri_t * uri) {
  curl_free(uri->scheme);
  curl_free(uri->host);
  curl_free(uri->path);
}

static inline nanoclj_uri_t parse_uri(strview_t input) {
  CURLUcode uc;
  nanoclj_uri_t uri = (nanoclj_uri_t){0};
  CURL * h = curl_url(); /* get a handle to work with */
  if (!h) return uri;

  char * tmp = alloc_c_str(input);

  uc = curl_url_set(h, CURLUPART_URL, tmp, 0);
  free(tmp);
  if (uc) {
    curl_url_cleanup(h);
    return uri;
  }
  
  curl_url_get(h, CURLUPART_SCHEME, &(uri.scheme), 0);
  curl_url_get(h, CURLUPART_HOST, &(uri.host), 0);
  curl_url_get(h, CURLUPART_PATH, &(uri.path), 0);

  char * port;
  if (!curl_url_get(h, CURLUPART_PORT, &port, 0)) {
    uri.port = atoi(port);
    curl_free(port);
  } else {
    uri.port = -1;
  }
  
  curl_url_cleanup(h);
  return uri;
}

static size_t curl_write_data_func(void * buffer, size_t size, size_t nmemb, void * userp) {
  size_t s = size * nmemb;
  http_load_t * context = (http_load_t *)userp;
  return write(context->fd, buffer, s);
}

static NANOCLJ_THREAD_SIG http_load(void *ptr) {
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
    if (d->http_max_connections) {
      curl_easy_setopt(curl, CURLOPT_MAXCONNECTS, d->http_max_connections);
    }
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, d->http_max_redirects);
    curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, d->http_keepalive ? 0 : 1);
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, d->http_keepalive ? 0 : 1);

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
