#include <stdio.h>

#include "core/sysinfo.h"
#include "python3-setup.h"

int python3_setup_sysinfo() {
    SysInfo sysinfo = {0};

    if (sysinfo_make(&sysinfo) < 0) {
        perror(NULL);
        return PYTHON3_SETUP_ERROR;
    }

    sysinfo_dump(sysinfo);
    sysinfo_free(sysinfo);
   
    return PYTHON3_SETUP_OK;
}
