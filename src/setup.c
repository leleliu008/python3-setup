#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "core/cp.h"
#include "core/log.h"
#include "core/sysinfo.h"
#include "core/sha256sum.h"
#include "core/base16.h"
#include "core/tar.h"
#include "core/exe.h"
#include "core/self.h"
#include "core/rm-r.h"
#include "core/exe.h"
#include "python3-setup.h"

#define LOG_STEP(output2Terminal, logLevel, stepN, msg) \
    if (logLevel != Python3SetupLogLevel_silent) { \
        if (output2Terminal) { \
            fprintf(stderr, "\n%s=>> STEP %u : %s%s\n", COLOR_PURPLE, stepN, msg, COLOR_OFF); \
        } else { \
            fprintf(stderr, "\n=>> STEP %u : %s\n", stepN, msg); \
        } \
    }

#define LOG_RUN_CMD(output2Terminal, logLevel, cmd) \
    if (logLevel != Python3SetupLogLevel_silent) { \
        if (output2Terminal) { \
            fprintf(stderr, "%s==>%s %s%s%s\n", COLOR_PURPLE, COLOR_OFF, COLOR_GREEN, cmd, COLOR_OFF); \
        } else { \
            fprintf(stderr, "==> %s\n", cmd); \
        } \
    }

static int run_cmd(char * cmd, int redirectOutput2FD) {
    pid_t pid = fork();

    if (pid < 0) {
        perror(NULL);
        return PYTHON3_SETUP_ERROR;
    }

    if (pid == 0) {
        if (redirectOutput2FD > 0) {
            if (dup2(redirectOutput2FD, STDOUT_FILENO) < 0) {
                perror(NULL);
                exit(125);
            }

            if (dup2(redirectOutput2FD, STDERR_FILENO) < 0) {
                perror(NULL);
                exit(126);
            }
        }

        ////////////////////////////////////////

        size_t argc = 0U;
        char*  argv[30] = {0};

        char * arg = strtok(cmd, " ");

        while (arg != NULL) {
            argv[argc] = arg;
            argc++;
            arg = strtok(NULL, " ");
        }

        ////////////////////////////////////////

        execv(argv[0], argv);
        perror(argv[0]);
        exit(127);
    } else {
        int childProcessExitStatusCode;

        if (waitpid(pid, &childProcessExitStatusCode, 0) < 0) {
            perror(NULL);
            return PYTHON3_SETUP_ERROR;
        }

        if (childProcessExitStatusCode == 0) {
            return PYTHON3_SETUP_OK;
        }

        if (WIFEXITED(childProcessExitStatusCode)) {
            fprintf(stderr, "running command '%s' exit with status code: %d\n", cmd, WEXITSTATUS(childProcessExitStatusCode));
        } else if (WIFSIGNALED(childProcessExitStatusCode)) {
            fprintf(stderr, "running command '%s' killed by signal: %d\n", cmd, WTERMSIG(childProcessExitStatusCode));
        } else if (WIFSTOPPED(childProcessExitStatusCode)) {
            fprintf(stderr, "running command '%s' stopped by signal: %d\n", cmd, WSTOPSIG(childProcessExitStatusCode));
        }

        return PYTHON3_SETUP_ERROR;
    }
}

static int python3_setup_download_and_uncompress(const char * url, const char * sha, const char * downloadsDir, size_t downloadsDirLength, const char * uncompressDir, Python3SetupLogLevel logLevel) {
    size_t   urlLength = strlen(url);

    size_t   fetchPhaseCmdLength = urlLength + 10U;
    char     fetchPhaseCmd[fetchPhaseCmdLength];
    snprintf(fetchPhaseCmd, fetchPhaseCmdLength, "Fetching %s", url);

    LOG_RUN_CMD(true, logLevel, fetchPhaseCmd)

    ////////////////////////////////////////////////////////////////////////

    char    urlCopy[urlLength + 1U];
    strncpy(urlCopy, url, urlLength + 1U);

    char *   fileName = basename(urlCopy);

    size_t   fileNameLength = strlen(urlCopy);

    size_t   filePathLength = downloadsDirLength + fileNameLength + 2U;
    char     filePath[filePathLength];
    snprintf(filePath, filePathLength, "%s/%s", downloadsDir, fileName);

    bool needFetch = true;

    struct stat st;

    if ((stat(filePath, &st) == 0) && S_ISREG(st.st_mode)) {
        char actualSHA256SUM[65] = {0};

        if (sha256sum_of_file(actualSHA256SUM, filePath) != 0) {
            perror(filePath);
            return PYTHON3_SETUP_ERROR;
        }

        if (strcmp(actualSHA256SUM, sha) == 0) {
            needFetch = false;

            if (logLevel >= Python3SetupLogLevel_verbose) {
                fprintf(stderr, "%s already have been fetched.\n", filePath);
            }
        }
    }

    if (needFetch) {
        int ret = python3_setup_http_fetch_to_file(url, filePath, logLevel >= Python3SetupLogLevel_verbose, logLevel != Python3SetupLogLevel_silent);

        if (ret != PYTHON3_SETUP_OK) {
            return ret;
        }

        char actualSHA256SUM[65] = {0};

        if (sha256sum_of_file(actualSHA256SUM, filePath) != 0) {
            perror(filePath);
            return PYTHON3_SETUP_ERROR;
        }

        if (strcmp(actualSHA256SUM, sha) != 0) {
            fprintf(stderr, "sha256sum mismatch.\n    expect : %s\n    actual : %s\n", sha, actualSHA256SUM);
            return PYTHON3_SETUP_ERROR_SHA256_MISMATCH;
        }
    }

    size_t   uncompressPhaseCmdLength = filePathLength + 36U;
    char     uncompressPhaseCmd[uncompressPhaseCmdLength];
    snprintf(uncompressPhaseCmd, uncompressPhaseCmdLength, "Uncompressing %s --strip-components=1", filePath);

    LOG_RUN_CMD(true, logLevel, uncompressPhaseCmd)

    return tar_extract(uncompressDir, filePath, ARCHIVE_EXTRACT_TIME, logLevel >= Python3SetupLogLevel_verbose, 1);
}

