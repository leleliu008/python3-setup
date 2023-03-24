#include <stdio.h>
#include <string.h>

#include "core/self.h"
#include "python3-setup.h"

int python3_setup_show_runtime_info() {
    const char * const userHomeDir = getenv("HOME");

    if (userHomeDir == NULL) {
        return PYTHON3_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t userHomeDirLength = strlen(userHomeDir);

    if (userHomeDirLength == 0U) {
        return PYTHON3_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t   python3SetupHomeDirLength = userHomeDirLength + 18U;
    char     python3SetupHomeDir[python3SetupHomeDirLength];
    snprintf(python3SetupHomeDir, python3SetupHomeDirLength, "%s/.python3-setup", userHomeDir);

    printf("\n");
    printf("python3-setup.vers : %s\n", PYTHON3_SETUP_VERSION);
    printf("python3-setup.home : %s\n", python3SetupHomeDir);

    char * selfRealPath = self_realpath();

    if (selfRealPath == NULL) {
        perror(NULL);
        return PYTHON3_SETUP_ERROR;
    }

    printf("python3-setup.path : %s\n\n", selfRealPath);

    free(selfRealPath);

    return PYTHON3_SETUP_OK;
}
