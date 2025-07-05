/*
 * DPP Configurator - Main hostapd Integration
 * Main entry point and initialization for hostapd integration
 */

#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include "../include/dpp_configurator.h"

// External function declarations
extern int hostapd_cli_send_command(const char *interface, const char *cmd,
                                    char *response, size_t response_size);

// DPP初期化関数（hostapd統合版）
struct dpp_configurator_ctx *dpp_configurator_init(void)
{
    struct dpp_configurator_ctx *ctx;
    struct dpp_global_config dpp_config;

    ctx = os_zalloc(sizeof(*ctx));
    if (!ctx)
        return NULL;

    // DPPグローバル設定初期化
    os_memset(&dpp_config, 0, sizeof(dpp_config));
    dpp_config.cb_ctx = ctx;

    ctx->dpp_global = dpp_global_init(&dpp_config);
    if (!ctx->dpp_global)
    {
        os_free(ctx);
        return NULL;
    }

    ctx->configurator_count = 0;
    ctx->bootstrap_count = 0;
    ctx->verbose = false;
    ctx->current_auth = NULL; // 現在の認証セッションを初期化
    ctx->hapd = NULL;         // hostapd interface (後で設定)
    ctx->wireless_interface = NULL;
    ctx->operating_freq = 2412;          // デフォルト: Channel 6
    ctx->config_request_monitor = false; // Configuration Request監視状態を初期化

    printf("DPP Configurator initialized (hostapd mode)\n");
    printf("Ready for wireless interface integration\n");
    return ctx;
}

// DPP終了処理（hostapd統合版）
void dpp_configurator_deinit(struct dpp_configurator_ctx *ctx)
{
    if (!ctx)
        return;

    // 現在の認証セッションがあればクリーンアップ
    if (ctx->current_auth)
    {
        dpp_auth_deinit(ctx->current_auth);
        ctx->current_auth = NULL;
    }

    // 無線インターフェース情報をクリーンアップ
    if (ctx->wireless_interface)
    {
        free(ctx->wireless_interface);
        ctx->wireless_interface = NULL;
    }

    if (ctx->dpp_global)
    {
        dpp_global_deinit(ctx->dpp_global);
    }

    os_free(ctx);
}

// 残りのコマンド実装（リアルタイム監視など）
// GAS/Configuration関連のコマンド（簡略化版）
int cmd_config_request_monitor(struct dpp_configurator_ctx *ctx, char *args)
{
    char *interface = parse_argument(args, "interface");
    if (!interface)
    {
        printf("Error: interface parameter required\n");
        return -1;
    }

    printf("Configuration Request monitoring active\n");
    printf("hostapd will automatically handle Configuration Request/Response\n");
    printf("Monitor hostapd logs for detailed information\n");

    ctx->config_request_monitor = true;
    free(interface);
    return 0;
}
