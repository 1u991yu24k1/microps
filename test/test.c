// テストプログラム.
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "net.h"
#include "util.h"

#include "test.h"

#define DUMMY_NET_DEV_NAME "dummy_nic"

/* Dummy 用 ネットワークデバイスを割当, ダミーパラメータをセットしてからプロトコルスタックに登録する.  */
struct net_device *dummy_init(void) 
{
    struct net_device *dev = net_device_alloc();
    if (!dev) {
        errorf("net_device_alloc() failure");
        return NULL;
    }
    dev->type = NET_DEVICE_TYPE_DUMMY;
    dev->mtu = 128;
    dev->hlen = 0; // Header長.
    dev->alen = 0; // アドレス長.
    infof("set dummy nic name");
    strncpy(dev->name, DUMMY_NET_DEV_NAME, strlen(DUMMY_NET_DEV_NAME) + 1); 
    if (net_device_register(dev) == -1) {
        errorf("net_device_register() failure");
        return NULL;
    }
    infof("success, dev=%s", dev->name);
    return dev;
}

static volatile sig_atomic_t terminate;

static struct net_device *dev;

static void on_signal(int signum)
{
    (void)signum;
    terminate = 1;
}

// プロトコル・スタックの事前準備関数
// 1. Signal Handler (SIGINT) のみハンドリング.
// 2. 
static int setup(void)
{
    struct sigaction sa = { 0 };

    sa.sa_handler = on_signal;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        errorf("sigaction() %s", strerror(errno));
        return -1;
    }
    infof("setup protocol stack...");
    if (net_init() == -1) {
        errorf("net_init() failure");
        return -1;
    }
    
    dev = dummy_init();
    if (!dev) {
        errorf("dummy_init() failure");
        return -1;
    }

    if (net_run() == -1) {
        errorf("net_run() failure");
        return -1;
    }
    return 0;
}

static int cleanup(void)
{
    infof("cleanup protocol stack...");
    if (net_shutdown() == -1) {
        errorf("net_shutdown() failure");
        return -1;
    }
    return 0;
}

// startupやdescruct処理を除いた main部分. 
static int app_main(void) { 
    debugf( "press Ctrl+C to terminate");
    while (!terminate) {
        int output_stat = net_device_output(dev, 0x0800, test_data, sizeof(test_data), NULL);
        if (output_stat == -1) {
            errorf("net_device_output() failure");
            break;
        }
        sleep(1);
    }
    debugf("terminated");
    return 0; 
}


// テストコードのエントリポイント
int main(void)
{
    int ret;

    if (setup() == -1) {
        errorf("setup() failure");
        return -1;
    }
    ret = app_main();
    if (cleanup() == -1) {
        errorf("cleanup() failure");
        return -1;
    }
    return ret;
}
