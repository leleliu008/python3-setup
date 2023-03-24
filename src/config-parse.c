#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <libgen.h>
#include <time.h>
#include <yaml.h>

#include "python3-setup.h"

typedef enum {
    Python3SetupConfigKeyCode_unknown,

    Python3SetupConfigKeyCode_src_url_zlib,
    Python3SetupConfigKeyCode_src_sha_zlib,

    Python3SetupConfigKeyCode_src_url_libbz2,
    Python3SetupConfigKeyCode_src_sha_libbz2,

    Python3SetupConfigKeyCode_src_url_libffi,
    Python3SetupConfigKeyCode_src_sha_libffi,

    Python3SetupConfigKeyCode_src_url_sqlite,
    Python3SetupConfigKeyCode_src_sha_sqlite,

    Python3SetupConfigKeyCode_src_url_expat,
    Python3SetupConfigKeyCode_src_sha_expat,

    Python3SetupConfigKeyCode_src_url_openssl,
    Python3SetupConfigKeyCode_src_sha_openssl,

    Python3SetupConfigKeyCode_src_url_cmake,
    Python3SetupConfigKeyCode_src_sha_cmake,

    Python3SetupConfigKeyCode_src_url_perl,
    Python3SetupConfigKeyCode_src_sha_perl,
} Python3SetupConfigKeyCode;

void python3_setup_config_dump(Python3SetupConfig * config) {
    if (config == NULL) {
        return;
    }

    printf("src-url-zlib:    %s\n", config->src_url_zlib);
    printf("src-sha-zlib:    %s\n", config->src_sha_zlib);

    printf("src-url-libbz2:  %s\n", config->src_url_libbz2);
    printf("src-sha-libbz2:  %s\n", config->src_sha_libbz2);

    printf("src-url-libffi:  %s\n", config->src_url_libffi);
    printf("src-sha-libffi:  %s\n", config->src_sha_libffi);

    printf("src-url-sqlite:  %s\n", config->src_url_sqlite);
    printf("src-sha-sqlite:  %s\n", config->src_sha_sqlite);

    printf("src-url-expat:   %s\n", config->src_url_expat);
    printf("src-sha-expat:   %s\n", config->src_sha_expat);

    printf("src-url-openssl: %s\n", config->src_url_openssl);
    printf("src-sha-openssl: %s\n", config->src_sha_openssl);

    printf("src-url-cmake:   %s\n", config->src_url_cmake);
    printf("src-sha-cmake:   %s\n", config->src_sha_cmake);

    printf("src-url-perl:    %s\n", config->src_url_perl);
    printf("src-sha-perl:    %s\n", config->src_sha_perl);
}

void python3_setup_config_free(Python3SetupConfig * config) {
    if (config == NULL) {
        return;
    }

    //////////////////////////////////////

    if (config->src_url_zlib != NULL) {
        free(config->src_url_zlib);
        config->src_url_zlib = NULL;
    }

    if (config->src_sha_zlib != NULL) {
        free(config->src_sha_zlib);
        config->src_sha_zlib = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_libbz2 != NULL) {
        free(config->src_url_libbz2);
        config->src_url_libbz2 = NULL;
    }

    if (config->src_sha_libbz2 != NULL) {
        free(config->src_sha_libbz2);
        config->src_sha_libbz2 = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_libffi != NULL) {
        free(config->src_url_libffi);
        config->src_url_libffi = NULL;
    }

    if (config->src_sha_libffi != NULL) {
        free(config->src_sha_libffi);
        config->src_sha_libffi = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_sqlite != NULL) {
        free(config->src_url_sqlite);
        config->src_url_sqlite = NULL;
    }

    if (config->src_sha_sqlite != NULL) {
        free(config->src_sha_sqlite);
        config->src_sha_sqlite = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_expat != NULL) {
        free(config->src_url_expat);
        config->src_url_expat = NULL;
    }

    if (config->src_sha_expat != NULL) {
        free(config->src_sha_expat);
        config->src_sha_expat = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_openssl != NULL) {
        free(config->src_url_openssl);
        config->src_url_openssl = NULL;
    }

    if (config->src_sha_openssl != NULL) {
        free(config->src_sha_openssl);
        config->src_sha_openssl = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_cmake != NULL) {
        free(config->src_url_cmake);
        config->src_url_cmake = NULL;
    }

    if (config->src_sha_cmake != NULL) {
        free(config->src_sha_cmake);
        config->src_sha_cmake = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_perl != NULL) {
        free(config->src_url_perl);
        config->src_url_perl = NULL;
    }

    if (config->src_sha_perl != NULL) {
        free(config->src_sha_perl);
        config->src_sha_perl = NULL;
    }

    free(config);
}

