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
#include "utils/wpabuf.h"
#include "common/dpp.h"
#include "common/dpp_i.h"
#include "crypto/crypto.h"
#include "utils/json.h"
#include "common/gas.h"
#include "common/ieee802_11_defs.h"
#include "common/wpa_ctrl.h"
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

// Forward declarations
struct dpp_configurator_ctx;

// DPP認証イベントハンドラー用の構造体
struct dpp_event_handler
{
    void (*auth_response_received)(struct dpp_configurator_ctx *ctx,
                                   struct dpp_authentication *auth);
    void (*auth_confirm_received)(struct dpp_configurator_ctx *ctx,
                                  struct dpp_authentication *auth);
    void (*config_result_received)(struct dpp_configurator_ctx *ctx,
                                   struct dpp_authentication *auth);
    void (*auth_failed)(struct dpp_configurator_ctx *ctx,
                        struct dpp_authentication *auth,
                        const char *reason);
};

// メイン構造体
struct dpp_configurator_ctx
{
    struct dpp_global *dpp_global;
    int configurator_count;
    int bootstrap_count;
    bool verbose;
    struct dpp_authentication *current_auth; // 現在の認証セッション
    struct dpp_event_handler event_handler;  // イベントハンドラー
#ifndef STUB_MODE
    void *hapd;                  // hostapd interface context (実際の実装用)
    char *wireless_interface;    // 無線インターフェース名
    unsigned int operating_freq; // 動作周波数
    bool listening_events;       // イベントリスニング状態
    bool gas_server_active;      // GAS サーバーアクティブ状態
    bool config_request_monitor; // Configuration Request監視状態
#endif
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
int cmd_dpp_qr_code(struct dpp_configurator_ctx *ctx, char *args);
int cmd_bootstrap_get_uri(struct dpp_configurator_ctx *ctx, char *args);
int cmd_auth_init_real(struct dpp_configurator_ctx *ctx, char *args);
int cmd_auth_status(struct dpp_configurator_ctx *ctx, char *args);
int cmd_auth_monitor(struct dpp_configurator_ctx *ctx, char *args);
int cmd_auth_detailed_monitor(struct dpp_configurator_ctx *ctx, char *args);
int cmd_auth_realtime_events(struct dpp_configurator_ctx *ctx, char *args);
int cmd_auth_control(struct dpp_configurator_ctx *ctx, char *args);
int cmd_test_hostapd(struct dpp_configurator_ctx *ctx, char *args);
int cmd_debug_dpp(struct dpp_configurator_ctx *ctx, char *args);
int cmd_status(struct dpp_configurator_ctx *ctx, char *args);
int cmd_help(struct dpp_configurator_ctx *ctx, char *args);

// GAS/DPP Configuration Request/Response コマンド
int cmd_gas_server_start(struct dpp_configurator_ctx *ctx, char *args);
int cmd_gas_server_stop(struct dpp_configurator_ctx *ctx, char *args);
int cmd_gas_monitor(struct dpp_configurator_ctx *ctx, char *args);
int cmd_config_request_monitor(struct dpp_configurator_ctx *ctx, char *args);

// TX診断コマンド
int cmd_diagnose_tx(struct dpp_configurator_ctx *ctx, char *args);
int cmd_diagnose_wireless(struct dpp_configurator_ctx *ctx, char *args);
int cmd_monitor_tx(struct dpp_configurator_ctx *ctx, char *args);

// Legacy commands
int cmd_auth_init(struct dpp_configurator_ctx *ctx, char *args);

// ユーティリティ関数
char *parse_argument(char *args, const char *key);
void print_usage(const char *prog_name);
char *decode_hex_string(const char *hex_str);
bool is_hex_string(const char *str);

#endif /* DPP_CONFIGURATOR_H */
