cmake_minimum_required(VERSION 3.5)
project(nanoclj C)

include(CheckSymbolExists)

IF (MSVC)
  set(VCPKG_TARGET_TRIPLET "x64-windows")
  set(VCPKG_ROOT "vcpkg")
  # set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
  include("${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
else()
  # The math optimizations are not necessary, but they do not cause harm. Since NaNs are needed -ffast-math or -ffinite-math-only cannot be used.
  add_compile_options(-std=gnu99 -Wall -O3 -march=native -funsafe-math-optimizations -fno-math-errno -fno-rounding-math -ffp-contract=fast -Wno-unused-value -Wno-unused-variable -Wno-unused-function -Werror=implicit-function-declaration -Wno-unused-but-set-variable -Werror=return-type -Werror=discarded-qualifiers)
  
  if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  else()
    add_compile_options(-fno-signaling-nans -fcx-limited-range)
  endif()

  if(DEFINED ENV{WARN_ERROR_BUILD})
    add_compile_options(-Werror)
  endif()
endif()

find_package(Threads REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_definitions(-DNANOCLJ_STANDALONE -DNANOCLJ_USE_LINENOISE)

# Data types, macros and functions related to controlling CPU affinity
# are available on Linux through GNU extensions in libc
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_compile_definitions(_GNU_SOURCE)
endif()

set(SOURCE_FILES
  src/nanoclj.c
  src/linenoise.c
  )

add_executable(nanoclj ${SOURCE_FILES})
target_link_libraries(nanoclj Threads::Threads)

IF (MSVC)
  find_package(utf8proc NAMES unofficial-utf8proc REQUIRED)
  target_link_libraries(nanoclj utf8proc)
else()
  find_library(UTF8PROC_LIBRARY utf8proc REQUIRED)
  target_link_libraries(nanoclj utf8proc)

  find_library(PCRE2_LIBRARY pcre2-8 REQUIRED)
  target_link_libraries(nanoclj pcre2-8)

  target_link_libraries(nanoclj m)

  find_library(CAIRO_LIBRARY cairo REQUIRED)
  target_link_libraries(nanoclj cairo)

  find_library(SIXEL_LIBRARY sixel REQUIRED)
  if(SIXEL_LIBRARY)
    target_link_libraries(nanoclj sixel)
    find_path(SIXEL_INCLUDE_DIR NAMES sixel.h PATHS "/usr/include" "/usr/include/sixel")
    target_include_directories(nanoclj PRIVATE ${SIXEL_INCLUDE_DIR})
  endif()

  find_library(SHP_LIBRARY shp REQUIRED)
  target_link_libraries(nanoclj shp)

  if(false)
    find_library(LIBZIP zip REQUIRED)
    target_link_libraries(nanoclj zip)
  endif()

  find_library(ZLIB z REQUIRED)
  target_link_libraries(nanoclj z)

  find_package(LibXml2 REQUIRED)
  target_link_libraries(nanoclj LibXml2::LibXml2)
  include_directories(${LIBXML2_INCLUDE_DIR})

  find_package(CURL REQUIRED)
  target_link_libraries(nanoclj CURL::libcurl)
  include_directories(${CURL_INCLUDE_DIR})

  check_symbol_exists("arc4random" "stdlib.h" HAVE_ARC4RANDOM)
  check_symbol_exists("arc4random_buf" "stdlib.h" HAVE_ARC4RANDOM_BUF)

  if (HAVE_ARC4RANDOM AND HAVE_ARC4RANDOM_BUF)
    add_definitions(-DUSE_ARC4RANDOM=1)
  endif()
endif()

install(TARGETS nanoclj RUNTIME DESTINATION bin)
install(FILES lib/init.clj
	      lib/bootstrap.clj
	      lib/clojure.core.clj
	      lib/clojure.java.io.clj
	      lib/clojure.java.shell.clj
	      lib/clojure.repl.clj
	      lib/clojure.string.clj
	      lib/clojure.set.clj
	      lib/clojure.instant.clj
	      lib/clojure.test.clj
	      lib/clojure.lang.PersistentVector.clj
	      lib/user.clj
	      lib/java.lang.Object.clj
	      lib/java.lang.Class.clj
	      lib/java.lang.Float.clj
	      lib/java.lang.Double.clj
	      lib/java.lang.Boolean.clj
	      lib/java.lang.Byte.clj
	      lib/java.lang.Short.clj
	      lib/java.lang.Integer.clj
	      lib/java.lang.Long.clj
	      lib/java.lang.String.clj
	      lib/java.util.Date.clj
	      lib/java.util.UUID.clj
	      lib/nanoclj.core.Codepoint.clj
	      lib/nanoclj.core.Tensor.clj
	      lib/nanoclj.core.Image.clj
	      lib/nanoclj.plot.clj
	      lib/nanoclj.graph.clj
	      lib/nanoclj.image.clj
	      lib/nanoclj.audio.clj
	      lib/nanoclj.art.clj
        DESTINATION share/nanoclj)

configure_file(lib/init.clj init.clj @ONLY)
configure_file(lib/bootstrap.clj bootstrap.clj @ONLY)
configure_file(lib/clojure.core.clj clojure.core.clj @ONLY)
configure_file(lib/clojure.java.io.clj clojure.java.io.clj @ONLY)
configure_file(lib/clojure.java.shell.clj clojure.java.shell.clj @ONLY)
configure_file(lib/clojure.repl.clj clojure.repl.clj @ONLY)
configure_file(lib/clojure.string.clj clojure.string.clj @ONLY)
configure_file(lib/clojure.set.clj clojure.set.clj @ONLY)
configure_file(lib/clojure.instant.clj clojure.instant.clj @ONLY)
configure_file(lib/clojure.test.clj clojure.test.clj @ONLY)
configure_file(lib/clojure.lang.PersistentVector.clj clojure.lang.PersistentVector.clj @ONLY)
configure_file(lib/user.clj user.clj @ONLY)
configure_file(lib/java.lang.Object.clj java.lang.Object.clj @ONLY)
configure_file(lib/java.lang.Class.clj java.lang.Class.clj @ONLY)
configure_file(lib/java.lang.Float.clj java.lang.Float.clj @ONLY)
configure_file(lib/java.lang.Double.clj java.lang.Double.clj @ONLY)
configure_file(lib/java.lang.Boolean.clj java.lang.Boolean.clj @ONLY)
configure_file(lib/java.lang.Byte.clj java.lang.Byte.clj @ONLY)
configure_file(lib/java.lang.Short.clj java.lang.Short.clj @ONLY)
configure_file(lib/java.lang.Integer.clj java.lang.Integer.clj @ONLY)
configure_file(lib/java.lang.Long.clj java.lang.Long.clj @ONLY)
configure_file(lib/java.lang.String.clj java.lang.String.clj @ONLY)
configure_file(lib/java.util.Date.clj java.util.Date.clj @ONLY)
configure_file(lib/java.util.UUID.clj java.util.UUID.clj @ONLY)
configure_file(lib/nanoclj.core.Codepoint.clj nanoclj.core.Codepoint.clj @ONLY)
configure_file(lib/nanoclj.core.Tensor.clj nanoclj.core.Tensor.clj @ONLY)
configure_file(lib/nanoclj.core.Image.clj nanoclj.core.Image.clj @ONLY)
configure_file(lib/nanoclj.plot.clj nanoclj.plot.clj @ONLY)
configure_file(lib/nanoclj.graph.clj nanoclj.graph.clj @ONLY)
configure_file(lib/nanoclj.image.clj nanoclj.image.clj @ONLY)
configure_file(lib/nanoclj.audio.clj nanoclj.audio.clj @ONLY)
configure_file(lib/nanoclj.art.clj nanoclj.art.clj @ONLY)

configure_file(tests/run.clj tests/run.clj @ONLY)
configure_file(tests/core.clj tests/core.clj @ONLY)
configure_file(tests/string.clj tests/string.clj @ONLY)
configure_file(tests/set.clj tests/set.clj @ONLY)
configure_file(tests/shell.clj tests/shell.clj @ONLY)
configure_file(tests/xml.clj tests/xml.clj @ONLY)
configure_file(tests/csv.clj tests/csv.clj @ONLY)
configure_file(tests/numeric-tower.clj tests/numeric-tower.clj @ONLY)
