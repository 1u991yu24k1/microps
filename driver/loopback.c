/* Loopback インターフェースを作成する. 
 * 
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "util.h"
#include "net.h"


#define LOOPBACK_MTU UINT16_MAX

/*
 * Loopbackデバイスの出力ルーチン. 
 * 通常のネットワークデバイスでは, 出力ルーチン内で, 
 * データリンクのプロトコルのヘッダを追加したフレームを作成したり, デバイスへの書き込みを処理する.
 * しかし, Loopbackデバイスは, 引数で渡されたデータをそのままプロトコル・スタックに折り返す. 
 *  Args:
 *      @dev: ネットワークデバイス.
 *      @type: 送信パケットのプロトコル種別.
 *      @data: 送信パケットのバイト列.
 *      @len : 送信パケットのバイト数.
 *      @dst : データリンク上の宛先アドレス.
 *  Returns:
 *      0: 成功, -1: 失敗. 
*/
static int loopback_output(struct net_device *dev, uint16_t type, 
                    const uint8_t *data, size_t len, const void *dst)
{
    debugf("dev=%s, type=0x%04x, len=%zu", dev->name, type, len);
    debugdump(data, len);
    return net_input(type, data, len, dev); // プロトコル・スタックの入力ルーチンにloopbackする.    
}

// loopback_device に紐づく callback ハンドラ. 
static struct net_device_ops loopback_ops = {
    .output = loopback_output,
};

/* Loopback デバイスの初期化. */
struct net_device *loopback_init(void)
{
    struct net_device *dev = net_device_alloc();
    if (!dev) {
        errorf("net_device_alloc() failure");
        return NULL;
    }
    dev->type = NET_DEVICE_TYPE_LOOPBACK;
    dev->mtu = LOOPBACK_MTU;
    dev->flags = NET_DEVICE_FLAG_LOOPBACK;
    dev->hlen = 0; /* non header */
    dev->alen = 0; /* non address */
    dev->ops = &loopback_ops;

    if (net_device_register(dev) == -1) {
        errorf("net_device_registre() failure");
        return NULL;
    }
    infof("success dev=%s", dev->name);
    return dev;
}
