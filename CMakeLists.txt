cmake_minimum_required(VERSION 2.8.12)
project(nanoclj C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_definitions(-DNANOCLJ_STANDALONE)

add_compile_options(-Wall -O3 -march=native)

if(DEFINED ENV{WARN_ERROR_BUILD})
    add_compile_options(-Werror)
endif()

set(SOURCE_FILES
    src/nanoclj.c
    src/linenoise.c
    )

add_executable(nanoclj ${SOURCE_FILES})

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(nanoclj ${MATH_LIBRARY})
endif()

find_library(UTF8PROC_LIBRARY utf8proc)
if(UTF8PROC_LIBRARY)
  target_link_libraries(nanoclj utf8proc)
endif()
  
configure_file(lib/init.clj init.clj @ONLY)
configure_file(lib/bootstrap.clj bootstrap.clj @ONLY)
configure_file(lib/clojure.core.clj clojure.core.clj @ONLY)
configure_file(lib/clojure.lang.io.clj clojure.lang.io.clj @ONLY)
configure_file(lib/clojure.repl.clj clojure.repl.clj @ONLY)
configure_file(lib/clojure.string.clj clojure.string.clj @ONLY)
configure_file(lib/Integer.clj Integer.clj @ONLY)
configure_file(lib/Long.clj Long.clj @ONLY)
configure_file(lib/sparkline.clj sparkline.clj @ONLY)
