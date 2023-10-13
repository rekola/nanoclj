cmake_minimum_required(VERSION 3.4)
project(nanoclj C)

find_package(Threads REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_definitions(-DNANOCLJ_STANDALONE -DNANOCLJ_USE_LINENOISE)

if (NOT MSVC)
  # The math optimizations are not necessary, but they do not cause harm. Since NaNs are needed -ffast-math or -ffinite-math-only cannot be used.
  add_compile_options(-std=gnu99 -Wall -O3 -march=native -funsafe-math-optimizations -fno-math-errno -fno-rounding-math -fno-signaling-nans -fcx-limited-range -ffp-contract=fast -Wno-unused-value -Wno-unused-variable -Wno-unused-function -Werror=implicit-function-declaration -Wno-unused-but-set-variable -Werror=return-type)

  if(DEFINED ENV{WARN_ERROR_BUILD})
    add_compile_options(-Werror)
  endif()
endif()

# Data types, macros and functions related to controlling CPU affinity
# are available on Linux through GNU extensions in libc
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_compile_definitions(_GNU_SOURCE)
endif()
  
set(SOURCE_FILES
  src/nanoclj.c
  src/linenoise.c
  src/wcwidth.c
  src/ggml.c
  src/ggml-alloc.c
  )

add_executable(nanoclj ${SOURCE_FILES})
target_link_libraries(nanoclj Threads::Threads)

IF (WIN32)
  find_package(unofficial-utf8proc CONFIG REQUIRED)
  target_link_libraries(main PRIVATE utf8proc)
else()
  find_library(UTF8PROC_LIBRARY utf8proc REQUIRED)
  if(UTF8PROC_LIBRARY)
    target_link_libraries(nanoclj utf8proc)
  endif()

  target_link_libraries(nanoclj m)

  find_library(CAIRO_LIBRARY cairo REQUIRED)
  if(CAIRO_LIBRARY)
    target_link_libraries(nanoclj cairo)
  endif()

  find_library(SIXEL_LIBRARY sixel REQUIRED)
  if(SIXEL_LIBRARY) 
    target_link_libraries(nanoclj sixel)
    find_path(SIXEL_INCLUDE_DIR NAMES sixel.h PATHS "/usr/include" "/usr/include/sixel")
    target_include_directories(nanoclj PRIVATE ${SIXEL_INCLUDE_DIR})
  endif()

  find_library(SHP_LIBRARY shp REQUIRED)
  if(SHP_LIBRARY)
    target_link_libraries(nanoclj shp)
  endif()
endif()

find_library(PCRE2_LIBRARY pcre2-8 REQUIRED)
if(PCRE2_LIBRARY)
  target_link_libraries(nanoclj pcre2-8)
endif()

FIND_PACKAGE(CURL REQUIRED)
if(CURL_FOUND)
  include_directories(${CURL_INCLUDE_DIR})
  target_link_libraries(nanoclj ${CURL_LIBRARIES})
endif()

configure_file(lib/init.clj init.clj @ONLY)
configure_file(lib/bootstrap.clj bootstrap.clj @ONLY)
configure_file(lib/clojure.core.clj clojure.core.clj @ONLY)
configure_file(lib/clojure.java.io.clj clojure.java.io.clj @ONLY)
configure_file(lib/clojure.java.shell.clj clojure.java.shell.clj @ONLY)
configure_file(lib/clojure.repl.clj clojure.repl.clj @ONLY)
configure_file(lib/clojure.string.clj clojure.string.clj @ONLY)
configure_file(lib/java.lang.Integer.clj java.lang.Integer.clj @ONLY)
configure_file(lib/java.lang.Long.clj java.lang.Long.clj @ONLY)
configure_file(lib/java.util.Date.clj java.util.Date.clj @ONLY)
configure_file(lib/sparkline.clj sparkline.clj @ONLY)
configure_file(lib/Plot.clj Plot.clj @ONLY)
configure_file(lib/nanoclj.core.Tensor.clj nanoclj.core.Tensor.clj @ONLY)
