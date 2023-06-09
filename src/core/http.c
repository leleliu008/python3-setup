#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <curl/curlver.h>

#include "http.h"

//static size_t write_callback(void * ptr, size_t size, size_t nmemb, void * stream) {
//    return fwrite(ptr, size, nmemb, (FILE *)stream);
//}

int http_fetch_to_stream(const char * url, FILE * outputFile, bool verbose, bool showProgress) {
    if (outputFile == NULL) {
        size_t  urlCopyLength = strlen(url) + 1U;
        char    urlCopy[urlCopyLength];
        strncpy(urlCopy, url, urlCopyLength);

        const char * filename = basename(urlCopy);

        outputFile = fopen(filename, "wb");

        if (outputFile == NULL) {
            return -1;
        }
    }

    curl_global_init(CURL_GLOBAL_ALL);

    CURL * curl = curl_easy_init();

    // https://curl.se/libcurl/c/CURLOPT_URL.html
    curl_easy_setopt(curl, CURLOPT_URL, url);

    // https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
    //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    // https://curl.se/libcurl/c/CURLOPT_WRITEDATA.html
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outputFile);

    // https://curl.se/libcurl/c/CURLOPT_FOLLOWLOCATION.html
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // https://curl.se/libcurl/c/CURLOPT_FAILONERROR.html
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    // https://curl.se/libcurl/c/CURLOPT_VERBOSE.html
    curl_easy_setopt(curl, CURLOPT_VERBOSE, verbose ? 1 : 0);

    // https://curl.se/libcurl/c/CURLOPT_NOPROGRESS.html
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, showProgress ? 0: 1L);

    //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    // https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_default_verify_paths.html
    const char * const SSL_CERT_FILE = getenv("SSL_CERT_FILE");

    if ((SSL_CERT_FILE != NULL) && (strcmp(SSL_CERT_FILE, "") != 0)) {
        // https://curl.se/libcurl/c/CURLOPT_CAINFO.html
        curl_easy_setopt(curl, CURLOPT_CAINFO, SSL_CERT_FILE);
    }

    const char * const SSL_CERT_DIR = getenv("SSL_CERT_DIR");

    if ((SSL_CERT_DIR != NULL) && (strcmp(SSL_CERT_DIR, "") != 0)) {
        // https://curl.se/libcurl/c/CURLOPT_CAPATH.html
        curl_easy_setopt(curl, CURLOPT_CAPATH, SSL_CERT_DIR);
    }

    char     userAgent[50];
    snprintf(userAgent, 50, "User-Agent: libcurl-%s", LIBCURL_VERSION);

    struct curl_slist *list = NULL;

    //list = curl_slist_append(list, "Accept: *");
    list = curl_slist_append(list, userAgent);

    // https://curl.se/libcurl/c/CURLOPT_HTTPHEADER.html
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    CURLcode curlcode = curl_easy_perform(curl);
    
    // https://curl.se/libcurl/c/libcurl-errors.html
    if (curlcode != CURLE_OK) {
        fprintf(stderr, "%s\n", curl_easy_strerror(curlcode));
    }

    curl_slist_free_all(list);

    curl_easy_cleanup(curl);

    curl_global_cleanup();

    return curlcode;
}

int http_fetch_to_file(const char * url, const char * outputFilePath, bool verbose, bool showProgress) {
    FILE * file = fopen(outputFilePath, "wb");

    if (file == NULL) {
        return -1;
    }

    int ret = http_fetch_to_stream(url, file, verbose, showProgress);

    fclose(file);

    return ret;
}
