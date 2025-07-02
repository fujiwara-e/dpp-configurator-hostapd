#ifndef DPP_CONFIGURATOR_H
#define DPP_CONFIGURATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef STUB_MODE
// スタブモード: 基本的な型定義
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

// 基本的な定数
#define ETH_ALEN 6
#define SHA256_MAC_LEN 32

// 前方宣言（スタブ）
struct dpp_global;
struct dpp_authentication;
struct dpp_bootstrap_info;
struct dpp_configurator;

#else
// hostapd統合モード: hostapdの型定義を使用
#include "utils/common.h"
#include "utils/eloop.h"
#include "common/dpp.h"
#include "common/dpp_i.h"
#include "crypto/crypto.h"
#include "utils/json.h"
#include "common/gas.h"
#endif

// hostapd address type stub
union hostapd_addr
{
    struct in_addr v4;
    struct in6_addr v6;
};

// hostapd gas types stub
struct gas_query_ap;

// DPP asymmetric key stub
struct dpp_asymmetric_key;

// メイン構造体
struct dpp_configurator_ctx
{
    struct dpp_global *dpp_global;
    int configurator_count;
    int bootstrap_count;
    bool verbose;
};

// コマンド構造体
struct dpp_command
{
    const char *name;
    int (*handler)(struct dpp_configurator_ctx *ctx, char *args);
    const char *help;
};

// 関数プロトタイプ
struct dpp_configurator_ctx *dpp_configurator_init(void);
void dpp_configurator_deinit(struct dpp_configurator_ctx *ctx);
int execute_command(struct dpp_configurator_ctx *ctx, const char *cmd, char *args);

// コマンドハンドラー
int cmd_configurator_add(struct dpp_configurator_ctx *ctx, char *args);
int cmd_bootstrap_gen(struct dpp_configurator_ctx *ctx, char *args);
int cmd_bootstrap_get_uri(struct dpp_configurator_ctx *ctx, char *args);
int cmd_auth_init(struct dpp_configurator_ctx *ctx, char *args);
int cmd_status(struct dpp_configurator_ctx *ctx, char *args);
int cmd_help(struct dpp_configurator_ctx *ctx, char *args);
int cmd_test_bootstrap(struct dpp_configurator_ctx *ctx, char *args);

// ユーティリティ関数
char *parse_argument(char *args, const char *key);
void print_usage(const char *prog_name);

#endif /* DPP_CONFIGURATOR_H */
