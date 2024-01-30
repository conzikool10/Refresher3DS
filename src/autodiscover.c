#include <SDL2/SDL.h>

#include <http/http.h>
#include <http/https.h>
#include <http/util.h>
#include <net/net.h>
#include <sysmodule/sysmodule.h>
#include <stdbool.h>
#include <cJSON.h>

#include "assert.h"
#include "autodiscover.h"

#define HTTP_POOL_SIZE 0x10000
#define SSL_POOL_SIZE 0x40000

int autodiscover_init(autodiscover_t *autodiscover)
{
    int ret;

    // Try to load the network module
    ret = sysModuleLoad(SYSMODULE_NET);
    if (ret < 0)
    {
        SDL_Log("Failed to load NET module: %d", ret);
        return ret;
    }

    // Try to initialize the network
    ret = netInitialize();
    if (ret < 0)
    {
        SDL_Log("Failed to initialize network: %d", ret);
        return ret;
    }

    // Try to load the HTTP module
    ret = sysModuleLoad(SYSMODULE_HTTP);
    if (ret < 0)
    {
        SDL_Log("Failed to load HTTP: %d", ret);
        return ret;
    }

    // Allocate the HTTP data pool
    autodiscover->pool = malloc(HTTP_POOL_SIZE);
    ASSERT_NONZERO(autodiscover->pool, "Failed to allocate HTTP pool");

    // Initialize the HTTP library
    ret = httpInit(autodiscover->pool, HTTP_POOL_SIZE);
    if (ret < 0)
    {
        SDL_Log("Failed to initialize HTTP: %d", ret);
        return ret;
    }

    // Assume HTTPS until proven otherwise
    autodiscover->has_https = true;

    // Try to load the HTTPS module
    ret = sysModuleLoad(SYSMODULE_HTTPS);
    if (ret < 0)
    {
        autodiscover->has_https = false;
        SDL_Log("Failed to load HTTPS: %d", ret);
    }

    // Try to load the SSL module
    ret = sysModuleLoad(SYSMODULE_SSL);
    if (ret < 0)
    {
        autodiscover->has_https = false;
        SDL_Log("Failed to load SSL: %d", ret);
    }

    if (autodiscover->has_https)
    {
        // Allocate the SSL pool
        autodiscover->ssl_pool = malloc(SSL_POOL_SIZE);
        ASSERT_NONZERO(autodiscover->ssl_pool, "Failed to allocate SSL pool");

        // Initialize the SSL library
        ret = sslInit(autodiscover->ssl_pool, SSL_POOL_SIZE);
        if (ret < 0)
        {
            SDL_Log("Failed to initialize SSL: %d", ret);
            goto no_ssl;
        }

        uint32_t cert_size;

        // Load the SSL certificates
        autodiscover->ca_list = (httpsData *)malloc(sizeof(httpsData));
        ret = sslCertificateLoader(SSL_LOAD_CERT_ALL, NULL, 0, &cert_size);
        if (ret < 0)
        {
            SDL_Log("Failed to load SSL certificate size: %d", ret);
            goto no_cert_size_check;
        }

        // Allocate the certificate buffer
        autodiscover->cert_buffer = malloc(cert_size);
        ASSERT_NONZERO(autodiscover->cert_buffer, "Failed to allocate certificate buffer");

        // Load the SSL certificates
        ret = sslCertificateLoader(SSL_LOAD_CERT_ALL, autodiscover->cert_buffer, cert_size, NULL);
        if (ret < 0)
        {
            SDL_Log("Failed to load SSL certificates: %d", ret);
            goto no_cert_load;
        }

        autodiscover->ca_list->ptr = autodiscover->cert_buffer;
        autodiscover->ca_list->size = cert_size;

        ret = httpsInit(1, autodiscover->ca_list);
        if (ret < 0)
        {
            SDL_Log("Failed to initialize HTTPS: %d", ret);
            goto no_https;
        }

        // If everything checks out, skip past the HTTPS deinit stuff
        goto https_good;

    no_https:
    no_cert_load:
        // Free the certificate buffer if loading the certificates failed
        free(autodiscover->cert_buffer);

    no_cert_size_check:
        // Free the certificate list if loading the certificates failed
        free(autodiscover->ca_list);

    no_ssl:
        // Free the SSL pool if initializing SSL failed
        free(autodiscover->ssl_pool);

        autodiscover->has_https = false;
    }
https_good:

    return 0;
}

#define HTTP_USER_AGENT "RefresherPS3/1.0"