static Python3SetupConfigKeyCode python3_setup_config_key_code_from_key_name(char * key) {
           if (strcmp(key, "src-url-perl") == 0) {
        return Python3SetupConfigKeyCode_src_url_perl;
    } else if (strcmp(key, "src-sha-perl") == 0) {
        return Python3SetupConfigKeyCode_src_sha_perl;
    } else if (strcmp(key, "src-url-cmake") == 0) {
        return Python3SetupConfigKeyCode_src_url_cmake;
    } else if (strcmp(key, "src-sha-cmake") == 0) {
        return Python3SetupConfigKeyCode_src_sha_cmake;
    } else if (strcmp(key, "src-url-openssl") == 0) {
        return Python3SetupConfigKeyCode_src_url_openssl;
    } else if (strcmp(key, "src-sha-openssl") == 0) {
        return Python3SetupConfigKeyCode_src_sha_openssl;
    } else if (strcmp(key, "src-url-expat") == 0) {
        return Python3SetupConfigKeyCode_src_url_expat;
    } else if (strcmp(key, "src-sha-expat") == 0) {
        return Python3SetupConfigKeyCode_src_sha_expat;
    } else if (strcmp(key, "src-url-sqlite") == 0) {
        return Python3SetupConfigKeyCode_src_url_sqlite;
    } else if (strcmp(key, "src-sha-sqlite") == 0) {
        return Python3SetupConfigKeyCode_src_sha_sqlite;
    } else if (strcmp(key, "src-url-libffi") == 0) {
        return Python3SetupConfigKeyCode_src_url_libffi;
    } else if (strcmp(key, "src-sha-libffi") == 0) {
        return Python3SetupConfigKeyCode_src_sha_libffi;
    } else if (strcmp(key, "src-url-zlib") == 0) {
        return Python3SetupConfigKeyCode_src_url_zlib;
    } else if (strcmp(key, "src-sha-zlib") == 0) {
        return Python3SetupConfigKeyCode_src_sha_zlib;
    } else if (strcmp(key, "src-url-libbz2") == 0) {
        return Python3SetupConfigKeyCode_src_url_libbz2;
    } else if (strcmp(key, "src-sha-libbz2") == 0) {
        return Python3SetupConfigKeyCode_src_sha_libbz2;
    } else {
        return Python3SetupConfigKeyCode_unknown;
    }
}

