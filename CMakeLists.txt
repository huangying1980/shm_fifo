cmake_minimum_required(VERSION 3.0)

project(libshmfifo VERSION 1.0.1)

set(CMAKE_CXX_FLAGS_DEBUG "-g -DFIFO_DEBUG_VERBOSE -DFIFO_ERROR_VERBOSE -DFIFO_FENCE")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mavx512f -DFIFO_FAST_MEMCPY -DFIFO_FENCE")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mavx2 -DFIFO_FAST_MEMCPY -DFIFO_FENCE -DFIFO_DEBUG")
if(NOT CMAKE_BUILD_TYPE)
  # set(CMAKE_BUILD_TYPE "Debug")
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_VERBOSE_MAKEFILE on)
find_library(NUMA libnuma.so /usr/lib64)
IF (${NUMA} MATCHES "NOTFOUND")
message(STATUS "WARNING: Disable Numa")
set(CMAKE_CXX_FLAGS_DEBUG "-DFIFO_DISABLE_NUMA")
set(CMAKE_CXX_FLAGS_RELEASE "-DFIFO_DISABLE_NUMA")
ENDIF()
  
set(CMAKE_CXX_COMPILER "g++")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

string(TIMESTAMP COMPILE_TIME "%Y-%m-%d_%H:%M:%S")

aux_source_directory(src LIBFIFO_SRC)
include_directories(include)

set (CMAKE_DEBUG_POSTFIX d)

add_library(shmfifo SHARED ${LIBFIFO_SRC})
set_target_properties(shmfifo PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(shmfifo PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

target_link_libraries(shmfifo PRIVATE numa )

add_library(shmfifo_static STATIC ${LIBFIFO_SRC})
set_target_properties(shmfifo_static PROPERTIES OUTPUT_NAME "shmfifo")

add_definitions(--std=c++11)
add_definitions(-Wall -Werror -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE)
add_definitions(-DVERSION_STR=\"${PROJECT_VERSION}_${COMPILE_TIME}\")

install(TARGETS shmfifo DESTINATION lib)
install(TARGETS shmfifo_static DESTINATION lib)
file(GLOB HEADERS "include/*.h")
install(FILES ${HEADERS} DESTINATION include/shmfifo)

#add_subdirectory(test)
