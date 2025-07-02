#ifndef DPP_CONFIGURATOR_H
#define DPP_CONFIGURATOR_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// 基本的な型定義
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

// 必要な定数
#define ETH_ALEN 6
#define SHA256_MAC_LEN 32
#define PMKID_LEN 16
#define PMK_LEN 32
#define PMK_LEN_MAX 64
#define SSID_MAX_LEN 32

// hostapd DPP includes (必要最小限)
// struct dpp_global の前方宣言
struct dpp_global;
struct dpp_authentication;
struct dpp_bootstrap_info;
struct dpp_configurator;

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

// ユーティリティ関数
char *parse_argument(char *args, const char *key);
void print_usage(const char *prog_name);

#endif /* DPP_CONFIGURATOR_H */
