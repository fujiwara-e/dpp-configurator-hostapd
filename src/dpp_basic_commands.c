/*
 * DPP Configurator - Basic Commands
 * Basic DPP operations: configurator_add, qr_code, bootstrap_get_uri
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dpp_configurator.h"

// External functions
extern int hostapd_cli_send_command(const char *interface, const char *cmd,
                                    char *response, size_t response_size);
extern int save_bootstrap_info(int id, const char *uri);
extern int save_configurator_info(int id, const char *curve);
extern char *load_bootstrap_uri(int id);
extern char *encode_hex_string(const char *str);

// configurator_add の実装（hostapd統合版）
int cmd_configurator_add(struct dpp_configurator_ctx *ctx, char *args)
{
    char *key_file = NULL;
    char *curve = NULL;
    int id;
    char cmd_str[512];

    if (ctx->verbose)
    {
        printf("Processing configurator_add command: %s\n", args);
    }

    // 引数解析
    key_file = parse_argument(args, "key");
    curve = parse_argument(args, "curve");

    if (!curve)
    {
        curve = strdup("prime256v1"); // デフォルト
    }

    // コマンド文字列構築
    if (key_file)
    {
        snprintf(cmd_str, sizeof(cmd_str), "key=%s", key_file);
    }
    else
    {
        snprintf(cmd_str, sizeof(cmd_str), "curve=%s", curve);
    }

    // Configurator追加
    id = dpp_configurator_add(ctx->dpp_global, cmd_str);

    if (id < 0)
    {
        printf("Failed to add configurator\n");
        if (key_file)
            free(key_file);
        if (curve)
            free(curve);
        return -1;
    }

    printf("Configurator added with ID: %d\n", id);
    ctx->configurator_count++;

    // Configurator情報を永続化
    save_configurator_info(id, curve);

    // メモリ開放
    if (key_file)
        free(key_file);
    if (curve)
        free(curve);

    return 0;
}

// dpp_qr_code の実装（hostapd統合版）
int cmd_dpp_qr_code(struct dpp_configurator_ctx *ctx, char *args)
{
    struct dpp_bootstrap_info *bi;

    if (ctx->verbose)
    {
        printf("Processing dpp_qr_code command: %s\n", args);
    }

    // QRコードのURIが必要
    if (!args || strlen(args) == 0)
    {
        printf("Error: QR code URI is required\n");
        return -1;
    }

    // DPP URIかどうかの基本チェック
    if (strncmp(args, "DPP:", 4) != 0)
    {
        printf("Error: Invalid DPP URI format (must start with 'DPP:')\n");
        return -1;
    }

    // Bootstrap情報をhostapdに追加
    bi = dpp_add_qr_code(ctx->dpp_global, args);

    if (!bi)
    {
        printf("Failed to parse QR code\n");
        return -1;
    }

    printf("Bootstrap info added with ID: %d\n", bi->id);
    if (ctx->verbose)
    {
        printf("  Parsed QR code: %s\n", args);
        if (bi->info)
        {
            printf("  Device info: %s\n", bi->info);
        }
        if (bi->chan)
        {
            printf("  Channel list: %s\n", bi->chan);
        }
    }
    ctx->bootstrap_count++;

    // 解析した情報を永続化（オリジナルのURIを保存）
    save_bootstrap_info(bi->id, args);

    return bi->id;
}

// bootstrap_get_uri の実装（hostapd統合版）
int cmd_bootstrap_get_uri(struct dpp_configurator_ctx *ctx, char *args)
{
    int id = -1;
    char *id_str = NULL;
    struct dpp_bootstrap_info *bi;

    if (ctx->verbose)
    {
        printf("Processing bootstrap_get_uri command: %s\n", args);
    }

    // 引数解析
    id_str = parse_argument(args, "id");
    if (id_str)
    {
        id = atoi(id_str);
        free(id_str);
    }

    if (id < 0)
    {
        printf("Error: id parameter required\n");
        return -1;
    }

    // hostapd内部のBootstrap情報取得
    bi = dpp_bootstrap_get_id(ctx->dpp_global, id);
    if (!bi)
    {
        // 保存された情報から読み込みを試行
        char *saved_uri = load_bootstrap_uri(id);
        if (saved_uri)
        {
            printf("Stored Peer QR Code (ID %d): %s\n", id, saved_uri);
            free(saved_uri);
            return 0;
        }
        else
        {
            printf("Error: Bootstrap ID %d not found\n", id);
            return -1;
        }
    }

    // Bootstrap情報の詳細を表示
    printf("Bootstrap ID %d Details:\n", id);
    if (bi->uri)
    {
        printf("  URI: %s\n", bi->uri);
    }
    if (bi->info)
    {
        printf("  Info: %s\n", bi->info);
    }

    // Check if pubkey_hash is set (non-zero)
    int hash_set = 0;
    for (int i = 0; i < SHA256_MAC_LEN; i++)
    {
        if (bi->pubkey_hash[i] != 0)
        {
            hash_set = 1;
            break;
        }
    }

    if (hash_set)
    {
        printf("  Public Key Hash: ");
        for (int i = 0; i < SHA256_MAC_LEN; i++)
        {
            printf("%02x", bi->pubkey_hash[i]);
            if (i < SHA256_MAC_LEN - 1)
                printf(":");
        }
        printf("\n");
    }
    else
    {
        printf("  Public Key Hash: (not set)\n");
    }

    printf("  Type: %s\n", bi->type == DPP_BOOTSTRAP_QR_CODE ? "QR Code" : bi->type == DPP_BOOTSTRAP_PKEX ? "PKEX"
                                                                                                          : "Other");

    return 0;
}

// status コマンド（hostapd統合版）
int cmd_status(struct dpp_configurator_ctx *ctx, char *args)
{
    (void)args; // 未使用パラメータの警告を避ける

    printf("DPP Configurator Status (hostapd mode):\n");
    printf("  Configurators: %d\n", ctx->configurator_count);
    printf("  Bootstrap entries: %d\n", ctx->bootstrap_count);
    printf("  Verbose mode: %s\n", ctx->verbose ? "enabled" : "disabled");

    // 追加情報があれば表示
    if (ctx->dpp_global)
    {
        printf("  DPP Global: initialized\n");
    }

    return 0;
}
