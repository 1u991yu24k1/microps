#ifndef _LOOPBACK_H
#define _LOOPBACK_H

#include "net.h"

extern int loopback_output(struct net_device *dev, uint16_t type, 
                    const uint8_t *data, size_t len, const void *dst);
struct net_device *loopback_init(void);
#endif /* _LOOPBACK_H */