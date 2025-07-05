/*
 * DPP Configurator - Monitoring Commands
 * DPP event monitoring and diagnostic commands
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/dpp_configurator.h"

#define MAX_RESPONSE_SIZE 4096

// External functions
extern int hostapd_cli_send_command(const char *interface, const char *cmd,
                                    char *response, size_t response_size);

// DPP認証の進行状況チェック
static int dpp_check_auth_progress(struct dpp_configurator_ctx *ctx,
                                   const char *interface)
{
    char cmd[512];
    char response[MAX_RESPONSE_SIZE];
    int ret;

    // DPP認証の状態を確認
    snprintf(cmd, sizeof(cmd), "STATUS");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret < 0)
    {
        printf("Failed to check authentication progress\n");
        return -1;
    }

    // レスポンスを解析してDPP関連の情報を探す
    if (strstr(response, "dpp_auth_ok_on_ack"))
    {
        printf("✓ DPP Authentication confirmed\n");
        return 1;
    }
    else if (strstr(response, "dpp_auth_"))
    {
        printf("? DPP Authentication in progress\n");
        return 0;
    }

    return -1;
}

// DPP認証完了チェック（Configuration Request/Response含む）
static int dpp_check_config_completion(struct dpp_configurator_ctx *ctx,
                                       const char *interface)
{
    char cmd[512];
    char response[MAX_RESPONSE_SIZE];
    int ret;

    // hostapdのログレベルを上げてDPPイベントを監視
    snprintf(cmd, sizeof(cmd), "LOG_LEVEL DEBUG");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));

    // DPP関連の統計情報を取得
    snprintf(cmd, sizeof(cmd), "DPP_BOOTSTRAP_INFO 1");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret == 0 && strstr(response, "OK"))
    {
        printf("Bootstrap info available: %s\n", response);
    }

    // DPP設定送信状態を確認
    snprintf(cmd, sizeof(cmd), "STATUS");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret == 0)
    {
        // Configuration完了の兆候を検索
        if (strstr(response, "DPP_CONF_SENT") ||
            strstr(response, "dpp_conf_sent=1") ||
            strstr(response, "conf_status=0"))
        {
            printf("✓ DPP Configuration sent successfully\n");
            return 1;
        }
        else if (strstr(response, "DPP_CONF_REQ_RX"))
        {
            printf("? DPP Configuration Request received, processing...\n");
            return 0;
        }
    }

    return -1;
}

// DPP認証イベントループ（非同期処理）
static int dpp_auth_event_loop(struct dpp_configurator_ctx *ctx,
                               const char *interface,
                               int timeout_seconds)
{
    int elapsed = 0;
    int check_interval = 2; // 2秒間隔でチェック
    int progress_status = 0;
    int config_status = 0;

    printf("Monitoring DPP authentication progress (timeout: %ds)...\n",
           timeout_seconds);

    while (elapsed < timeout_seconds)
    {
        sleep(check_interval);
        elapsed += check_interval;

        // 認証進行状況をチェック
        progress_status = dpp_check_auth_progress(ctx, interface);

        if (progress_status == 1)
        {
            printf("✓ DPP Authentication completed successfully!\n");

            // Configuration完了もチェック
            config_status = dpp_check_config_completion(ctx, interface);
            if (config_status == 1)
            {
                printf("✓ DPP Configuration completed successfully!\n");
                return 0;
            }
            else
            {
                printf("? Waiting for DPP Configuration completion...\n");
            }
        }
        else if (progress_status == 0)
        {
            printf("... DPP Authentication in progress (%ds elapsed)\n", elapsed);
        }
        else
        {
            printf("Checking authentication status... (%ds elapsed)\n", elapsed);
        }
    }

    printf("✗ DPP Authentication timeout after %d seconds\n", timeout_seconds);
    return -1;
}

// DPP認証の詳細監視コマンド
int cmd_auth_monitor(struct dpp_configurator_ctx *ctx, char *args)
{
    char *interface = NULL;
    int timeout = 30; // デフォルト30秒
    char *timeout_str = NULL;
    int ret;

    // 引数解析
    interface = parse_argument(args, "interface");
    timeout_str = parse_argument(args, "timeout");

    if (timeout_str)
    {
        timeout = atoi(timeout_str);
        free(timeout_str);
    }

    if (!interface)
    {
        printf("Error: interface parameter required\n");
        printf("Usage: auth_monitor interface=<ifname> [timeout=<seconds>]\n");
        return -1;
    }

    printf("Monitoring DPP authentication events on interface %s\n", interface);
    printf("Timeout: %d seconds\n", timeout);

    // DPP認証イベントループを開始
    ret = dpp_auth_event_loop(ctx, interface, timeout);

    free(interface);
    return ret;
}
