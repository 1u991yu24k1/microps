/*
  プロトコルスタックのメインコード.
*/

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "platform.h"
#include "util.h"
#include "net.h"

static struct net_device *devices; // Protocol Stackに登録しているネットワークデバイスのリスト

// ネットワークデバイスのオブジェクト割当. 
struct net_device *net_device_alloc(void) 
{
    struct net_device *dev;

    dev = memory_alloc(sizeof(*dev));
    if (!dev) {
        errorf("memory_alloc()@%s", __func__);
        goto out;
    }
out:
    return dev;
}


// ネットワークデバイスを Protocol Stackに登録する. 
// @dev: 新しく登録するネットワークデバイス. (caller側で非 NULL をチェックすること)
// Returns:
//  0: On Success
// -1: On Failure
int net_device_register(struct net_device *dev) 
{
    static unsigned int index = 0;
    dev->index++;
    snprintf(dev->name, sizeof(dev->name), "net%d", dev->index);
    // 巡回リストにする. new_device -> old_device -> devices -> new_device -> ...
    dev->next = devices;
    devices = dev;
    infof("success: dev=%s, type=0x%04x", dev->name, dev->type);
    return 0;
}

// ネットワークデバイスを起動させる関数
// @dev: 新しく登録するネットワークデバイス. (caller側で非 NULL をチェックすること)
// Returns:
//  0: On Success
// -1: On Failure
int net_device_open(struct net_device *dev)
{
    infof("dev=%s", dev->name);
    if (NET_DEVICE_IS_UP(dev)) {
        errorf("already opened, dev=%s", dev->name);
        return -1;
    }
    // flag を up にセット
    dev->flags |= NET_DEVICE_FLAG_UP;
    return 0;
}

// ネットワークデバイスを停止させる関数
// @dev: 新しく登録するネットワークデバイス. (caller側で非 NULL をチェックすること)
// Returns:
//  0: On Success
// -1: On Failure
int net_device_close(struct net_device *dev)
{
    infof("dev=%s", dev->name);
    if (!NET_DEVICE_IS_UP(dev)) { // そもそも up していない.
        errorf("not opened", dev->name);
        return -1;
    }
    dev->flags &= ~NET_DEVICE_FLAG_UP;
    return 0;
}

/* ネットワークデバイスからデータを送信する関数
 *  Args:
 *      @dev : 送信に使用するネットワークデバイス.
 *      @type: 送信データのプロトコル種別. 
 *      @data: 送信データのバイト列
 *      @len : 送信データのバイト数
 *      @dst : データリンク上の宛先アドレス (宛先 MAC アドレス) 
    Returns:
        0: 成功, -1: 失敗. 
 */ 
int net_device_output(struct net_device *dev, uint16_t type, const uint8_t *data, size_t len, const void *dst)
{
    debugf("dev=%s, type=%04x, len=%zu", dev->name, dev->type, len);
    debugdump(data, len);
    if (!NET_DEVICE_IS_UP(dev)) { // opened なデバイスか?
        errorf("not opened, dev=%s", dev->name);
        return -1;
    }
    if (dev->mtu < len) {
        
    }
    return 0;
}

/// プロトコルスタックの初期化
int net_init(void)
{
    infof("initialize ...");
    if (platform_init() == -1) {
        errorf("platform_init()@net_init failure");
        return -1;
    }
    infof("net_init: success...");
    return 0;
}

/// プロトコルスタックの起動
int net_run(void)
{
    struct net_device *dev;
    infof("startup...");
    if (platform_init() == -1) {
        errorf("platform_init()@net_run failure");
        return -1;
    }
    /**
     * ここでdevicesからすべての デバイスを open 状態にする.
     * 書籍のコードでは, 
     *  for (dev = devices; dev; dev = dev->next) { ... } となっているが, 
     * net_device_registerの実装的に無限ループになる. そのため, 
     *  for (dev = devices->next; dev != devices; dev = dev->next) { ... } に修正. 
     */ 
    for (dev = devices->next; dev != devices; dev = dev->next) {
        if (net_device_open(dev) == -1) {
            return -1;
        }
    }
    infof(" net_run: success");
    return 0;
}

/// プロトコルスタックの停止.
int net_shutdown(void)
{
    struct net_device *dev = NULL; // 初期化しておく. 
    infof("shutting down...");
    if (platform_shutdown() == -1) {
        errorf("platform_shutdown()@net_shutdown failure");
        return -1;
    }
    /**
     * ここでdevicesからすべての デバイスを open 状態にする.
     * 書籍のコードでは, 
     *  for (dev = devices; dev; dev = dev->next) { ... } となっているが, 
     * net_device_registerの実装的に無限ループになる.
     */ 
    for (dev = devices->next; dev != devices; dev = dev->next) {
        if (net_device_close(dev) == -1) {
            return -1;
        }
    }
    devices = NULL;
    infof("net_shutdown: success");
    return 0;
}

