cmake_minimum_required(VERSION 3.0)

project(python3-setup C)

set(PROJECT_VERSION       1.0.0)
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

########################################################################################

option(LINK_STATIC_LIBS "if prefer to use static libraries over dynamic libraries." OFF)

########################################################################################

if ("${CMAKE_EXE_LINKER_FLAGS}" MATCHES ".*-static.*")
    set(LINK_STATIC_LIBS ON)

    if ("${CMAKE_SYSTEM_NAME}" STREQUAL Darwin)
        message(FATAL "macOS DO NOT support static link the c runtime.")
    endif()
endif()

########################################################################################

if (DEFINED VCPKG_INSTALLED_DIR)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/pkgconfig")
    set(CMAKE_C_FLAGS              "-L${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib ${CMAKE_C_FLAGS}")
    list(INSERT CMAKE_INCLUDE_PATH 0 "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")
    list(INSERT CMAKE_LIBRARY_PATH 0 "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
endif()

########################################################################################

include(CheckIncludeFiles)

function(check_if_have_header_files)
    foreach(item ${ARGV})
        string(REPLACE "/" "_" item2 "${item}")
        string(REPLACE "." "_" item2 "${item2}")
        string(TOUPPER "${item2}" item2)
        check_include_files(${item} HAVE_${item2})
        #message("HAVE_${item2}=${HAVE_${item2}}")
    endforeach()
endfunction()

check_if_have_header_files(linux/limits.h)

########################################################################################

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

########################################################################################

message(STATUS "CMAKE_HOST_SYSTEM_NAME = ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_NAME      = ${CMAKE_SYSTEM_NAME}")

message(STATUS "CMAKE_VERSION        = ${CMAKE_VERSION}")
message(STATUS "CMAKE_PREFIX_PATH    = ${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_INCLUDE_PATH   = ${CMAKE_INCLUDE_PATH}")
message(STATUS "CMAKE_LIBRARY_PATH   = ${CMAKE_LIBRARY_PATH}")
message(STATUS "CMAKE_FIND_ROOT_PATH = ${CMAKE_FIND_ROOT_PATH}")
message(STATUS "ENV: PKG_CONFIG_PATH = $ENV{PKG_CONFIG_PATH}")

########################################################################################

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

if (LINK_STATIC_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

# https://cmake.org/cmake/help/latest/module/FindPkgConfig.html
find_package(PkgConfig REQUIRED)

# https://cmake.org/cmake/help/latest/module/FindOpenSSL.html
find_package(OpenSSL REQUIRED)

# https://cmake.org/cmake/help/latest/module/FindCURL.html
find_package(CURL REQUIRED)

# https://cmake.org/cmake/help/latest/module/FindLibArchive.html
find_package(LibArchive REQUIRED)

find_package(LibYAML REQUIRED)

find_package(ZLIB REQUIRED)

########################################################################################

if (OPENSSL_VERSION)
    message(STATUS "OPENSSL_VERSION = ${OPENSSL_VERSION}")

    string(REPLACE "." ";" OPENSSL_VERSION_PART_LIST ${OPENSSL_VERSION})
    list(GET OPENSSL_VERSION_PART_LIST 0 OPENSSL_VERSION_MAJOR)

    message(STATUS "OPENSSL_VERSION_MAJOR = ${OPENSSL_VERSION_MAJOR}")
else()
    message(FATAL "no OPENSSL_VERSION set.")
endif()

########################################################################################

aux_source_directory(src/core CORE_SRCS)
aux_source_directory(src           SRCS)

add_executable(python3-setup "${CORE_SRCS};${REGEX_SRC};${SRCS}")

########################################################################################

message(STATUS "CURL_LIBRARIES=${CURL_LIBRARIES}")
message(STATUS "OPENSSL_LIBRARIES=${OPENSSL_LIBRARIES}")
message(STATUS "LibArchive_LIBRARIES=${LibArchive_LIBRARIES}")
message(STATUS "LIBYAML_LIBRARY=${LIBYAML_LIBRARY}")
message(STATUS "ZLIB_LIBRARIES=${ZLIB_LIBRARIES}")

target_link_libraries(python3-setup CURL::libcurl)
target_link_libraries(python3-setup OpenSSL::SSL)
target_link_libraries(python3-setup OpenSSL::Crypto)
target_link_libraries(python3-setup LibArchive::LibArchive)
target_link_libraries(python3-setup LIBYAML::LIBYAML)
target_link_libraries(python3-setup ZLIB::ZLIB)
target_link_libraries(python3-setup "-lm")

########################################################################################

configure_file(src/config.h.in config.h @ONLY)

include_directories("${CMAKE_BINARY_DIR}")

########################################################################################

include(GNUInstallDirs)

install(TARGETS python3-setup RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
