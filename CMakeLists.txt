cmake_minimum_required(VERSION 2.8.12)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

#######################
# project information #
#######################

project(yantarantana)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

########################
# custom cmake modules #
########################

include(${CMAKE_SOURCE_DIR}/cmake/include/define_module.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/include/git_sha1.cmake)

#####################
# compiler settings #
#####################

if (WIN32)
  set(CMAKE_CXX_FLAGS_RELEASE "/O3")
  set(SYSTEM_WINDOWS 1)
  if(MSVC)
    # Default to statically-linked runtime.
    if("${MSVC_RUNTIME}" STREQUAL "")
      set(MSVC_RUNTIME "static")
    endif()
    # Set compiler options.
    set(variables
      CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
    if(${MSVC_RUNTIME} STREQUAL "static")
      message(STATUS
        "MSVC uses static runtime"
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
        endif()
      endforeach()
    else()
      message(STATUS
        "MSVC uses dynamic runtime"
      )
      foreach(variable ${variables})
        if(${variable} MATCHES "/MT")
          string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
        endif()
      endforeach()
    endif()
  endif()
else()
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-deprecated-declarations -fomit-frame-pointer -fPIC -std=c++11")
  set(CMAKE_CXX_FLAGS_DEBUG   "-g -Wall -Wextra -fPIC -std=c++11")
  set(SYSTEM_UNIX 1)
endif()
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Release or Debug" FORCE)
endif()

#######################
# project directories #
#######################

set(BUILD_TESTS TRUE CACHE BOOL "Build boost unit tests")

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(modules)
add_subdirectory(document)

###############
# config file #
###############

configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/config.h.in
  ${PROJECT_BINARY_DIR}/config.h
)

define_module(yantarantana BINARY SOURCES yantarantana.cpp LINKS document sg_gui skia)
