#ifndef _NET_H
#define _NET_H

#include <stddef.h>
#include <stdint.h>

#ifndef IFNAMSIZ 
#define IFNAMSIZ 16
#endif /* IFNAMSIZ */

// Device Types 
#define NET_DEVICE_TYPE_DUMMY     0x0000
#define NET_DEVICE_TYPE_LOOPBACK  0x0001
#define NET_DEVICE_TYPE_ETHERNET  0x0002

// Device status flags
#define NET_DEVICE_FLAG_UP        0x0001
#define NET_DEVICE_FLAG_LOOPBACK  0x0010
#define NET_DEVICE_FLAG_BROADCAST 0x0020
#define NET_DEVICE_FLAG_P2P       0x0040
#define NET_DEVICE_NEED_ARP       0x0100

#define NET_DEVICE_ADDR_LEN       16
#define NET_DEVICE_IS_UP(x) ((x)->flags & NET_DEVICE_FLAG_UP)
#define NET_DEVICE_STATE(x) (NET_DEVICE_IS_UP(x) ? "UP": "DOWN")

/** 
 * ネットワークデバイスを管理するための構造体. 
 * 以下の情報を保持する.
 *   デバイスの識別情報
 *   ネットワークデバイスとデータリンクの特性
 *   稼働状態
 *   ハードウェアの物理アドレス.
 * 
 * Linuxカーネルでは, net_device構造体が相当する?
 *  Ref: https://elixir.bootlin.com/linux/v7.2/source/include/linux/netdevice.h#L2149
 */
struct net_device {
    struct net_device *next; // 片方向リンクリスト構造. 次デバイスへのポインタ
    unsigned int index;      // 識別番号.
    char name[IFNAMSIZ];     // デバイス名. 
    uint16_t type;           // デバイスの種別. DUMMY: 0, LOOPBACK: 1, Ethernet: 2
    uint16_t mtu;            // Maximum Transmission Unit
    uint16_t flags;
    uint16_t hlen;
    uint16_t alen;
    uint8_t  addr[NET_DEVICE_ADDR_LEN];
    uint8_t  broadcast[NET_DEVICE_ADDR_LEN];
    struct net_device_ops *ops; // デバイス固有の処理を行う関数ポインタテーブル. 
    void *priv;                 // デバイスドライバが内部で使用する private なデータを
                                // ネットワークデバイスの object に紐付ける. 
};

// callback ハンドラ用構造体
struct net_device_ops {
    int (*open)(struct net_device *dev);   // ネットワークデバイスを起動するための関数
    int (*close)(struct net_device *dev);  // ネットワークデバイスを停止するための関数. 
    int (*output)(struct net_device *dev, uint16_t type, const uint8_t *data, size_t len, const void *dst);
};

// Export 関数
extern struct net_device *net_device_alloc(void);
extern int net_device_register(struct net_device *dev);
extern int net_device_output(struct net_device *dev, uint16_t type, const uint8_t *data, size_t len, const void *dst);
extern int net_input(uint16_t type, const uint8_t *data, size_t len, struct net_device *dev);

extern int net_init(void);
extern int net_run(void);
extern int net_shutdown(void);

#endif /* _NET_H */


