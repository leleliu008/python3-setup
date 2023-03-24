#include <zlib.h>
#include <yaml.h>
#include <archive.h>
#include <curl/curlver.h>
#include <openssl/opensslv.h>

#include "python3-setup.h"

int python3_setup_show_build_info() {
    printf("build.utctime: %s\n\n", PYTHON3_SETUP_BUILD_TIMESTAMP);

    printf("build.libyaml: %s\n", yaml_get_version_string());
    printf("build.libcurl: %s\n", LIBCURL_VERSION);

//https://www.openssl.org/docs/man3.0/man3/OPENSSL_VERSION_BUILD_METADATA.html
//https://www.openssl.org/docs/man1.1.1/man3/OPENSSL_VERSION_TEXT.html
#ifdef OPENSSL_VERSION_STR
    printf("build.openssl: %s\n", OPENSSL_VERSION_STR);
#else
    printf("build.openssl: %s\n", OPENSSL_VERSION_TEXT);
#endif

    printf("build.archive: %s\n", ARCHIVE_VERSION_ONLY_STRING);
    printf("build.zlib:    %s\n\n", ZLIB_VERSION);

    return PYTHON3_SETUP_OK;
}
