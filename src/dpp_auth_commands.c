/*
 * DPP Configurator - Authentication Commands
 * DPP authentication and monitoring commands
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
extern char *load_bootstrap_uri(int id);
extern char *encode_hex_string(const char *str);

// DPP認証を実際のhostapdで実行
static int dpp_execute_real_auth(struct dpp_configurator_ctx *ctx,
                                 const char *interface,
                                 int peer_id, int configurator_id,
                                 const char *conf_type, const char *ssid, const char *pass,
                                 const char *matter_pin, const char *conf_json)
{
    char cmd[512];
    char response[MAX_RESPONSE_SIZE];
    int ret;

    (void)ctx; // 未使用パラメータ警告を回避

    printf("Executing DPP authentication via hostapd interface: %s\n", interface);

    // Step 1: hostapd にコンフィギュレーターを追加
    printf("Step 1: Adding configurator to hostapd...\n");
    snprintf(cmd, sizeof(cmd), "DPP_CONFIGURATOR_ADD curve=prime256v1");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret < 0)
    {
        printf("Failed to add configurator to hostapd\n");
        return -1;
    }

    // レスポンスから実際のコンフィギュレーターIDを取得
    int hostapd_configurator_id = atoi(response);
    printf("hostapd configurator ID: %d\n", hostapd_configurator_id);

    // Step 2: hostapd にブートストラップ情報を追加
    printf("Step 2: Adding bootstrap info to hostapd...\n");
    char *saved_uri = load_bootstrap_uri(peer_id);
    if (!saved_uri)
    {
        printf("Error: Cannot find bootstrap URI for peer ID %d\n", peer_id);
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "DPP_QR_CODE %s", saved_uri);
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    free(saved_uri);

    if (ret < 0)
    {
        printf("Failed to add bootstrap info to hostapd\n");
        return -1;
    }

    // レスポンスから実際のピアIDを取得
    int hostapd_peer_id = atoi(response);
    printf("hostapd peer ID: %d\n", hostapd_peer_id);

    // Step 3: DPP auth_init コマンドを構築（hostapdのIDを使用）
    printf("Step 3: Initiating DPP authentication...\n");
    
    // JSON設定が提供されている場合は、それを使用
    if (conf_json) {
        printf("Using JSON configuration: %s\n", conf_json);
        snprintf(cmd, sizeof(cmd),
                 "DPP_AUTH_INIT peer=%d configurator=%d conf_json='%s'",
                 hostapd_peer_id, hostapd_configurator_id, conf_json);
    }
    else if (ssid && pass)
    {
        // SSIDとパスワードを16進数エンコード
        char *ssid_hex = NULL;
        char *pass_hex = NULL;

        // SSIDが既に16進数でない場合はエンコード
        if (is_hex_string(ssid))
        {
            ssid_hex = strdup(ssid);
            printf("Using SSID as hex: %s\n", ssid_hex);
        }
        else
        {
            ssid_hex = encode_hex_string(ssid);
            printf("Encoded SSID '%s' to hex: %s\n", ssid, ssid_hex);
        }

        // パスワードが既に16進数でない場合はエンコード
        if (is_hex_string(pass))
        {
            pass_hex = strdup(pass);
            printf("Using password as hex: %s\n", pass_hex);
        }
        else
        {
            pass_hex = encode_hex_string(pass);
            printf("Encoded password '%s' to hex: %s\n", pass, pass_hex);
        }

        if (ssid_hex && pass_hex)
        {
            if (matter_pin && strlen(matter_pin) == 8)
            {
                snprintf(cmd, sizeof(cmd),
                         "DPP_AUTH_INIT peer=%d configurator=%d conf=%s ssid=%s pass=%s matter_pin=%s",
                         hostapd_peer_id, hostapd_configurator_id, conf_type, ssid_hex, pass_hex, matter_pin);
                printf("Including Matter PIN: %s\n", matter_pin);
            }
            else
            {
                snprintf(cmd, sizeof(cmd),
                         "DPP_AUTH_INIT peer=%d configurator=%d conf=%s ssid=%s pass=%s",
                         hostapd_peer_id, hostapd_configurator_id, conf_type, ssid_hex, pass_hex);
            }
        }
        else
        {
            printf("Error: Failed to encode SSID or password to hex\n");
            if (ssid_hex)
                free(ssid_hex);
            if (pass_hex)
                free(pass_hex);
            return -1;
        }

        free(ssid_hex);
        free(pass_hex);
    }
    else
    {
        if (matter_pin && strlen(matter_pin) == 8)
        {
            snprintf(cmd, sizeof(cmd),
                     "DPP_AUTH_INIT peer=%d configurator=%d conf=%s matter_pin=%s",
                     hostapd_peer_id, hostapd_configurator_id, conf_type, matter_pin);
            printf("Including Matter PIN: %s\n", matter_pin);
        }
        else
        {
            snprintf(cmd, sizeof(cmd),
                     "DPP_AUTH_INIT peer=%d configurator=%d conf=%s",
                     hostapd_peer_id, hostapd_configurator_id, conf_type);
        }
    }

    printf("Sending to hostapd: %s\n", cmd);

    // hostapdにコマンド送信
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret < 0)
    {
        printf("Failed to communicate with hostapd on interface %s\n", interface);
        printf("Make sure hostapd is running with DPP support and control interface enabled.\n");
        printf("hostapd.conf should include:\n");
        printf("  ctrl_interface=/var/run/hostapd\n");
        printf("  ctrl_interface_group=sudo\n");
        return -1;
    }

    printf("hostapd response: %s\n", response);

    // 応答を解析
    if (strstr(response, "OK") || strstr(response, "Authentication initiated"))
    {
        printf("✓ DPP Authentication successfully initiated via hostapd\n");
        return 0;
    }
    else if (strstr(response, "FAIL"))
    {
        printf("✗ DPP Authentication failed: %s\n", response);
        return -1;
    }
    else
    {
        printf("? Unknown response from hostapd: %s\n", response);
        return -1;
    }
}

// 実際の無線通信によるauth_init（hostapd統合版）
int cmd_auth_init_real(struct dpp_configurator_ctx *ctx, char *args)
{
    int peer_id = -1;
    int configurator_id = -1;
    char *conf_type = NULL;
    char *ssid = NULL;
    char *pass = NULL;
    char *interface = NULL;
    char *matter_pin = NULL;
    char *conf_json = NULL;
    int ret = -1;

    if (ctx->verbose)
    {
        printf("Processing auth_init_real command: %s\n", args);
    }

    // 引数解析
    char *peer_str = parse_argument(args, "peer");
    char *configurator_str = parse_argument(args, "configurator");
    conf_type = parse_argument(args, "conf");
    ssid = parse_argument(args, "ssid");
    pass = parse_argument(args, "pass");
    interface = parse_argument(args, "interface");
    matter_pin = parse_argument(args, "matter_pin");
    conf_json = parse_argument(args, "conf_json");

    if (peer_str)
    {
        peer_id = atoi(peer_str);
        free(peer_str);
    }

    if (configurator_str)
    {
        configurator_id = atoi(configurator_str);
        free(configurator_str);
    }

    // 必須パラメータチェック
    if (peer_id < 0 || configurator_id < 0 || !interface)
    {
        printf("Error: peer, configurator, and interface parameters required\n");
        printf("Usage: auth_init_real peer=<id> configurator=<id> interface=<ifname> [conf=<type>] [ssid=<ssid>] [pass=<pass>] [matter_pin=<8-digit-pin>] [conf_json=\"<json>\"]\n");
        printf("Example (traditional): auth_init_real peer=1 configurator=1 conf=sta-psk interface=wlan0 ssid=MyWiFi pass=secret123 matter_pin=12345678\n");
        printf("Example (JSON): auth_init_real peer=1 configurator=1 interface=wlan0 conf_json='{\"wi-fi_tech\":\"infra\",\"discovery\":{\"ssid\":\"MyWiFi\"},\"cred\":{\"akm\":\"psk\",\"pass\":\"secret123\"},\"matter\":{\"pinCode\":\"12345678\"}}'\n");
        printf("Note: Use single quotes around JSON to avoid shell interpretation issues\n");
        goto cleanup;
    }

    // JSON設定と従来の設定の混在チェック
    if (conf_json && (conf_type || ssid || pass || matter_pin))
    {
        printf("Error: Cannot mix conf_json with traditional parameters (conf, ssid, pass, matter_pin)\n");
        printf("Use either conf_json OR traditional parameters, not both\n");
        goto cleanup;
    }

    // 従来の設定の場合のみconf_typeが必須
    if (!conf_json && !conf_type)
    {
        printf("Error: conf parameter required when not using conf_json\n");
        goto cleanup;
    }

    // Matter PINの検証（従来の設定の場合のみ）
    if (matter_pin && !conf_json)
    {
        if (!is_valid_matter_pin(matter_pin))
        {
            printf("Error: Matter PIN must be exactly 8 digits (0-9 only)\n");
            printf("Example: matter_pin=12345678\n");
            goto cleanup;
        }
        printf("Matter PIN validation: OK\n");
    }

    printf("Real DPP Authentication Parameters:\n");
    printf("  Interface: %s\n", interface);
    printf("  Peer ID: %d\n", peer_id);
    printf("  Configurator ID: %d\n", configurator_id);
    
    if (conf_json)
    {
        printf("  JSON Configuration: %s\n", conf_json);
    }
    else
    {
        printf("  Configuration type: %s\n", conf_type);
        if (ssid)
            printf("  SSID: %s\n", ssid);
        if (pass)
            printf("  Password: %s\n", pass);
        if (matter_pin)
            printf("  Matter PIN: %s\n", matter_pin);
    }

    // 実際のhostapd経由でDPP認証を実行
    ret = dpp_execute_real_auth(ctx, interface, peer_id, configurator_id,
                                conf_type, ssid, pass, matter_pin, conf_json);

    if (ret == 0)
    {
        printf("\n✓ DPP Authentication initiated successfully via hostapd\n");
        printf("Monitor hostapd logs for authentication progress:\n");
        printf("  tail -f /var/log/hostapd.log\n");
        printf("  or use: hostapd_cli -i %s status\n", interface);
    }
    else
    {
        printf("\n✗ Failed to initiate DPP Authentication\n");
        printf("Troubleshooting steps:\n");
        printf("1. Verify hostapd is running: systemctl status hostapd\n");
        printf("2. Check interface exists: ip link show %s\n", interface);
        printf("3. Verify DPP support: hostapd_cli -i %s help | grep DPP\n", interface);
        printf("4. Check bootstrap info: hostapd_cli -i %s dpp_bootstrap_get_uri %d\n", interface, peer_id);
    }

cleanup:
    if (conf_type)
        free(conf_type);
    if (ssid)
        free(ssid);
    if (pass)
        free(pass);
    if (interface)
        free(interface);
    if (matter_pin)
        free(matter_pin);
    if (conf_json)
        free(conf_json);

    return ret;
}

// auth_status コマンド（hostapd統合版）
int cmd_auth_status(struct dpp_configurator_ctx *ctx, char *args)
{
    (void)args; // 未使用パラメータの警告を避ける

    printf("DPP Authentication Status:\n");

    if (!ctx->current_auth)
    {
        printf("  No active authentication session\n");
        return 0;
    }

    struct dpp_authentication *auth = ctx->current_auth;

    printf("  Active authentication session:\n");
    printf("    Initiator: %s\n", auth->initiator ? "YES" : "NO");
    printf("    Configurator: %s\n", auth->configurator ? "YES" : "NO");
    printf("    Peer version: %d\n", auth->peer_version);
    printf("    Waiting auth response: %s\n", auth->waiting_auth_resp ? "YES" : "NO");
    printf("    Waiting auth confirm: %s\n", auth->waiting_auth_conf ? "YES" : "NO");
    printf("    Authentication success: %s\n", auth->auth_success ? "YES" : "NO");
    printf("    Configuration success: %s\n", auth->waiting_conf_result ? "PENDING" : "COMPLETE");

    if (auth->peer_bi)
    {
        printf("    Peer bootstrap ID: %d\n", auth->peer_bi->id);
    }

    if (auth->conf)
    {
        printf("    Configurator object: PRESENT\n");
    }
    else
    {
        printf("    Configurator object: MISSING\n");
    }

    return 0;
}