static void python3_setup_config_set_value(Python3SetupConfigKeyCode keyCode, char * value, Python3SetupConfig * config) {
    if (keyCode == Python3SetupConfigKeyCode_unknown) {
        return;
    }

    for (;;) {
        char c = value[0];

        if (c == '\0') {
            return;
        }

        // non-printable ASCII characters and space
        if (c <= 32) {
            value = &value[1];
        } else {
            break;
        }
    }

    switch (keyCode) {
        case Python3SetupConfigKeyCode_src_url_perl: if (config->src_url_perl != NULL) free(config->src_url_perl); config->src_url_perl = strdup(value); break;
        case Python3SetupConfigKeyCode_src_sha_perl: if (config->src_sha_perl != NULL) free(config->src_sha_perl); config->src_sha_perl = strdup(value); break;

        case Python3SetupConfigKeyCode_src_url_cmake: if (config->src_url_cmake != NULL) free(config->src_url_cmake); config->src_url_cmake = strdup(value); break;
        case Python3SetupConfigKeyCode_src_sha_cmake: if (config->src_sha_cmake != NULL) free(config->src_sha_cmake); config->src_sha_cmake = strdup(value); break;

        case Python3SetupConfigKeyCode_src_url_openssl: if (config->src_url_openssl != NULL) free(config->src_url_openssl); config->src_url_openssl = strdup(value); break;
        case Python3SetupConfigKeyCode_src_sha_openssl: if (config->src_sha_openssl != NULL) free(config->src_sha_openssl); config->src_sha_openssl = strdup(value); break;

        case Python3SetupConfigKeyCode_src_url_expat: if (config->src_url_expat != NULL) free(config->src_url_expat); config->src_url_expat = strdup(value); break;
        case Python3SetupConfigKeyCode_src_sha_expat: if (config->src_sha_expat != NULL) free(config->src_sha_expat); config->src_sha_expat = strdup(value); break;

        case Python3SetupConfigKeyCode_src_url_sqlite: if (config->src_url_sqlite != NULL) free(config->src_url_sqlite); config->src_url_sqlite = strdup(value); break;
        case Python3SetupConfigKeyCode_src_sha_sqlite: if (config->src_sha_sqlite != NULL) free(config->src_sha_sqlite); config->src_sha_sqlite = strdup(value); break;

        case Python3SetupConfigKeyCode_src_url_libffi: if (config->src_url_libffi != NULL) free(config->src_url_libffi); config->src_url_libffi = strdup(value); break;
        case Python3SetupConfigKeyCode_src_sha_libffi: if (config->src_sha_libffi != NULL) free(config->src_sha_libffi); config->src_sha_libffi = strdup(value); break;

        case Python3SetupConfigKeyCode_src_url_zlib: if (config->src_url_zlib != NULL) free(config->src_url_zlib); config->src_url_zlib = strdup(value); break;
        case Python3SetupConfigKeyCode_src_sha_zlib: if (config->src_sha_zlib != NULL) free(config->src_sha_zlib); config->src_sha_zlib = strdup(value); break;

        case Python3SetupConfigKeyCode_src_url_libbz2: if (config->src_url_libbz2 != NULL) free(config->src_url_libbz2); config->src_url_libbz2 = strdup(value); break;
        case Python3SetupConfigKeyCode_src_sha_libbz2: if (config->src_sha_libbz2 != NULL) free(config->src_sha_libbz2); config->src_sha_libbz2 = strdup(value); break;
        case Python3SetupConfigKeyCode_unknown: break;
    } 
}

int python3_setup_config_parse(const char * configFilePath, Python3SetupConfig * * out) {
    FILE * file = fopen(configFilePath, "r");

    if (file == NULL) {
        perror(configFilePath);
        return PYTHON3_SETUP_ERROR;
    }

    yaml_parser_t parser;
    yaml_token_t  token;

    // https://libyaml.docsforge.com/master/api/yaml_parser_initialize/
    if (yaml_parser_initialize(&parser) == 0) {
        perror("Failed to initialize yaml parser");
        fclose(file);
        return PYTHON3_SETUP_ERROR;
    }

    yaml_parser_set_input_file(&parser, file);

    Python3SetupConfigKeyCode configKeyCode = Python3SetupConfigKeyCode_unknown;

    Python3SetupConfig * config = NULL;

    int lastTokenType = 0;

    int ret = PYTHON3_SETUP_OK;

    do {
        // https://libyaml.docsforge.com/master/api/yaml_parser_scan/
        if (yaml_parser_scan(&parser, &token) == 0) {
            fprintf(stderr, "syntax error in config file: %s\n", configFilePath);
            ret = PYTHON3_SETUP_ERROR_FORMULA_SYNTAX;
            goto finalize;
        }

        switch(token.type) {
            case YAML_KEY_TOKEN:
                lastTokenType = 1;
                break;
            case YAML_VALUE_TOKEN:
                lastTokenType = 2;
                break;
            case YAML_SCALAR_TOKEN:
                if (lastTokenType == 1) {
                    configKeyCode = python3_setup_config_key_code_from_key_name((char*)token.data.scalar.value);
                } else if (lastTokenType == 2) {
                    if (config == NULL) {
                        config = (Python3SetupConfig*)calloc(1, sizeof(Python3SetupConfig));

                        if (config == NULL) {
                            ret = PYTHON3_SETUP_ERROR_MEMORY_ALLOCATE;
                            goto finalize;
                        }
                    }

                    python3_setup_config_set_value(configKeyCode, (char*)token.data.scalar.value, config);
                }
                break;
            default: 
                lastTokenType = 0;
                break;
        }

        if (token.type != YAML_STREAM_END_TOKEN) {
            yaml_token_delete(&token);
        }
    } while(token.type != YAML_STREAM_END_TOKEN);

finalize:
    yaml_token_delete(&token);

    yaml_parser_delete(&parser);

    fclose(file);

    if (ret == PYTHON3_SETUP_OK) {
        //python3_setup_config_dump(config);
        (*out) = config;
    } else {
        python3_setup_config_free(config);
    }

    return ret;
}
