/* Loopback インターフェースを作成する. 
 * 
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "util.h"
#include "net.h"


#define LOOPBACK_MTU UINT16_MAX

static int loopback_output(struct net_device *dev, uint16_t type, 
                    const uint8_t *data, size_t len, const void *dst)
{
    return 0;   
}

// loopback_device に紐づく callback ハンドラ. 
static net_device_ops loopback_ops = {
    .output = loopback_output,
};


struct net_device *loopback_init() 
{
    struct net_device *dev = NULL;
    return dev;
}