int autodiscover_execute(autodiscover_t *autodiscover, char *orig_url, char **server_brand, char **patch_url, bool *patch_digest)
{
    int ret = 0;

    httpClientId client = 0;
    // Create a new HTTP client
    ret = httpCreateClient(&client);
    if (ret < 0)
    {
        SDL_Log("Failed to create HTTP client: %d", ret);
        return ret;
    }

    // Set the HTTP client options
    httpClientSetConnTimeout(client, 10 * 1000 * 1000);
    httpClientSetUserAgent(client, HTTP_USER_AGENT);
    httpClientSetAutoRedirect(client, 1);

    size_t orig_url_length = strlen(orig_url);

    size_t i = 0;
    bool found_protocol = false;
    while (orig_url[i] != '\0')
    {
        if (orig_url[i] == '/' && orig_url[i + 1] == '/')
        {
            found_protocol = true;
            break;
        }

        i++;
    }

    // Count the number of bytes before the first slash, ignoring double slashes
    uint32_t url_length = 0;
    for (size_t i = 0; i < orig_url_length; i++)
    {
        if (orig_url[i] == '/')
        {
            if (orig_url[i + 1] == '/')
            {
                i++;
                url_length += 2;

                continue;
            }

            break;
        }

        url_length++;
    }

    // Offset of bytes to leave to fit a new protocol into
    int protocol_offset = found_protocol ? 0 : strlen("http://");

    // Allocate a buffer for the URL
    char *url = (char *)malloc(protocol_offset + url_length + 1 + strlen("/autodiscover"));

    // If we did not find a protocol in the original URL, copy one in
    if (!found_protocol)
        strcpy(url, "http://");

    // Copy the URL into the buffer
    memcpy(url + protocol_offset, orig_url, url_length);
    // Append the autodiscover path
    strcpy(url + url_length + protocol_offset, "/autodiscover");

    SDL_Log("URL: %s", url);

    uint32_t uri_pool_size;
    httpUri uri;
    ret = httpUtilParseUri(&uri, url, NULL, 0, &uri_pool_size);
    if (ret < 0)
    {
        SDL_Log("Failed to calculate URI size: %x", ret);
        goto uri_size_calc_fail;
    }

    void *uri_pool = malloc(uri_pool_size);
    ASSERT_NONZERO(uri_pool, "Failed to allocate URI pool");

    ret = httpUtilParseUri(&uri, url, uri_pool, uri_pool_size, 0);
    if (ret < 0)
    {
        SDL_Log("Failed to parse URI: %x", ret);
        goto uri_parse_fail;
    }

    httpTransId trans;
    ret = httpCreateTransaction(&trans, client, HTTP_METHOD_GET, &uri);
    if (ret < 0)
    {
        SDL_Log("Failed to create HTTP transaction: %x", ret);
        goto transaction_creation_fail;
    }

    ret = httpSendRequest(trans, NULL, 0, NULL);
    if (ret < 0)
    {
        SDL_Log("Failed to send HTTP request: %x", ret);
        goto request_send_fail;
    }

    uint64_t content_length;
    ret = httpResponseGetContentLength(trans, &content_length);
    if (ret < 0)
    {
        SDL_Log("Failed to get HTTP response content length: %x", ret);
        goto content_length_fail;
    }

    int32_t response_code = 0;
    ret = httpResponseGetStatusCode(trans, &response_code);
    if (ret < 0)
    {
        SDL_Log("Failed to get HTTP response status code: %x", ret);
        goto status_code_fail;
    }

    // If the response code is 200, an error definitely occurred, so exit out
    if (response_code != HTTP_STATUS_CODE_OK)
    {
        ret = -1;

        SDL_Log("HTTP response status code was not 200: %x", response_code);
        goto status_code_fail;
    }

    // If the response code is 200, but the content length is 0, an error definitely occurred, so exit out
    if (content_length == 0)
    {
        ret = -1;

        SDL_Log("HTTP response content length was 0");
        goto content_length_fail;
    }

    // Allocate a buffer for the response
    char *response_buffer = (char *)malloc(content_length + 1);
    ASSERT_NONZERO(response_buffer, "Failed to allocate response buffer");
    // Null terminate the response buffer
    response_buffer[content_length] = '\0';

    uint32_t bytes_read = 1;
    uint32_t total_read = 0;
    while (bytes_read > 0)
    {
        ret = httpRecvResponse(trans, response_buffer + total_read, content_length - total_read, &bytes_read);

        if (ret > 0)
            break;

        total_read += bytes_read;
    }

    SDL_Log("Response: %s", response_buffer);

    cJSON *parsed = cJSON_ParseWithLength(response_buffer, content_length);
    if (parsed == NULL)
    {
        ret = -1;
        SDL_Log("Failed to parse JSON, %s", cJSON_GetErrorPtr());
        goto json_parse_fail;
    }

    // parse out `serverBrand`, `url`, and `usesCustomDigestKey`
    cJSON *server_brand_value = cJSON_GetObjectItem(parsed, "serverBrand");
    cJSON *server_url_value = cJSON_GetObjectItem(parsed, "url");
    cJSON *uses_custom_digest_key_value = cJSON_GetObjectItem(parsed, "usesCustomDigestKey");

    if (server_brand_value == NULL || server_url_value == NULL || uses_custom_digest_key_value == NULL)
    {
        ret = -1;

        SDL_Log("Failed to parse JSON, missing one of the required fields.");
        goto json_interpret_fail;
    }

    if (cJSON_GetStringValue(server_brand_value) == NULL ||
        cJSON_GetStringValue(server_url_value) == NULL ||
        cJSON_GetNumberValue(uses_custom_digest_key_value) == NAN)
    {
        ret = -1;

        SDL_Log("Failed to parse JSON, one of the required fields was not the correct type.");
        goto json_interpret_fail;
    }

    // Copy the server brand
    (*server_brand) = strdup(cJSON_GetStringValue(server_brand_value));
    ASSERT_NONZERO(*server_brand, "Failed to copy server brand");

    // Copy the server URL
    (*patch_url) = strdup(cJSON_GetStringValue(server_url_value));
    ASSERT_NONZERO(*patch_url, "Failed to copy server URL");

    (*patch_digest) = (int)cJSON_GetNumberValue(uses_custom_digest_key_value);

    // If we made it here, no errors occurred
    ret = 0;

json_interpret_fail:
    cJSON_Delete(parsed);

json_parse_fail:
    free(response_buffer);

status_code_fail:
content_length_fail:
request_send_fail:
    httpDestroyTransaction(trans);

transaction_creation_fail:
uri_parse_fail:
    free(uri_pool);

uri_size_calc_fail:
    ASSERT_ZERO(httpDestroyClient(client), "Failed to destroy HTTP client");

    return ret;
}