#include <stdio.h>

#include "python3-setup.h"

extern int python3_setup_show_build_info();
extern int python3_setup_show_runtime_info();

int python3_setup_env() {
    int ret = python3_setup_show_build_info();

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    ///////////////////////////////////////////////

    ret = python3_setup_sysinfo();

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    ///////////////////////////////////////////////

    ret = python3_setup_show_runtime_info();

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    ///////////////////////////////////////////////

    printf("default config:\n\n");

    return python3_setup_show_default_config();
}
