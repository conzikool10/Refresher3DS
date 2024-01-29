#include <SDL2/SDL.h>

#include <http/http.h>
#include <http/https.h>
#include <http/util.h>
#include <net/net.h>
#include <sysmodule/sysmodule.h>
#include <stdbool.h>

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
            SDL_Log("Failed to load SSL certificates: %d", ret);
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

        (&autodiscover->ca_list[0])->ptr = autodiscover->cert_buffer;
        (&autodiscover->ca_list[0])->size = cert_size;

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