static int python3_setup_write_receipt(const char * setupDir, size_t setupDirLength, const char * binUrlGmake, const char * binShaGmake, Python3SetupConfig config, SysInfo sysinfo) {
    size_t   receiptFilePathLength = setupDirLength + 13U;
    char     receiptFilePath[receiptFilePathLength];
    snprintf(receiptFilePath, receiptFilePathLength, "%s/receipt.yml", setupDir);

    FILE *   receiptFile = fopen(receiptFilePath, "w");

    if (receiptFile == NULL) {
        perror(receiptFilePath);
        return PYTHON3_SETUP_ERROR;
    }

    fprintf(receiptFile, "bin-url-gmake:   %s\n",   (binUrlGmake == NULL) ? "" : binUrlGmake);
    fprintf(receiptFile, "bin-sha-gmake:   %s\n\n", (binShaGmake == NULL) ? "" : binShaGmake);

    fprintf(receiptFile, "src-url-cmake:   %s\n",   config.src_url_cmake);
    fprintf(receiptFile, "src-sha-cmake:   %s\n\n", config.src_sha_cmake);

    fprintf(receiptFile, "src-url-perl:    %s\n",   config.src_url_perl);
    fprintf(receiptFile, "src-sha-perl:    %s\n\n", config.src_sha_perl);

    fprintf(receiptFile, "src-url-python3: %s\n",   config.src_url_python3);
    fprintf(receiptFile, "src-sha-python3: %s\n\n", config.src_sha_python3);

    fprintf(receiptFile, "src-url-readline: %s\n",   config.src_url_readline);
    fprintf(receiptFile, "src-sha-readline: %s\n\n", config.src_sha_readline);

    fprintf(receiptFile, "src-url-ncurses: %s\n",   config.src_url_ncurses);
    fprintf(receiptFile, "src-sha-ncurses: %s\n\n", config.src_sha_ncurses);

    fprintf(receiptFile, "src-url-openssl: %s\n",   config.src_url_openssl);
    fprintf(receiptFile, "src-sha-openssl: %s\n\n", config.src_sha_openssl);

    fprintf(receiptFile, "src-url-sqlite:  %s\n",   config.src_url_sqlite);
    fprintf(receiptFile, "src-sha-sqlite:  %s\n\n", config.src_sha_sqlite);

    fprintf(receiptFile, "src-url-libffi:  %s\n",   config.src_url_libffi);
    fprintf(receiptFile, "src-sha-libffi:  %s\n\n", config.src_sha_libffi);

    fprintf(receiptFile, "src-url-libbz2:  %s\n",   config.src_url_libbz2);
    fprintf(receiptFile, "src-sha-libbz2:  %s\n\n", config.src_sha_libbz2);

    fprintf(receiptFile, "src-url-expat:   %s\n",   config.src_url_expat);
    fprintf(receiptFile, "src-sha-expat:   %s\n\n", config.src_sha_expat);

    fprintf(receiptFile, "src-url-gdbm:    %s\n",   config.src_url_gdbm);
    fprintf(receiptFile, "src-sha-gdbm:    %s\n\n", config.src_sha_gdbm);

    fprintf(receiptFile, "src-url-zlib:    %s\n",   config.src_url_zlib);
    fprintf(receiptFile, "src-sha-zlib:    %s\n\n", config.src_sha_zlib);

    fprintf(receiptFile, "\nsignature: %s\ntimestamp: %lu\n\n", PYTHON3_SETUP_VERSION, time(NULL));

    fprintf(receiptFile, "build-on:\n    os-arch: %s\n    os-kind: %s\n    os-type: %s\n    os-name: %s\n    os-vers: %s\n    os-ncpu: %u\n    os-euid: %u\n    os-egid: %u\n", sysinfo.arch, sysinfo.kind, sysinfo.type, sysinfo.name, sysinfo.vers, sysinfo.ncpu, sysinfo.euid, sysinfo.egid);

    fclose(receiptFile);

    return PYTHON3_SETUP_OK;
}

static int setup_perl(const char * gmakePath, size_t gmakePathLength, const char * setupDir, size_t setupDirLength, unsigned int jobs, Python3SetupLogLevel logLevel, int redirectOutput2FD, bool output2Terminal) {
    size_t   configurePhaseCmdLength = setupDirLength + 110U;
    char     configurePhaseCmd[configurePhaseCmdLength];
    snprintf(configurePhaseCmd, configurePhaseCmdLength, "./Configure -Dprefix=%s -des -Dmake=gmake -Duselargefiles -Duseshrplib -Dusethreads -Dusenm=false -Dusedl=true", setupDir);

    LOG_RUN_CMD(output2Terminal, logLevel, configurePhaseCmd)

    int ret = run_cmd(configurePhaseCmd, redirectOutput2FD);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   buildPhaseCmdLength = gmakePathLength + 12U;
    char     buildPhaseCmd[buildPhaseCmdLength];
    snprintf(buildPhaseCmd, buildPhaseCmdLength, "%s --jobs=%u", gmakePath, jobs);

    LOG_RUN_CMD(output2Terminal, logLevel, buildPhaseCmd)

    ret = run_cmd(buildPhaseCmd, redirectOutput2FD);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   installPhaseCmdLength = gmakePathLength + 20U;
    char     installPhaseCmd[installPhaseCmdLength];
    snprintf(installPhaseCmd, installPhaseCmdLength, "%s install", gmakePath);

    LOG_RUN_CMD(output2Terminal, logLevel, installPhaseCmd)

    return run_cmd(installPhaseCmd, redirectOutput2FD);
}

