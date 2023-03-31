#ifndef PYTHON3_SETUP_H
#define PYTHON3_SETUP_H

#include <config.h>
#include <stdlib.h>
#include <stdbool.h>

 
#define PYTHON3_SETUP_OK                     0
#define PYTHON3_SETUP_ERROR                  1

#define PYTHON3_SETUP_ERROR_ARG_IS_NULL      2
#define PYTHON3_SETUP_ERROR_ARG_IS_EMPTY     3
#define PYTHON3_SETUP_ERROR_ARG_IS_INVALID   4
#define PYTHON3_SETUP_ERROR_ARG_IS_UNKNOWN   5

#define PYTHON3_SETUP_ERROR_MEMORY_ALLOCATE  6

#define PYTHON3_SETUP_ERROR_SHA256_MISMATCH  7

#define PYTHON3_SETUP_ERROR_ENV_HOME_NOT_SET 8
#define PYTHON3_SETUP_ERROR_ENV_PATH_NOT_SET 9

#define PYTHON3_SETUP_ERROR_EXE_NOT_FOUND    10

#define PYTHON3_SETUP_ERROR_FORMULA_SYNTAX   45
#define PYTHON3_SETUP_ERROR_FORMULA_SCHEME   46

// libgit's error [-35, -1]
#define PYTHON3_SETUP_ERROR_LIBGIT2_BASE    70

// libarchive's error [-30, 1]
#define PYTHON3_SETUP_ERROR_ARCHIVE_BASE    110

// libcurl's error [1, 99]
#define PYTHON3_SETUP_ERROR_NETWORK_BASE    150

////////////////////////////////////////////////////

#define DEFAULT_SRC_URL_XZ      "https://github.com/xz-mirror/xz/releases/download/v5.4.0/xz-5.4.0.tar.xz"
#define DEFAULT_SRC_SHA_XZ      "5f260e3b43f75cf43ca43d107dd18209f7d516782956a74ddd53288e02a83a31"

#define DEFAULT_SRC_URL_ZLIB    "https://zlib.net/zlib-1.2.13.tar.gz"
#define DEFAULT_SRC_SHA_ZLIB    "b3a24de97a8fdbc835b9833169501030b8977031bcb54b3b3ac13740f846ab30"

#define DEFAULT_SRC_URL_LIBBZ2  "https://github.com/leleliu008/bzip2/archive/refs/tags/1.0.8.tar.gz"
#define DEFAULT_SRC_SHA_LIBBZ2  "0100da0b55f552134d732acdd0325e84a0ef731a305c15f6a2ea24308de09759"

#define DEFAULT_SRC_URL_LIBFFI  "https://github.com/libffi/libffi/releases/download/v3.4.4/libffi-3.4.4.tar.gz"
#define DEFAULT_SRC_SHA_LIBFFI  "d66c56ad259a82cf2a9dfc408b32bf5da52371500b84745f7fb8b645712df676"

#define DEFAULT_SRC_URL_SQLITE  "https://www.sqlite.org/2023/sqlite-autoconf-3410100.tar.gz"
#define DEFAULT_SRC_SHA_SQLITE  "4dadfbeab9f8e16c695d4fbbc51c16b2f77fb97ff4c1c3d139919dfc038c9e33"

#define DEFAULT_SRC_URL_EXPAT   "https://github.com/libexpat/libexpat/releases/download/R_2_5_0/expat-2.5.0.tar.xz"
#define DEFAULT_SRC_SHA_EXPAT   "ef2420f0232c087801abf705e89ae65f6257df6b7931d37846a193ef2e8cdcbe"
 
#define DEFAULT_SRC_URL_GDBM    "https://ftp.gnu.org/gnu/gdbm/gdbm-1.23.tar.gz"
#define DEFAULT_SRC_SHA_GDBM    "74b1081d21fff13ae4bd7c16e5d6e504a4c26f7cde1dca0d963a484174bbcacd"

