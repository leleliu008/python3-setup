#include <math.h>
#include <string.h>
#include <sys/stat.h>

#include "core/sysinfo.h"
#include "core/self.h"
#include "core/tar.h"
#include "core/log.h"
#include "core/cp.h"
#include "python3-setup.h"

int python3_setup_upgrade_self(bool verbose) {
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

    size_t   python3SetupTmpDirLength = python3SetupHomeDirLength + 5U;
    char     python3SetupTmpDir[python3SetupTmpDirLength];
    snprintf(python3SetupTmpDir, python3SetupTmpDirLength, "%s/tmp", python3SetupHomeDir);

    if (stat(python3SetupTmpDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", python3SetupTmpDir);
            return PYTHON3_SETUP_ERROR;
        }
    } else {
        if (mkdir(python3SetupTmpDir, S_IRWXU) != 0) {
            perror(python3SetupTmpDir);
            return PYTHON3_SETUP_ERROR;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////

    const char * const githubApiUrl = "https://api.github.com/repos/leleliu008/python3-setup/releases/latest";

    size_t   githubApiResultJsonFilePathLength = python3SetupTmpDirLength + 13U;
    char     githubApiResultJsonFilePath[githubApiResultJsonFilePathLength];
    snprintf(githubApiResultJsonFilePath, githubApiResultJsonFilePathLength, "%s/latest.json", python3SetupTmpDir);

    int ret = python3_setup_http_fetch_to_file(githubApiUrl, githubApiResultJsonFilePath, verbose, verbose);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    FILE * file = fopen(githubApiResultJsonFilePath, "r");

    if (file == NULL) {
        perror(githubApiResultJsonFilePath);
        return PYTHON3_SETUP_ERROR;
    }

    char * latestVersion = NULL;

    char * p = NULL;

    char line[30];

    for (;;) {
        p = fgets(line, 30, file);

        if (p == NULL) {
            if (ferror(file)) {
                perror(githubApiResultJsonFilePath);
                goto finalize;
            } else {
                break;
            }
        }

        for (;;) {
            if (p[0] <= 32) { // non-printable ASCII characters and space
                p++;
            } else {
                break;
            }
        }

        if (strncmp(p, "\"tag_name\"", 10) == 0) {
            p += 10;

            for (;;) {
                if (p[0] == '\0') {
                    fprintf(stderr, "%s return invalid json.\n", githubApiUrl);
                    return PYTHON3_SETUP_ERROR;
                }

                if (p[0] == '"') { // found left double quote
                    p++;
                    break;
                } else {
                    p++;
                }
            }

            size_t n = 0;
            char * q = p;

            for (;;) {
                if (q[n] == '\0') {
                    fprintf(stderr, "%s return invalid json.\n", githubApiUrl);
                    return PYTHON3_SETUP_ERROR;
                }

                if (q[n] == '+') { // found right double quote
                    q[n] = '\0';
                    latestVersion = &q[0];
                    goto finalize;
                } else {
                    n++;
                }
            }
        }
    }

finalize:
    fclose(file);

    printf("latestVersion=%s\n", latestVersion);

    if (latestVersion == NULL) {
        fprintf(stderr, "%s return json has no tag_name key.\n", githubApiUrl);
        return PYTHON3_SETUP_ERROR;
    }

    size_t latestVersionCopyLength = strlen(latestVersion) + 1U;
    char   latestVersionCopy[latestVersionCopyLength];
    strncpy(latestVersionCopy, latestVersion, latestVersionCopyLength);

    char * latestVersionMajorStr = strtok(latestVersionCopy, ".");
    char * latestVersionMinorStr = strtok(NULL, ".");
    char * latestVersionPatchStr = strtok(NULL, ".");

    int latestVersionMajor = 0;
    int latestVersionMinor = 0;
    int latestVersionPatch = 0;

    if (latestVersionMajorStr != NULL) {
        latestVersionMajor = atoi(latestVersionMajorStr);
    }

    if (latestVersionMinorStr != NULL) {
        latestVersionMinor = atoi(latestVersionMinorStr);
    }

    if (latestVersionPatchStr != NULL) {
        latestVersionPatch = atoi(latestVersionPatchStr);
    }

    if (latestVersionMajor == 0 && latestVersionMinor == 0 && latestVersionPatch == 0) {
        fprintf(stderr, "invalid version format: %s\n", latestVersion);
        return PYTHON3_SETUP_ERROR;
    }

    if (latestVersionMajor < PYTHON3_SETUP_VERSION_MAJOR) {
        LOG_SUCCESS1("this software is already the latest version.");
        return PYTHON3_SETUP_OK;
    } else if (latestVersionMajor == PYTHON3_SETUP_VERSION_MAJOR) {
        if (latestVersionMinor < PYTHON3_SETUP_VERSION_MINOR) {
            LOG_SUCCESS1("this software is already the latest version.");
            return PYTHON3_SETUP_OK;
        } else if (latestVersionMinor == PYTHON3_SETUP_VERSION_MINOR) {
            if (latestVersionPatch <= PYTHON3_SETUP_VERSION_PATCH) {
                LOG_SUCCESS1("this software is already the latest version.");
                return PYTHON3_SETUP_OK;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////

    char osType[31] = {0};

    if (sysinfo_type(osType, 30) < 0) {
        perror(NULL);
        return PYTHON3_SETUP_ERROR;
    }

    char osArch[31] = {0};

    if (sysinfo_arch(osArch, 30) < 0) {
        perror(NULL);
        return PYTHON3_SETUP_ERROR;
    }

    size_t latestVersionLength = strlen(latestVersion);

    size_t   tarballFileNameLength = latestVersionLength + strlen(osType) + strlen(osArch) + 26U;
    char     tarballFileName[tarballFileNameLength];
    snprintf(tarballFileName, tarballFileNameLength, "python3-setup-%s-%s-%s.tar.xz", latestVersion, osType, osArch);

    size_t   tarballUrlLength = tarballFileNameLength + latestVersionLength + 66U;
    char     tarballUrl[tarballUrlLength];
    snprintf(tarballUrl, tarballUrlLength, "https://github.com/leleliu008/python3-setup/releases/download/%s/%s", latestVersion, tarballFileName);

    size_t   tarballFilePathLength = python3SetupTmpDirLength + tarballFileNameLength + 2U;
    char     tarballFilePath[tarballFilePathLength];
    snprintf(tarballFilePath, tarballFilePathLength, "%s/%s", python3SetupTmpDir, tarballFileName);

    ret = python3_setup_http_fetch_to_file(tarballUrl, tarballFilePath, verbose, verbose);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////////

    size_t   tarballExtractDirLength = tarballFilePathLength + 3U;
    char     tarballExtractDir[tarballExtractDirLength];
    snprintf(tarballExtractDir, tarballExtractDirLength, "%s.d", tarballFilePath);

    ret = tar_extract(tarballExtractDir, tarballFilePath, 0, verbose, 1);

    if (ret != 0) {
        return abs(ret) + PYTHON3_SETUP_ERROR_ARCHIVE_BASE;
    }

    size_t   upgradableExecutableFilePathLength = tarballExtractDirLength + 21U;
    char     upgradableExecutableFilePath[upgradableExecutableFilePathLength];
    snprintf(upgradableExecutableFilePath, upgradableExecutableFilePathLength, "%s/bin/python3-setup", tarballExtractDir);

    char * selfRealPath = self_realpath();

    if (selfRealPath == NULL) {
        perror(NULL);
        ret = PYTHON3_SETUP_ERROR;
        goto finally;
    }

    if (unlink(selfRealPath) != 0) {
        perror(selfRealPath);
        ret = PYTHON3_SETUP_ERROR;
        goto finally;
    }

    if (copy_file(upgradableExecutableFilePath, selfRealPath) < 0) {
        perror(NULL);
        ret = PYTHON3_SETUP_ERROR;
        goto finally;
    }

    if (chmod(selfRealPath, S_IRWXU) != 0) {
        perror(selfRealPath);
        ret = PYTHON3_SETUP_ERROR;
    }

finally:
    free(selfRealPath);

    if (ret == PYTHON3_SETUP_OK) {
        fprintf(stderr, "python3-setup is up to date with version %s\n", latestVersion);
    } else {
        fprintf(stderr, "Can't upgrade self. the latest version of executable was downloaded to %s, you can manually replace the current running program with it.\n", upgradableExecutableFilePath);
    }

    return ret;
}
