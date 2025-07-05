/*
 * DPP Configurator - Main hostapd Integration
 * Main entry point and initialization for hostapd integration
 */

#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include "../include/dpp_configurator.h"

#ifndef STUB_MODE

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
    ctx->operating_freq = 2412; // デフォルト: Channel 1
    ctx->listening_events = false; // イベントリスニング状態を初期化
    ctx->gas_server_active = false; // GAS サーバーアクティブ状態を初期化
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
int cmd_auth_realtime_events(struct dpp_configurator_ctx *ctx, char *args)
{
    char *interface = NULL;
    char cmd[512];
    char response[4096];
    int ret;

    // 引数解析
    interface = parse_argument(args, "interface");
    if (!interface) {
        printf("Error: interface parameter required\n");
        printf("Usage: auth_realtime_events interface=<ifname>\n");
        return -1;
    }

    printf("=== DPP Real-time Event Monitor ===\n");
    printf("Interface: %s\n", interface);
    printf("Monitoring DPP events in real-time...\n");
    printf("(This will show immediate hostapd responses)\n\n");

    // 1. 現在のDPP状態を表示
    printf("--- Current DPP State ---\n");
    
    snprintf(cmd, sizeof(cmd), "DPP_CONFIGURATOR_GET_KEY 1");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret == 0) {
        printf("Configurator 1 key: %s\n", response);
    }

    snprintf(cmd, sizeof(cmd), "DPP_BOOTSTRAP_GET_URI 1");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret == 0) {
        printf("Bootstrap 1 URI: %s\n", response);
    }

    // 2. DPP認証の詳細状態
    printf("\n--- DPP Authentication Details ---\n");
    
    // 各種DPPコマンドを試行
    const char* dpp_commands[] = {
        "DPP_BOOTSTRAP_INFO 1",
        "DPP_AUTH_STATUS",
        "DPP_CONFIGURATOR_PARAMS 1",
        NULL
    };

    for (int i = 0; dpp_commands[i] != NULL; i++) {
        snprintf(cmd, sizeof(cmd), "%s", dpp_commands[i]);
        ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
        if (ret == 0 && strlen(response) > 0) {
            printf("%s: %s\n", dpp_commands[i], response);
        }
    }

    // 3. hostapdの詳細ログを有効化
    printf("\n--- Enabling Detailed Logging ---\n");
    
    snprintf(cmd, sizeof(cmd), "LOG_LEVEL MSGDUMP");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    printf("Log level set to MSGDUMP: %s\n", response);

    // 4. DPP関連のイベント購読を試行
    snprintf(cmd, sizeof(cmd), "ATTACH");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret == 0) {
        printf("Event subscription: %s\n", response);
    }

    printf("\n=== Monitor hostapd logs in another terminal ===\n");
    printf("Run: sudo journalctl -f -u hostapd\n");
    printf("Or: tail -f /var/log/hostapd.log\n");
    printf("Look for DPP-related messages\n");

    free(interface);
    return 0;
}

// GAS/Configuration関連のコマンド（簡略化版）
int cmd_gas_server_start(struct dpp_configurator_ctx *ctx, char *args)
{
    char *interface = parse_argument(args, "interface");
    if (!interface) {
        printf("Error: interface parameter required\n");
        return -1;
    }

    printf("GAS server functionality integrated with hostapd\n");
    printf("Interface: %s\n", interface);
    printf("hostapd automatically handles GAS Request/Response for DPP\n");
    
    ctx->gas_server_active = true;
    free(interface);
    return 0;
}

int cmd_gas_server_stop(struct dpp_configurator_ctx *ctx, char *args)
{
    (void)args;
    printf("GAS server stopped\n");
    ctx->gas_server_active = false;
    return 0;
}

int cmd_gas_monitor(struct dpp_configurator_ctx *ctx, char *args)
{
    char *interface = parse_argument(args, "interface");
    if (!interface) {
        printf("Error: interface parameter required\n");
        return -1;
    }

    printf("GAS monitoring integrated with hostapd logs\n");
    printf("Monitor hostapd logs for GAS Request/Response messages\n");
    printf("Look for: DPP-CONF-REQ-RX, DPP-CONF-SENT\n");
    
    free(interface);
    return 0;
}

int cmd_config_request_monitor(struct dpp_configurator_ctx *ctx, char *args)
{
    char *interface = parse_argument(args, "interface");
    if (!interface) {
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

#endif /* STUB_MODE */
