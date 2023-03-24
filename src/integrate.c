#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "python3-setup.h"

int python3_setup_integrate_zsh_completion(const char * outputDir, bool verbose) {
    const char * const url = "https://raw.githubusercontent.com/leleliu008/python3-setup/master/python3-setup-zsh-completion";

    const char * const userHomeDir = getenv("HOME");

    if (userHomeDir == NULL) {
        return PYTHON3_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t userHomeDirLength = strlen(userHomeDir);

    if (userHomeDirLength == 0U) {
        return PYTHON3_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    ////////////////////////////////////////////////////////////////

    struct stat st;

    size_t   python3SetupHomeDirLength = userHomeDirLength + 18U;
    char     python3SetupHomeDir[python3SetupHomeDirLength];
    snprintf(python3SetupHomeDir, python3SetupHomeDirLength, "%s/.python3-setup", userHomeDir);

    if (stat(python3SetupHomeDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", python3SetupHomeDir);
            return PYTHON3_SETUP_ERROR;
        }
    } else {
        if (mkdir(python3SetupHomeDir, S_IRWXU) != 0) {
            perror(python3SetupHomeDir);
            return PYTHON3_SETUP_ERROR;
        }
    }

    ////////////////////////////////////////////////////////////////

    size_t zshCompletionDirLength = python3SetupHomeDirLength + 16U;
    char   zshCompletionDir[zshCompletionDirLength];
    snprintf(zshCompletionDir, zshCompletionDirLength, "%s/zsh_completion", python3SetupHomeDir);

    if (stat(zshCompletionDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", zshCompletionDir);
            return PYTHON3_SETUP_ERROR;
        }
    } else {
        if (mkdir(zshCompletionDir, S_IRWXU) != 0) {
            perror(zshCompletionDir);
            return PYTHON3_SETUP_ERROR;
        }
    }

    ////////////////////////////////////////////////////////////////

    size_t   zshCompletionFilePathLength = zshCompletionDirLength + 18U;
    char     zshCompletionFilePath[zshCompletionFilePathLength];
    snprintf(zshCompletionFilePath, zshCompletionFilePathLength, "%s/_python3-setup", zshCompletionDir);

    int ret = python3_setup_http_fetch_to_file(url, zshCompletionFilePath, verbose, verbose);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    if (outputDir == NULL) {
        return PYTHON3_SETUP_OK;
    }

    if (stat(outputDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", outputDir);
            return PYTHON3_SETUP_ERROR;
        }
    } else {
        fprintf(stderr, "'%s\n' directory was expected to be exist, but it was not.\n", outputDir);
        return PYTHON3_SETUP_ERROR;
    }

    size_t   destFilePathLength = strlen(outputDir) + 18U;
    char     destFilePath[destFilePathLength];
    snprintf(destFilePath, destFilePathLength, "%s/_python3-setup", outputDir);

    if (symlink(zshCompletionFilePath, destFilePath) != 0) {
        perror(destFilePath);
        return PYTHON3_SETUP_ERROR;
    } else {
        return PYTHON3_SETUP_OK;
    }
}

int python3_setup_integrate_bash_completion(const char * outputDir, bool verbose) {
    (void)outputDir;
    (void)verbose;
    return PYTHON3_SETUP_OK;
}

int python3_setup_integrate_fish_completion(const char * outputDir, bool verbose) {
    (void)outputDir;
    (void)verbose;
    return PYTHON3_SETUP_OK;
}
