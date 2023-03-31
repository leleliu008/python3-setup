#include <stdio.h>

#include "python3-setup.h"

int python3_setup_show_default_config() {
    printf("src-url-python3: %s\n",   DEFAULT_SRC_URL_PYTHON3);
    printf("src-sha-python3: %s\n\n", DEFAULT_SRC_SHA_PYTHON3);

    printf("src-url-readline: %s\n",   DEFAULT_SRC_URL_READLINE);
    printf("src-sha-readline: %s\n\n", DEFAULT_SRC_SHA_READLINE);

    printf("src-url-ncurses: %s\n",   DEFAULT_SRC_URL_NCURSES);
    printf("src-sha-ncurses: %s\n\n", DEFAULT_SRC_SHA_NCURSES);

    printf("src-url-openssl: %s\n",   DEFAULT_SRC_URL_OPENSSL);
    printf("src-sha-openssl: %s\n\n", DEFAULT_SRC_SHA_OPENSSL);

    printf("src-url-libffi:  %s\n",   DEFAULT_SRC_URL_LIBFFI);
    printf("src-sha-libffi:  %s\n\n", DEFAULT_SRC_SHA_LIBFFI);

    printf("src-url-sqlite:  %s\n",   DEFAULT_SRC_URL_SQLITE);
    printf("src-sha-sqlite:  %s\n\n", DEFAULT_SRC_SHA_SQLITE);

    printf("src-url-libbz2:  %s\n",   DEFAULT_SRC_URL_LIBBZ2);
    printf("src-sha-libbz2:  %s\n\n", DEFAULT_SRC_SHA_LIBBZ2);

    printf("src-url-expat:   %s\n",   DEFAULT_SRC_URL_EXPAT);
    printf("src-sha-expat:   %s\n\n", DEFAULT_SRC_SHA_EXPAT);

    printf("src-url-gdbm:    %s\n",   DEFAULT_SRC_URL_GDBM);
    printf("src-sha-gdbm:    %s\n\n", DEFAULT_SRC_SHA_GDBM);

    printf("src-url-zlib:    %s\n",   DEFAULT_SRC_URL_ZLIB);
    printf("src-sha-zlib:    %s\n\n", DEFAULT_SRC_SHA_ZLIB);

    printf("src-url-xz:      %s\n",   DEFAULT_SRC_URL_XZ);
    printf("src-sha-xz:      %s\n\n", DEFAULT_SRC_SHA_XZ);

    printf("src-url-perl:    %s\n",   DEFAULT_SRC_URL_PERL);
    printf("src-sha-perl:    %s\n\n", DEFAULT_SRC_SHA_PERL);

    return PYTHON3_SETUP_OK;
}