#define DEFAULT_SRC_URL_OPENSSL "https://www.openssl.org/source/openssl-3.0.5.tar.gz"
#define DEFAULT_SRC_SHA_OPENSSL "aa7d8d9bef71ad6525c55ba11e5f4397889ce49c2c9349dcea6d3e4f0b024a7a"

#define DEFAULT_SRC_URL_NCURSES "https://ftp.gnu.org/gnu/ncurses/ncurses-6.3.tar.gz"
#define DEFAULT_SRC_SHA_NCURSES "97fc51ac2b085d4cde31ef4d2c3122c21abc217e9090a43a30fc5ec21684e059"

#define DEFAULT_SRC_URL_READLINE "https://ftp.gnu.org/gnu/readline/readline-8.1.2.tar.gz"
#define DEFAULT_SRC_SHA_READLINE "7589a2381a8419e68654a47623ce7dfcb756815c8fee726b98f90bf668af7bc6"

#define DEFAULT_SRC_URL_PYTHON3 "https://www.python.org/ftp/python/3.11.2/Python-3.11.2.tgz"
#define DEFAULT_SRC_SHA_PYTHON3  "2411c74bda5bbcfcddaf4531f66d1adc73f247f529aee981b029513aefdbf849"
 
#define DEFAULT_SRC_URL_CMAKE   "https://github.com/Kitware/CMake/releases/download/v3.26.0/cmake-3.26.0.tar.gz"
#define DEFAULT_SRC_SHA_CMAKE   "4256613188857e95700621f7cdaaeb954f3546a9249e942bc2f9b3c26e381365"
 
#define DEFAULT_SRC_URL_PERL    "https://cpan.metacpan.org/authors/id/R/RJ/RJBS/perl-5.36.0.tar.xz"
#define DEFAULT_SRC_SHA_PERL    "0f386dccbee8e26286404b2cca144e1005be65477979beb9b1ba272d4819bcf0"

////////////////////////////////////////////////////

typedef struct {
    char * src_url_xz;
    char * src_sha_xz;

    char * src_url_zlib;
    char * src_sha_zlib;

    char * src_url_libbz2;
    char * src_sha_libbz2;

    char * src_url_libffi;
    char * src_sha_libffi;

    char * src_url_sqlite;
    char * src_sha_sqlite;

    char * src_url_gdbm;
    char * src_sha_gdbm;

    char * src_url_expat;
    char * src_sha_expat;

    char * src_url_openssl;
    char * src_sha_openssl;

    char * src_url_ncurses;
    char * src_sha_ncurses;

    char * src_url_readline;
    char * src_sha_readline;

    char * src_url_python3;
    char * src_sha_python3;

    char * src_url_cmake;
    char * src_sha_cmake;

    char * src_url_perl;
    char * src_sha_perl;

} Python3SetupConfig;

int  python3_setup_config_parse(const char * configFilePath, Python3SetupConfig * * out);
void python3_setup_config_free(Python3SetupConfig * config);
void python3_setup_config_dump(Python3SetupConfig * config);

////////////////////////////////////////////////////

int python3_setup_main(int argc, char* argv[]);

int python3_setup_util(int argc, char* argv[]);

int python3_setup_help();

int python3_setup_sysinfo();

int python3_setup_env();

int python3_setup_show_default_config();

int python3_setup_upgrade_self(bool verbose);

int python3_setup_integrate_zsh_completion (const char * outputDir, bool verbose);
int python3_setup_integrate_bash_completion(const char * outputDir, bool verbose);
int python3_setup_integrate_fish_completion(const char * outputDir, bool verbose);

typedef enum {
    Python3SetupLogLevel_silent,
    Python3SetupLogLevel_normal,
    Python3SetupLogLevel_verbose,
    Python3SetupLogLevel_very_verbose
} Python3SetupLogLevel;

int python3_setup_setup(const char * configFilePath, const char * setupDir, Python3SetupLogLevel logLevel, unsigned int jobs);

int python3_setup_http_fetch_to_file(const char * url, const char * outputFilePath, bool verbose, bool showProgress);

#endif
