cmake_minimum_required(VERSION 2.8.12)
project(nanoclj C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_definitions(-DNANOCLJ_STANDALONE)

# Uncomment to enable Clang Address Sanitizer
#set (CMAKE_C_FLAGS "-fsanitize=address -O1 -fno-omit-frame-pointer")

add_compile_options(-Wall -O3 -march=native)

if(DEFINED ENV{WARN_ERROR_BUILD})
    add_compile_options(-Werror)
endif()

set(SOURCE_FILES
    nanoclj.c
    linenoise.c
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
  
configure_file(init.clj init.clj @ONLY)
configure_file(bootstrap.clj bootstrap.clj @ONLY)
configure_file(clojure.core.clj clojure.core.clj @ONLY)
configure_file(clojure.lang.io.clj clojure.lang.io.clj @ONLY)
configure_file(clojure.repl.clj clojure.repl.clj @ONLY)
configure_file(clojure.string.clj clojure.string.clj @ONLY)
configure_file(Integer.clj Integer.clj @ONLY)
configure_file(Long.clj Long.clj @ONLY)
configure_file(Math.clj Math.clj @ONLY)
