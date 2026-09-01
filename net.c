/*
  プロトコルスタックのメインコード.

 */
#include "platform.h"

#include "util.h"

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
    infof("startup...");
    if (platform_init() == -1) {
        errorf("platform_init()@net_run failure");
        return -1;
    }
    infof(" net_run: success");
    return 0;
}

/// プロトコルスタックの停止.
int net_shutdown(void)
{
    infof("shutting down...");
    if (platform_shutdown() == -1) {
        errorf("platform_shutdown()@net_shutdown failure");
        return -1;
    }
    infof("net_shutdown: success");
    return 0;
}
