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
#include "ip.h"

/*  将来的に IPv4 以外のプロトコルにも拡張できるように,  
    各プロトコルの処理をプラグインの形で, 実装する.
    リンクリストで Protocol を走査する.  
*/
struct net_protocol {
    struct net_protocol *next;
    uint16_t type;
    net_protocol_handler_t handler;
};

static struct net_device *devices; // Protocol Stackに登録しているネットワークデバイスのリスト
static struct net_protocol *protocols;

// ネットワークデバイスのオブジェクト割当. 
struct net_device *net_device_alloc(void) 
{
    struct net_device *dev = memory_alloc(sizeof(*dev));
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
    dev->index = index++;
    snprintf(dev->name, sizeof(dev->name), "net%d", dev->index);
    // 巡回リストにする. new_device -> old_device -> devices -> new_device -> ...
    dev->next = devices;
    devices = dev;
    infof("success: dev=%s, type=0x%04x", dev->name, dev->type);
    return 0;
}

// プロトコルスタックに登録しているネットワークデバイスを起動させる関数
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
    // ops から open 関数を呼び出して, 対象デバイスの 固有 open 処理を実施.
    // NULL なら デバイス固有の処理は不在としてスキップ
    if (dev->ops->open) {
        if (dev->ops->open(dev) == -1) {
            errorf("failure dev=%s", dev->name);
            return -1;
        }
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
    /* 固有の デバイス停止処理を実施. */
    if (dev->ops->close) {
        if (dev->ops->close(dev) == -1) {
            errorf("failure, dev=%s", dev->name);
            return -1;
        }
    }
    dev->flags &= ~NET_DEVICE_FLAG_UP;
    return 0;
}

/*  
 * 一般的なネットワークデバイスからの入力は, 通常 割り込みにより処理が起動する.  
 * Args:
 *  @type: 入力パケットのプロトコル識別
 *  @data: 入力パケットのバイト列
 *  @len:  入力パケットのバイト数.
 *  @dev:  入力デバイスへのポインタ. 
 * 
 * Returns:
 *   0: 成功, 失敗. 
*/
int net_input(uint16_t type, const uint8_t *data, size_t len, struct net_device *dev)
{
    debugf("dev=%s, type=0x%04x, len=%zu", dev->name, type, len);
    debugdump(data, len);
    return 0;
}


/* Protocol Stack のハンドラ登録. 
 * @type: プロトコル種別.
 * @handler: そのプロトコルのハンドラ.
*/
int net_protocol_register(uint16_t type, net_protocol_handler_t handler) 
{
    struct net_protocol *proto;
    for (proto = protocols; proto; proto = proto->next) {
        if (proto->type == type) {
            errorf("already registerd, type=0x%04x", proto->type);
            return -1;
        }
    }
    proto = memory_alloc(sizeof(*proto));
    if (!proto) {
        errorf("memory_alloc() failure");
        return -1;
    }
    proto->type = type;
    proto->handler = handler;
    proto->next = protocols;
    protocols = proto;
    infof("success, type=0x%04x", type);
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
int net_device_output(struct net_device *dev, uint16_t type, 
                    const uint8_t *data, size_t len, const void *dst)
{
    debugf("dev=%s, type=%04x, len=%zu", dev->name, dev->type, len);
    debugdump(data, len);
    if (!NET_DEVICE_IS_UP(dev)) { // opened なデバイスか?
        errorf("not opened, dev=%s", dev->name);
        return -1;
    }
    // データサイズが MTU 内に収まっているか?
    if (dev->mtu < len) {
        errorf("too long, dev=%s, mtu=%u, len=%zu", dev->name, dev->mtu, len);
        return -1;
    }
    // デバイス側に出力用 callback が登録されているかチェック
    if (!dev->ops->output) {
        errorf("failure, dev=%s, len=%zu", dev->name, len);
        return -1;
    }

    // 実際にデバイスにデータを流す. 
    if (dev->ops->output(dev, type, data, len, dst) == -1) {
        errorf("failure, dev=%s, len=%zu", dev->name, len);
        return -1;
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
    if (ip_init() == -1) {
        errorf("ip_init() failure");
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
    for (dev = devices; dev; dev = dev->next) {
        infof("dev: %p", dev);
        if (net_device_open(dev) == -1) {
            errorf("net_device_open() failure");
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
    for (dev = devices; dev; dev = dev->next) {
        if (net_device_close(dev) == -1) {
            return -1;
        }
    }
    devices = NULL;
    infof("net_shutdown: success");
    return 0;
}

