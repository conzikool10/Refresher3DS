#include <http/http.h>
#include <http/https.h>
#include <http/util.h>
#include <net/net.h>
#include <sysmodule/sysmodule.h>
#include <stdbool.h>

typedef struct autodiscover_t
{
    void *pool;
    void *ssl_pool;
    void *cert_buffer;
    httpsData *ca_list;
    bool has_https;
} autodiscover_t;

int autodiscover_init(autodiscover_t *autodiscover);