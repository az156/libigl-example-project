# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/source/libigl-example-project/out/build/_deps/mpfr-src"
  "C:/source/libigl-example-project/out/build/_deps/mpfr-build"
  "C:/source/libigl-example-project/out/build/_deps/mpfr-subbuild/mpfr-populate-prefix"
  "C:/source/libigl-example-project/out/build/_deps/mpfr-subbuild/mpfr-populate-prefix/tmp"
  "C:/source/libigl-example-project/out/build/_deps/mpfr-subbuild/mpfr-populate-prefix/src/mpfr-populate-stamp"
  "C:/source/libigl-example-project/out/build/_deps/mpfr-subbuild/mpfr-populate-prefix/src"
  "C:/source/libigl-example-project/out/build/_deps/mpfr-subbuild/mpfr-populate-prefix/src/mpfr-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/source/libigl-example-project/out/build/_deps/mpfr-subbuild/mpfr-populate-prefix/src/mpfr-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/source/libigl-example-project/out/build/_deps/mpfr-subbuild/mpfr-populate-prefix/src/mpfr-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