static int setup_openssl(const char * gmakePath, size_t gmakePathLength, const char * setupDir, size_t setupDirLength, unsigned int jobs, Python3SetupLogLevel logLevel, int redirectOutput2FD, bool output2Terminal, SysInfo sysinfo) {
    // https://github.com/openssl/openssl/issues/19232

    if (strcmp(sysinfo.kind, "openbsd") == 0) {
        const char * patchPhaseCmd = "/usr/bin/sed -i s/-Wl,-z,defs// Configurations/shared-info.pl";

        size_t  patchPhaseCmdCopyLength = strlen(patchPhaseCmd);
        char    patchPhaseCmdCopy[patchPhaseCmdCopyLength + 1U];
        strncpy(patchPhaseCmdCopy, patchPhaseCmd, patchPhaseCmdCopyLength);

        patchPhaseCmdCopy[patchPhaseCmdCopyLength] = '\0';

        LOG_RUN_CMD(output2Terminal, logLevel, patchPhaseCmd)

        int ret = run_cmd(patchPhaseCmdCopy, redirectOutput2FD);

        if (ret != PYTHON3_SETUP_OK) {
            return ret;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   configurePhaseCmdLength = (setupDirLength << 1) + 73U;
    char     configurePhaseCmd[configurePhaseCmdLength];
    snprintf(configurePhaseCmd, configurePhaseCmdLength, "./config no-tests no-ssl3 no-ssl3-method no-zlib --prefix=%s --libdir=%s/lib", setupDir, setupDir);

    LOG_RUN_CMD(output2Terminal, logLevel, configurePhaseCmd)

    int ret = run_cmd(configurePhaseCmd, redirectOutput2FD);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   buildPhaseCmdLength = gmakePathLength + 12U;
    char     buildPhaseCmd[buildPhaseCmdLength];
    snprintf(buildPhaseCmd, buildPhaseCmdLength, "%s --jobs=%u", gmakePath, jobs);

    LOG_RUN_CMD(output2Terminal, logLevel, buildPhaseCmd)

    ret = run_cmd(buildPhaseCmd, redirectOutput2FD);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   installPhaseCmdLength = gmakePathLength + 20U;
    char     installPhaseCmd[installPhaseCmdLength];
    snprintf(installPhaseCmd, installPhaseCmdLength, "%s install", gmakePath);

    LOG_RUN_CMD(output2Terminal, logLevel, installPhaseCmd)

    return run_cmd(installPhaseCmd, redirectOutput2FD);
}

static int setup_liblzma(const char * gmakePath, size_t gmakePathLength, const char * setupDir, size_t setupDirLength, unsigned int jobs, Python3SetupLogLevel logLevel, int redirectOutput2FD, bool output2Terminal) {
    size_t   configurePhaseCmdLength = setupDirLength + 38U;
    char     configurePhaseCmd[configurePhaseCmdLength];
    snprintf(configurePhaseCmd, configurePhaseCmdLength, "./configure --prefix=%s --enable-static %s", setupDir, (logLevel == Python3SetupLogLevel_silent) ? "--silent" : "");

    LOG_RUN_CMD(output2Terminal, logLevel, configurePhaseCmd)

    int ret = run_cmd(configurePhaseCmd, redirectOutput2FD);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (chdir("src/liblzma") != 0) {
        perror("src/liblzma");
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   buildPhaseCmdLength = gmakePathLength + 12U;
    char     buildPhaseCmd[buildPhaseCmdLength];
    snprintf(buildPhaseCmd, buildPhaseCmdLength, "%s --jobs=%u", gmakePath, jobs);

    LOG_RUN_CMD(output2Terminal, logLevel, buildPhaseCmd)

    ret = run_cmd(buildPhaseCmd, redirectOutput2FD);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   installPhaseCmdLength = gmakePathLength + 20U;
    char     installPhaseCmd[installPhaseCmdLength];
    snprintf(installPhaseCmd, installPhaseCmdLength, "%s install", gmakePath);

    LOG_RUN_CMD(output2Terminal, logLevel, installPhaseCmd)

    return run_cmd(installPhaseCmd, redirectOutput2FD);
}

static int cmakew(const char * cmakePath, size_t cmakePathLength, const char * configurePhaseExtraOptions, size_t configurePhaseExtraOptionsLength, const char * setupDir, size_t setupDirLength, unsigned int jobs, Python3SetupLogLevel logLevel, int redirectOutput2FD, bool output2Terminal) {
    size_t   configurePhaseCmdLength = cmakePathLength + setupDirLength + configurePhaseExtraOptionsLength + 124U;
    char     configurePhaseCmd[configurePhaseCmdLength];
    snprintf(configurePhaseCmd, configurePhaseCmdLength, "%s -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_INSTALL_PREFIX=%s -DEXPAT_SHARED_LIBS=OFF -DCMAKE_VERBOSE_MAKEFILE=%s %s -S . -B build.d", cmakePath, setupDir, (logLevel >= Python3SetupLogLevel_verbose) ? "ON" : "OFF", configurePhaseExtraOptions);

    LOG_RUN_CMD(output2Terminal, logLevel, configurePhaseCmd)

    int ret = run_cmd(configurePhaseCmd, redirectOutput2FD);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   buildPhaseCmdLength = cmakePathLength + 30U;
    char     buildPhaseCmd[buildPhaseCmdLength];
    snprintf(buildPhaseCmd, buildPhaseCmdLength, "%s --build build.d -- --jobs=%u", cmakePath, jobs);

    LOG_RUN_CMD(output2Terminal, logLevel, buildPhaseCmd)

    ret = run_cmd(buildPhaseCmd, redirectOutput2FD);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   installPhaseCmdLength = cmakePathLength + 20U;
    char     installPhaseCmd[installPhaseCmdLength];
    snprintf(installPhaseCmd, installPhaseCmdLength, "%s --install build.d", cmakePath);

    LOG_RUN_CMD(output2Terminal, logLevel, installPhaseCmd)

    return run_cmd(installPhaseCmd, redirectOutput2FD);
}

static int configurew(const char * gmakePath, size_t gmakePathLength, const char * configurePhaseExtraOptions, size_t configurePhaseExtraOptionsLength, const char * setupDir, size_t setupDirLength, unsigned int jobs, Python3SetupLogLevel logLevel, int redirectOutput2FD, bool output2Terminal) {
    size_t   configurePhaseCmdLength = setupDirLength + configurePhaseExtraOptionsLength + 64U;
    char     configurePhaseCmd[configurePhaseCmdLength];
    snprintf(configurePhaseCmd, configurePhaseCmdLength, "./configure --prefix=%s %s --enable-static --enable-rpath %s", setupDir, (logLevel == Python3SetupLogLevel_silent) ? "--silent" : "", configurePhaseExtraOptions);

    LOG_RUN_CMD(output2Terminal, logLevel, configurePhaseCmd)

    int ret = run_cmd(configurePhaseCmd, redirectOutput2FD);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   buildPhaseCmdLength = gmakePathLength + 12U;
    char     buildPhaseCmd[buildPhaseCmdLength];
    snprintf(buildPhaseCmd, buildPhaseCmdLength, "%s --jobs=%u", gmakePath, jobs);

    LOG_RUN_CMD(output2Terminal, logLevel, buildPhaseCmd)

    ret = run_cmd(buildPhaseCmd, redirectOutput2FD);

    if (ret != PYTHON3_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   installPhaseCmdLength = gmakePathLength + 20U;
    char     installPhaseCmd[installPhaseCmdLength];
    snprintf(installPhaseCmd, installPhaseCmdLength, "%s install", gmakePath);

    LOG_RUN_CMD(output2Terminal, logLevel, installPhaseCmd)

    return run_cmd(installPhaseCmd, redirectOutput2FD);
}

typedef struct {
    const char * name;
    const char * src_url;
    const char * src_sha;
} Package;

static int python3_setup_install_the_given_package(Package package, const char * python3SetupDownloadsDir, size_t python3SetupDownloadsDirLength, const char * python3SetupInstallingRootDir, size_t python3SetupInstallingRootDirLength, const char * setupDir, size_t setupDirLength, Python3SetupLogLevel logLevel, unsigned int jobs, struct stat st, const char * cmakePath, size_t cmakePathLength, const char * gmakePath, size_t gmakePathLength, bool output2Terminal, int redirectOutput2FD, SysInfo sysinfo) {
    size_t   packageInstallingDirLength = python3SetupInstallingRootDirLength + strlen(package.name);
    char     packageInstallingDir[packageInstallingDirLength];
    snprintf(packageInstallingDir, packageInstallingDirLength, "%s/%s", python3SetupInstallingRootDir, package.name);

    if (stat(packageInstallingDir, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            if (rm_r(packageInstallingDir, logLevel >= Python3SetupLogLevel_verbose) != 0) {
                perror(packageInstallingDir);
                return PYTHON3_SETUP_ERROR;
            }
        } else {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", packageInstallingDir);
            return PYTHON3_SETUP_ERROR;
        }
    }

    if (mkdir(packageInstallingDir, S_IRWXU) != 0) {
        perror(packageInstallingDir);
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    int ret = python3_setup_download_and_uncompress(package.src_url, package.src_sha, python3SetupDownloadsDir, python3SetupDownloadsDirLength, packageInstallingDir, logLevel);

    if (ret != PYTHON3_SETUP_OK) {
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   cdSrcCmdLength = packageInstallingDirLength + 4U;
    char     cdSrcCmd[cdSrcCmdLength];
    snprintf(cdSrcCmd, cdSrcCmdLength, "cd %s", packageInstallingDir);

    LOG_RUN_CMD(true, logLevel, cdSrcCmd)

    if (chdir(packageInstallingDir) != 0) {
        perror(packageInstallingDir);
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (strcmp(package.name, "perl") == 0) {
        return setup_perl(gmakePath, gmakePathLength, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal);
    } else if (strcmp(package.name, "openssl") == 0) {
        return setup_openssl(gmakePath, gmakePathLength, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal, sysinfo);
    } else if (strcmp(package.name, "zlib") == 0) {
        size_t   setupPkgconfigDirLength = setupDirLength + 14U;
        char     setupPkgconfigDir[setupPkgconfigDirLength + 1U];
        snprintf(setupPkgconfigDir,setupPkgconfigDirLength + 1U, "%s/lib/pkgconfig", setupDir);

        size_t   configurePhaseExtraOptionsLength = setupPkgconfigDirLength + 24U;
        char     configurePhaseExtraOptions[configurePhaseExtraOptionsLength + 1U];
        snprintf(configurePhaseExtraOptions,configurePhaseExtraOptionsLength + 1U, "-DINSTALL_PKGCONFIG_DIR=%s", setupPkgconfigDir);

        return cmakew(cmakePath, cmakePathLength, configurePhaseExtraOptions, configurePhaseExtraOptionsLength, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal);
    } else if (strcmp(package.name, "xz") == 0) {
        return setup_liblzma(gmakePath, gmakePathLength, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal);
    } else if (strcmp(package.name, "libbz2") == 0) {
        return cmakew(cmakePath, cmakePathLength, "-DINSTALL_EXECUTABLES=OFF -DINSTALL_LIBRARIES=ON", 48U, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal);
    } else if (strcmp(package.name, "expat") == 0) {
        return cmakew(cmakePath, cmakePathLength, "-DEXPAT_BUILD_DOCS=OFF -DEXPAT_BUILD_TESTS=OFF -DEXPAT_BUILD_FUZZERS=OFF -DEXPAT_BUILD_EXAMPLES=OFF -DEXPAT_BUILD_TOOLS=OFF", 123U, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal);
    } else if (strcmp(package.name, "gdbm") == 0) {
        return configurew(gmakePath, gmakePathLength, "--enable-libgdbm-compat --without-readline", 42U, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal);
    } else if (strcmp(package.name, "sqlite") == 0) {
        return configurew(gmakePath, gmakePathLength, "--disable-editline --disable-readline", 37U, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal);
    } else if (strcmp(package.name, "libffi") == 0) {
        return configurew(gmakePath, gmakePathLength, "--disable-symvers --disable-docs", 32U, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal);
    } else if (strcmp(package.name, "ncurses") == 0) {
        size_t   setupPkgconfigDirLength = setupDirLength + 14U;
        char     setupPkgconfigDir[setupPkgconfigDirLength + 1U];
        snprintf(setupPkgconfigDir,setupPkgconfigDirLength + 1U, "%s/lib/pkgconfig", setupDir);

        size_t   configurePhaseExtraOptionsLength = setupPkgconfigDirLength + setupDirLength + 380U;
        char     configurePhaseExtraOptions[configurePhaseExtraOptionsLength];
        snprintf(configurePhaseExtraOptions, configurePhaseExtraOptionsLength, "--enable-const --enable-widec --enable-termcap --enable-warnings --enable-pc-files --enable-ext-mouse --enable-ext-colors --disable-stripping --disable-assertions --disable-gnat-projects --disable-echo --without-tests --without-debug --without-valgrind --without-ada --with-pkg-config-libdir=%s --with-terminfo-dirs=%s/share/terminfo:/etc/terminfo:/lib/terminfo:/usr/share/terminfo", setupPkgconfigDir, setupDir);

        return configurew(gmakePath, gmakePathLength, configurePhaseExtraOptions, configurePhaseExtraOptionsLength, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal);
    } else if (strcmp(package.name, "readline") == 0) {
        size_t   configurePhaseExtraOptionsLength = setupDirLength + 34U;
        char     configurePhaseExtraOptions[configurePhaseExtraOptionsLength];
        snprintf(configurePhaseExtraOptions, configurePhaseExtraOptionsLength, "--enable-multibyte --with-curses=%s", setupDir);

        return configurew(gmakePath, gmakePathLength, configurePhaseExtraOptions, configurePhaseExtraOptionsLength, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal);
    } else if (strcmp(package.name, "python3") == 0) {
        //size_t   strLength = (setupIncludeDirLength << 1) + (setupLibDirLength << 1) + 69U;
        //char     str[strLength + 1U];
        //snprintf(str,strLength + 1U, "zlib zlibmodule.c -I%s -L%s -l:libz.a\n_bz2 _bz2module.c -I%s -L%s -l:libbz2.a", setupIncludeDir, setupLibDir, setupIncludeDir, setupLibDir);

        int fd = open("Modules/Setup.localxxxx", O_WRONLY | O_CREAT, 0666);

        if (fd < 0) {
            perror("Modules/Setup.local");
            return PYTHON3_SETUP_ERROR;
        }

        if (write(fd, "1", 1) < 0) {
            perror("Modules/Setup.local");
            close(fd);
            return PYTHON3_SETUP_ERROR;
        }

        close(fd);

        /////////////////////////////////////////////////////////////////////////

        if (unsetenv("PYTHONHOME") < 0) {
            perror("unsetenv PYTHONHOME");
            return PYTHON3_SETUP_ERROR;
        }

        if (unsetenv("PYTHONPATH") < 0) {
            perror("unsetenv PYTHONPATH");
            return PYTHON3_SETUP_ERROR;
        }

        /////////////////////////////////////////////////////////////////////////

        size_t   configurePhaseExtraOptionsLength = setupDirLength + 174U;
        char     configurePhaseExtraOptions[configurePhaseExtraOptionsLength];
        snprintf(configurePhaseExtraOptions, configurePhaseExtraOptionsLength, "--with-system-expat --with-system-ffi --with-openssl=%s --with-ensurepip=yes --with-lto --enable-ipv6 --enable-shared --enable-loadable-sqlite-extensions --disable-profiling", setupDir);

        return configurew(gmakePath, gmakePathLength, configurePhaseExtraOptions, configurePhaseExtraOptionsLength, setupDir, setupDirLength, jobs, logLevel, redirectOutput2FD, output2Terminal);
    }
}

static int python3_setup_setup_internal(const char * setupDir, Python3SetupConfig config, Python3SetupLogLevel logLevel, unsigned int jobs, SysInfo sysinfo) {
    bool output2Terminal = isatty(STDOUT_FILENO);

    const char * const userHomeDir = getenv("HOME");

    if (userHomeDir == NULL) {
        return PYTHON3_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t userHomeDirLength = strlen(userHomeDir);

    if (userHomeDirLength == 0U) {
        return PYTHON3_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   python3SetupHomeDirLength = userHomeDirLength + 18U;
    char     python3SetupHomeDir[python3SetupHomeDirLength];
    snprintf(python3SetupHomeDir, python3SetupHomeDirLength, "%s/.python3-setup", userHomeDir);

    struct stat st;

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

    //////////////////////////////////////////////////////////////////////////////

    size_t   defaultSetupDirLength = python3SetupHomeDirLength + 11U;
    char     defaultSetupDir[defaultSetupDirLength];
    snprintf(defaultSetupDir, defaultSetupDirLength, "%s/python3", python3SetupHomeDir);

    if (setupDir == NULL) {
        setupDir = defaultSetupDir;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   setupDirLength = strlen(setupDir);

    size_t   setupBinDirLength = setupDirLength + 4U;
    char     setupBinDir[setupBinDirLength + 1U];
    snprintf(setupBinDir,setupBinDirLength + 1U, "%s/bin", setupDir);

    size_t   setupLibDirLength = setupDirLength + 4U;
    char     setupLibDir[setupLibDirLength + 1U];
    snprintf(setupLibDir,setupLibDirLength + 1U, "%s/lib", setupDir);

    size_t   setupPkgconfigDirLength = setupLibDirLength + 10U;
    char     setupPkgconfigDir[setupPkgconfigDirLength + 1U];
    snprintf(setupPkgconfigDir,setupPkgconfigDirLength + 1U, "%s/pkgconfig", setupLibDir);

    size_t   setupIncludeDirLength = setupDirLength + 8U;
    char     setupIncludeDir[setupIncludeDirLength + 1U];
    snprintf(setupIncludeDir,setupIncludeDirLength + 1U, "%s/include", setupDir);

    //////////////////////////////////////////////////////////////////////////////

    if (setenv("PKG_CONFIG_LIBDIR", setupPkgconfigDir, 1) != 0) {
        perror("PKG_CONFIG_LIBDIR");
        return PYTHON3_SETUP_ERROR;
    }

    if (setenv("PKG_CONFIG_PATH", setupPkgconfigDir, 1) != 0) {
        perror("PKG_CONFIG_PATH");
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    const char * const PATH = getenv("PATH");

    if ((PATH == NULL) || (strcmp(PATH, "") == 0)) {
        return PYTHON3_SETUP_ERROR_ENV_PATH_NOT_SET;
    }

    size_t   newPATHLength = setupBinDirLength + strlen(PATH) + 2U;
    char     newPATH[newPATHLength];
    snprintf(newPATH, newPATHLength, "%s:%s", setupBinDir, PATH);

    if (setenv("PATH", newPATH, 1) != 0) {
        perror("PATH");
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   cppFlagsLength = (setupIncludeDirLength << 1) + 15U;
    char     cppFlags[cppFlagsLength];
    snprintf(cppFlags, cppFlagsLength, "-I%s -I%s/ncursesw", setupIncludeDir, setupIncludeDir);

    if (setenv("CPPFLAGS", cppFlags, 1) != 0) {
        perror("CPPFLAGS");
        return PYTHON3_SETUP_ERROR;
    }

    ///////////////////////////////////////////////////////

    if (logLevel == Python3SetupLogLevel_very_verbose) {
        size_t   ldFlagsLength = (setupDirLength << 1) + 30U;
        char     ldFlags[ldFlagsLength];
        snprintf(ldFlags, ldFlagsLength, "-L%s/lib -Wl,-rpath,%s/lib -Wl,-v", setupDir, setupDir);

        if (setenv("LDFLAGS", ldFlags, 1) != 0) {
            perror("LDFLAGS");
            return PYTHON3_SETUP_ERROR;
        }
    } else {
        size_t   ldFlagsLength = (setupDirLength << 1) + 23U;
        char     ldFlags[ldFlagsLength];
        snprintf(ldFlags, ldFlagsLength, "-L%s/lib -Wl,-rpath,%s/lib", setupDir, setupDir);

        if (setenv("LDFLAGS", ldFlags, 1) != 0) {
            perror("LDFLAGS");
            return PYTHON3_SETUP_ERROR;
        }
    }

    ///////////////////////////////////////////////////////

    if (logLevel == Python3SetupLogLevel_very_verbose) {
        if (setenv("CFLAGS", "-fPIC -v", 1) != 0) {
            perror("CFLAGS");
            return PYTHON3_SETUP_ERROR;
        }
    } else {
        if (setenv("CFLAGS", "-fPIC", 1) != 0) {
            perror("CFLAGS");
            return PYTHON3_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    if (setenv("GDBM_CFLAGS", cppFlags, 1) != 0) {
        perror("GDBM_CFLAGS");
        return PYTHON3_SETUP_ERROR;
    }

    size_t   ldFlagsForGDBMLength = setupLibDirLength + 10U;
    char     ldFlagsForGDBM[ldFlagsForGDBMLength];
    snprintf(ldFlagsForGDBM, ldFlagsForGDBMLength, "-L%s -lgdbm", setupLibDir);

    if (setenv("GDBM_LIBS", ldFlagsForGDBM, 1) != 0) {
        perror("GDBM_LIBS");
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (setenv("ZLIB_CFLAGS", cppFlags, 1) != 0) {
        perror("ZLIB_CFLAGS");
        return PYTHON3_SETUP_ERROR;
    }

    size_t   ldFlagsForLibzLength = setupLibDirLength + 7U;
    char     ldFlagsForLibz[ldFlagsForLibzLength];
    snprintf(ldFlagsForLibz, ldFlagsForLibzLength, "-L%s -lz", setupLibDir);

    if (setenv("ZLIB_LIBS", ldFlagsForLibz, 1) != 0) {
        perror("ZLIB_LIBS");
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (setenv("BZIP2_CFLAGS", cppFlags, 1) != 0) {
        perror("BZIP2_CFLAGS");
        return PYTHON3_SETUP_ERROR;
    }

    size_t   ldFlagsForLibbz2Length = setupLibDirLength + 10U;
    char     ldFlagsForLibbz2[ldFlagsForLibbz2Length];
    snprintf(ldFlagsForLibbz2, ldFlagsForLibbz2Length, "-L%s -lbz2", setupLibDir);

    if (setenv("BZIP2_LIBS", ldFlagsForLibbz2, 1) != 0) {
        perror("BZIP2_LIBS");
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (setenv("LIBLZMA_CFLAGS", cppFlags, 1) != 0) {
        perror("LIBLZMA_CFLAGS");
        return PYTHON3_SETUP_ERROR;
    }

    size_t   ldFlagsForLIBLZMALength = setupLibDirLength + 10U;
    char     ldFlagsForLIBLZMA[ldFlagsForLIBLZMALength];
    snprintf(ldFlagsForLIBLZMA, ldFlagsForLIBLZMALength, "-L%s -llzma", setupLibDir);

    if (setenv("LIBLZMA_LIBS", ldFlagsForLIBLZMA, 1) != 0) {
        perror("LIBLZMA_LIBS");
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (setenv("LIBSQLITE3_CFLAGS", cppFlags, 1) != 0) {
        perror("LIBSQLITE3_CFLAGS");
        return PYTHON3_SETUP_ERROR;
    }

    size_t   ldFlagsForLIBSQLITE3Length = setupLibDirLength + 19U;
    char     ldFlagsForLIBSQLITE3[ldFlagsForLIBSQLITE3Length];
    snprintf(ldFlagsForLIBSQLITE3, ldFlagsForLIBSQLITE3Length, "-L%s -l:libsqlite3.a", setupLibDir);

    if (setenv("LIBSQLITE3_LIBS", ldFlagsForLIBSQLITE3, 1) != 0) {
        perror("LIBSQLITE3_LIBS");
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (setenv("CMAKE_GENERATOR", "Unix Makefiles", 1) != 0) {
        perror("CMAKE_GENERATOR");
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (setenv("CMAKE_GENERATOR", "Unix Makefiles", 1) != 0) {
        perror("CMAKE_GENERATOR");
        return PYTHON3_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   python3SetupDownloadsDirLength = python3SetupHomeDirLength + 11U;
    char     python3SetupDownloadsDir[python3SetupDownloadsDirLength];
    snprintf(python3SetupDownloadsDir, python3SetupDownloadsDirLength, "%s/downloads", python3SetupHomeDir);

    if (stat(python3SetupDownloadsDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", python3SetupDownloadsDir);
            return PYTHON3_SETUP_ERROR;
        }
    } else {
        if (mkdir(python3SetupDownloadsDir, S_IRWXU) != 0) {
            perror(python3SetupDownloadsDir);
            return PYTHON3_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   python3SetupInstallingRootDirLength = python3SetupHomeDirLength + 12U;
    char     python3SetupInstallingRootDir[python3SetupInstallingRootDirLength];
    snprintf(python3SetupInstallingRootDir, python3SetupInstallingRootDirLength, "%s/installing", python3SetupHomeDir);

    if (stat(python3SetupInstallingRootDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", python3SetupInstallingRootDir);
            return PYTHON3_SETUP_ERROR;
        }
    } else {
        if (mkdir(python3SetupInstallingRootDir, S_IRWXU) != 0) {
            perror(python3SetupInstallingRootDir);
            return PYTHON3_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    const char * binUrlGmake = NULL;
    const char * binShaGmake = NULL;

    if (strcmp(sysinfo.kind, "linux") == 0) {
        if (strcmp(sysinfo.arch, "x86_64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-linux-x86_64.tar.xz";
            binShaGmake = "b68f0033f4163bd94af6fb93b281c61fc02bc4af2cc7e80e74722dbf4c639dd3";
        } else if (strcmp(sysinfo.arch, "aarch64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-linux-aarch64.tar.xz";
            binShaGmake = "8ba11716b9d473dbc100cd87474d381bd2bcb7822cc029b50e5a8307c6695691";
        } else if (strcmp(sysinfo.arch, "ppc64le") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-linux-ppc64le.tar.xz";
            binShaGmake = "635c8e41683e318f39a81b964deac2ae1fa736009dc05a04f1110393fa0c9480";
        } else if (strcmp(sysinfo.arch, "s390x") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-linux-s390x.tar.xz";
            binShaGmake = "4e25857f78bb0a1932cf5e0de1ad7b424a42875775d174753362c3af61ceeb0d";
        }
    } else if (strcmp(sysinfo.kind, "darwin") == 0) {
        if (strcmp(sysinfo.arch, "x86_64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-macos11.0-x86_64.tar.xz";
            binShaGmake = "f22660038bc9e318fc37660f406767fe3e2a0ccc205defaae3f4b2bc0708e3a9";
        } else if (strcmp(sysinfo.arch, "arm64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-macos11.0-arm64.tar.xz";
            binShaGmake = "41680f6d1270497f1a3c717ac6150b4239b44430cfbfde4b9f51ff4d4dd1d52c";
        }
    } else if (strcmp(sysinfo.kind, "freebsd") == 0) {
        if (strcmp(sysinfo.arch, "amd64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-freebsd-amd64.tar.xz";
            binShaGmake = "8bab8e9b83afc8d8e08a4406b2167f8f66efd05fa4d4ba4f8c2b14a543860888";
        }
    } else if (strcmp(sysinfo.kind, "openbsd") == 0) {
        if (strcmp(sysinfo.arch, "amd64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-openbsd-amd64.tar.xz";
            binShaGmake = "a7d61765f08d536942c6894c0d81fb7e7052906aa2590586237ada8d09cbdf45";
        }
    } else if (strcmp(sysinfo.kind, "netbsd") == 0) {
        if (strcmp(sysinfo.arch, "amd64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-netbsd-amd64.tar.xz";
            binShaGmake = "8ba11716b9d473dbc100cd87474d381bd2bcb7822cc029b50e5a8307c6695691";
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    int redirectOutput2FD = -1;

    if (logLevel < Python3SetupLogLevel_verbose) {
        size_t   logFilePathLength = python3SetupInstallingRootDirLength + 9U;
        char     logFilePath[logFilePathLength];
        snprintf(logFilePath, logFilePathLength, "%s/log.txt", python3SetupInstallingRootDir);

        redirectOutput2FD = open("/dev/null", O_CREAT | O_TRUNC | O_WRONLY, 0666);

        if (redirectOutput2FD < 0) {
            perror(NULL);
            return PYTHON3_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    int STDERR_FILENO2 = STDERR_FILENO;

    if (logLevel == Python3SetupLogLevel_silent) {
        STDERR_FILENO2 = dup(STDERR_FILENO);

        if (STDERR_FILENO2 < 0) {
            perror(NULL);
            close(redirectOutput2FD);
            return PYTHON3_SETUP_ERROR;
        }

        if (dup2(redirectOutput2FD, STDOUT_FILENO) < 0) {
            perror(NULL);
            close(STDERR_FILENO2);
            close(redirectOutput2FD);
            return PYTHON3_SETUP_ERROR;
        }

        if (dup2(redirectOutput2FD, STDERR_FILENO) < 0) {
            perror(NULL);
            close(STDERR_FILENO2);
            close(redirectOutput2FD);
            return PYTHON3_SETUP_ERROR;
        }

        close(redirectOutput2FD);
        redirectOutput2FD = -1;
    }

    //////////////////////////////////////////////////////////////////////////////

    unsigned int stepN = 1U;

    int ret = PYTHON3_SETUP_OK;

    //////////////////////////////////////////////////////////////////////////////

    size_t   defaultGmakePathLength = setupBinDirLength + 7U;
    char     defaultGmakePath[defaultGmakePathLength];
    snprintf(defaultGmakePath, defaultGmakePathLength, "%s/gmake", setupBinDir);

    //////////////////////////////////////////////////////////////////////////////

    char * gmakePath = NULL;

    size_t gmakePathLength = 0U;

    bool   gmakePathNeedsToBeFreed;

    if (binUrlGmake == NULL) {
        gmakePathNeedsToBeFreed = true;

        LOG_STEP(output2Terminal, logLevel, stepN++, "finding gmake")

        switch (exe_lookup("gmake", &gmakePath, &gmakePathLength)) {
            case  0:
                break;
            case -1:
                perror("gmake");
                return PYTHON3_SETUP_ERROR;
            case -2:
                return PYTHON3_SETUP_ERROR_ENV_PATH_NOT_SET;
            case -3:
                return PYTHON3_SETUP_ERROR_ENV_PATH_NOT_SET;
            default:
                return PYTHON3_SETUP_ERROR;
        }

        if (gmakePath == NULL) {
            switch (exe_lookup("make", &gmakePath, &gmakePathLength)) {
                case  0:
                    break;
                case -1:
                    perror("make");
                    return PYTHON3_SETUP_ERROR;
                case -2:
                    return PYTHON3_SETUP_ERROR_ENV_PATH_NOT_SET;
                case -3:
                    return PYTHON3_SETUP_ERROR_ENV_PATH_NOT_SET;
                default:
                    return PYTHON3_SETUP_ERROR;
            }
        }

        if (gmakePath == NULL) {
            fprintf(stderr, "neither 'gmake' nor 'make' command was found in PATH.\n");
            return PYTHON3_SETUP_ERROR;
        }
    } else {
        gmakePathNeedsToBeFreed = false;
        gmakePath =       defaultGmakePath;
        gmakePathLength = defaultGmakePathLength - 1U;

        LOG_STEP(output2Terminal, logLevel, stepN++, "installing gmake")

        ret = python3_setup_download_and_uncompress(binUrlGmake, binShaGmake, python3SetupDownloadsDir, python3SetupDownloadsDirLength, setupDir, logLevel);

        if (ret != PYTHON3_SETUP_OK) {
            return ret;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    char * cmakePath = NULL;

    size_t cmakePathLength = 0U;

    bool   cmakePathNeedsToBeFreed;

    if (true) {
        cmakePathNeedsToBeFreed = true;

        LOG_STEP(output2Terminal, logLevel, stepN++, "finding cmake")

        switch (exe_lookup("cmake", &cmakePath, &cmakePathLength)) {
            case  0:
                if (cmakePath == NULL) {
                    if (gmakePathNeedsToBeFreed) {
                        free(gmakePath);
                    }
                    fprintf(stderr, "'cmake' command not found in PATH.\n");
                    return PYTHON3_SETUP_ERROR;
                }
                break;
            case -1:
                perror("cmake");
                if (gmakePathNeedsToBeFreed) {
                    free(gmakePath);
                }
                return PYTHON3_SETUP_ERROR;
            case -2:
                if (gmakePathNeedsToBeFreed) {
                    free(gmakePath);
                }
                return PYTHON3_SETUP_ERROR_ENV_PATH_NOT_SET;
            case -3:
                if (gmakePathNeedsToBeFreed) {
                    free(gmakePath);
                }
                return PYTHON3_SETUP_ERROR_ENV_PATH_NOT_SET;
            default:
                if (gmakePathNeedsToBeFreed) {
                    free(gmakePath);
                }
                return PYTHON3_SETUP_ERROR;
        }
    }
 
    //////////////////////////////////////////////////////////////////////////////

    Package packages[12];

    packages[0] = (Package){ "perl",    config.src_url_perl,    config.src_sha_perl    };
    packages[1] = (Package){ "openssl", config.src_url_openssl, config.src_sha_openssl };
    packages[2] = (Package){ "zlib",    config.src_url_zlib,    config.src_sha_zlib    };
    packages[3] = (Package){ "libbz2",  config.src_url_libbz2,  config.src_sha_libbz2  };
    packages[4] = (Package){ "xz",      config.src_url_xz,      config.src_sha_xz      };
    packages[5] = (Package){ "gdbm",    config.src_url_gdbm,    config.src_sha_gdbm    };
    packages[6] = (Package){ "expat",   config.src_url_expat,   config.src_sha_expat   };
    packages[7] = (Package){ "sqlite",  config.src_url_sqlite,  config.src_sha_sqlite  };
    packages[8] = (Package){ "libffi",  config.src_url_libffi,  config.src_sha_libffi  };
    packages[9] = (Package){ "ncurses",  config.src_url_ncurses,  config.src_sha_ncurses  };
    packages[10] = (Package){ "readline",  config.src_url_readline,  config.src_sha_readline  };
    packages[11] = (Package){ "python3", config.src_url_python3, config.src_sha_python3 };

    for (unsigned int i = 0U; i < 12U; i++) {
        Package package = packages[i];

        if (logLevel != Python3SetupLogLevel_silent) { \
            fprintf(stderr, "\n%s=>> STEP %d : installing %s%s\n", COLOR_PURPLE, stepN++, package.name, COLOR_OFF);
        }

        ret = python3_setup_install_the_given_package(package, python3SetupDownloadsDir, python3SetupDownloadsDirLength, python3SetupInstallingRootDir, python3SetupInstallingRootDirLength, setupDir, setupDirLength, logLevel, jobs, st, cmakePath, cmakePathLength, gmakePath, gmakePathLength, output2Terminal, redirectOutput2FD, sysinfo);

        if (ret != PYTHON3_SETUP_OK) {
            goto finalize;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    LOG_STEP(output2Terminal, logLevel, stepN++, "generating receipt.yml")

    ret = python3_setup_write_receipt(setupDir, setupDirLength, binUrlGmake, binShaGmake, config, sysinfo);

    if (ret != PYTHON3_SETUP_OK) {
        goto finalize;
    }

    //////////////////////////////////////////////////////////////////////////////

    fprintf(stderr, "\n%s[✔] successfully setup.%s\n", COLOR_GREEN, COLOR_OFF);
    fprintf(stderr, "\n%s🔔  to use this, run %s'export PATH=\"%s:$PATH\"'%s in your terminal.%s\n\n", COLOR_YELLOW, COLOR_OFF, setupBinDir, COLOR_YELLOW, COLOR_OFF);

    //////////////////////////////////////////////////////////////////////////////

    //if (rm_r(python3SetupInstallingRootDir, logLevel >= Python3SetupLogLevel_verbose) != 0) {
    //    perror(python3SetupInstallingRootDir);
    //    ret = PYTHON3_SETUP_ERROR;
    //}

finalize:
    if (gmakePathNeedsToBeFreed) {
        free(gmakePath);
    }

    if (cmakePathNeedsToBeFreed) {
        free(cmakePath);
    }

    return ret;
}

int python3_setup_setup(const char * configFilePath, const char * setupDir, Python3SetupLogLevel logLevel, unsigned int jobs) {
    SysInfo sysinfo = {0};

    if (sysinfo_make(&sysinfo) < 0) {
        perror(NULL);
        return PYTHON3_SETUP_ERROR;
    }

    if (jobs == 0U) {
        jobs = sysinfo.ncpu;
    }

    //////////////////////////////////////////////////////////////////////////////

    Python3SetupConfig config = {0};

    config.src_url_python3 = DEFAULT_SRC_URL_PYTHON3;
    config.src_sha_python3 = DEFAULT_SRC_SHA_PYTHON3;

    config.src_url_readline = DEFAULT_SRC_URL_READLINE;
    config.src_sha_readline = DEFAULT_SRC_SHA_READLINE;

    config.src_url_ncurses = DEFAULT_SRC_URL_NCURSES;
    config.src_sha_ncurses = DEFAULT_SRC_SHA_NCURSES;

    config.src_url_openssl = DEFAULT_SRC_URL_OPENSSL;
    config.src_sha_openssl = DEFAULT_SRC_SHA_OPENSSL;

    config.src_url_sqlite = DEFAULT_SRC_URL_SQLITE;
    config.src_sha_sqlite = DEFAULT_SRC_SHA_SQLITE;

    config.src_url_libffi = DEFAULT_SRC_URL_LIBFFI;
    config.src_sha_libffi = DEFAULT_SRC_SHA_LIBFFI;

    config.src_url_libbz2 = DEFAULT_SRC_URL_LIBBZ2;
    config.src_sha_libbz2 = DEFAULT_SRC_SHA_LIBBZ2;

    config.src_url_expat = DEFAULT_SRC_URL_EXPAT;
    config.src_sha_expat = DEFAULT_SRC_SHA_EXPAT;

    config.src_url_gdbm = DEFAULT_SRC_URL_GDBM;
    config.src_sha_gdbm = DEFAULT_SRC_SHA_GDBM;

    config.src_url_zlib = DEFAULT_SRC_URL_ZLIB;
    config.src_sha_zlib = DEFAULT_SRC_SHA_ZLIB;

    config.src_url_xz = DEFAULT_SRC_URL_XZ;
    config.src_sha_xz = DEFAULT_SRC_SHA_XZ;

    config.src_url_perl = DEFAULT_SRC_URL_PERL;
    config.src_sha_perl = DEFAULT_SRC_SHA_PERL;

    Python3SetupConfig * userSpecifiedConfig = NULL;

    int ret = PYTHON3_SETUP_OK;

    if (configFilePath != NULL) {
        ret = python3_setup_config_parse(configFilePath, &userSpecifiedConfig);

        if (ret != PYTHON3_SETUP_OK) {
            goto finalize;
        }
 
        if (userSpecifiedConfig->src_url_python3 != NULL) {
            config.src_url_python3 = userSpecifiedConfig->src_url_python3;
        }

        if (userSpecifiedConfig->src_sha_python3 != NULL) {
            config.src_sha_python3 = userSpecifiedConfig->src_sha_python3;
        }
  
        if (userSpecifiedConfig->src_url_readline != NULL) {
            config.src_url_readline = userSpecifiedConfig->src_url_readline;
        }

        if (userSpecifiedConfig->src_sha_readline != NULL) {
            config.src_sha_readline = userSpecifiedConfig->src_sha_readline;
        }
 
        if (userSpecifiedConfig->src_url_ncurses != NULL) {
            config.src_url_ncurses = userSpecifiedConfig->src_url_ncurses;
        }

        if (userSpecifiedConfig->src_sha_ncurses != NULL) {
            config.src_sha_ncurses = userSpecifiedConfig->src_sha_ncurses;
        }

        if (userSpecifiedConfig->src_url_openssl != NULL) {
            config.src_url_openssl = userSpecifiedConfig->src_url_openssl;
        }

        if (userSpecifiedConfig->src_sha_openssl != NULL) {
            config.src_sha_openssl = userSpecifiedConfig->src_sha_openssl;
        }

        if (userSpecifiedConfig->src_url_perl != NULL) {
            config.src_url_perl = userSpecifiedConfig->src_url_perl;
        }

        if (userSpecifiedConfig->src_sha_perl != NULL) {
            config.src_sha_perl = userSpecifiedConfig->src_sha_perl;
        }

        if (userSpecifiedConfig->src_url_sqlite != NULL) {
            config.src_url_sqlite = userSpecifiedConfig->src_url_sqlite;
        }

        if (userSpecifiedConfig->src_sha_sqlite != NULL) {
            config.src_sha_sqlite = userSpecifiedConfig->src_sha_sqlite;
        }

        if (userSpecifiedConfig->src_url_libffi != NULL) {
            config.src_url_libffi = userSpecifiedConfig->src_url_libffi;
        }

        if (userSpecifiedConfig->src_sha_libffi != NULL) {
            config.src_sha_libffi = userSpecifiedConfig->src_sha_libffi;
        }
 
        if (userSpecifiedConfig->src_url_expat != NULL) {
            config.src_url_expat = userSpecifiedConfig->src_url_expat;
        }

        if (userSpecifiedConfig->src_sha_expat != NULL) {
            config.src_sha_expat = userSpecifiedConfig->src_sha_expat;
        }

        if (userSpecifiedConfig->src_url_gdbm != NULL) {
            config.src_url_gdbm = userSpecifiedConfig->src_url_gdbm;
        }

        if (userSpecifiedConfig->src_sha_gdbm != NULL) {
            config.src_sha_gdbm = userSpecifiedConfig->src_sha_gdbm;
        }

        if (userSpecifiedConfig->src_url_libbz2 != NULL) {
            config.src_url_libbz2 = userSpecifiedConfig->src_url_libbz2;
        }

        if (userSpecifiedConfig->src_sha_libbz2 != NULL) {
            config.src_sha_libbz2 = userSpecifiedConfig->src_sha_libbz2;
        }

        if (userSpecifiedConfig->src_url_zlib != NULL) {
            config.src_url_zlib = userSpecifiedConfig->src_url_zlib;
        }

        if (userSpecifiedConfig->src_sha_zlib != NULL) {
            config.src_sha_zlib = userSpecifiedConfig->src_sha_zlib;
        }

        if (userSpecifiedConfig->src_url_xz != NULL) {
            config.src_url_xz = userSpecifiedConfig->src_url_xz;
        }

        if (userSpecifiedConfig->src_sha_xz != NULL) {
            config.src_sha_xz = userSpecifiedConfig->src_sha_xz;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    ret = python3_setup_setup_internal(setupDir, config, logLevel, jobs, sysinfo);

finalize:
    sysinfo_free(sysinfo);
    python3_setup_config_free(userSpecifiedConfig);
    return ret;
}
