#include <stdbool.h>
#include "../include/dpp_configurator.h"

#ifndef STUB_MODE
// 実際のhostapd DPP統合実装

// Bootstrap情報の永続化
#define DPP_STATE_FILE "/tmp/dpp_configurator_state.json"

// Bootstrap情報を保存
static int save_bootstrap_info(int id, const char *uri)
{
    FILE *fp;
    char buffer[4096];
    int len;

    // 既存の状態を読み込み
    fp = fopen(DPP_STATE_FILE, "r");
    if (fp)
    {
        len = fread(buffer, 1, sizeof(buffer) - 1, fp);
        buffer[len] = '\0';
        fclose(fp);
    }
    else
    {
        strcpy(buffer, "{}");
    }

    // 新しい情報を追加
    fp = fopen(DPP_STATE_FILE, "w");
    if (!fp)
    {
        return -1;
    }

    fprintf(fp, "{\n");

    // 既存のconfigurator情報があれば保持
    if (strstr(buffer, "configurator_"))
    {
        char *start = strstr(buffer, "\"configurator_");
        char *end = NULL;
        if (start)
        {
            end = strstr(start, "\n  }\n");
            if (end)
            {
                end += 4; // "\n  }\n" の長さ
                fwrite(start, 1, end - start, fp);
                fprintf(fp, ",\n");
            }
        }
    }

    // Bootstrap情報を追加
    fprintf(fp, "  \"bootstrap_%d\": {\n", id);
    fprintf(fp, "    \"id\": %d,\n", id);
    fprintf(fp, "    \"uri\": \"%s\"\n", uri);
    fprintf(fp, "  }\n");
    fprintf(fp, "}\n");

    fclose(fp);
    return 0;
}

// Configurator情報を保存
static int save_configurator_info(int id, const char *curve)
{
    FILE *fp;
    char buffer[4096];
    int len;

    // 既存の状態を読み込み
    fp = fopen(DPP_STATE_FILE, "r");
    if (fp)
    {
        len = fread(buffer, 1, sizeof(buffer) - 1, fp);
        buffer[len] = '\0';
        fclose(fp);
    }
    else
    {
        strcpy(buffer, "{}");
    }

    // 新しい情報を追加
    fp = fopen(DPP_STATE_FILE, "w");
    if (!fp)
    {
        return -1;
    }

    fprintf(fp, "{\n");

    // 既存のbootstrap情報があれば保持
    if (strstr(buffer, "bootstrap_"))
    {
        char *start = strstr(buffer, "\"bootstrap_");
        char *end = NULL;
        if (start)
        {
            end = strstr(start, "\n  }\n");
            if (end)
            {
                end += 4; // "\n  }\n" の長さ
                fwrite(start, 1, end - start, fp);
                fprintf(fp, ",\n");
            }
        }
    }

    // Configurator情報を追加
    fprintf(fp, "  \"configurator_%d\": {\n", id);
    fprintf(fp, "    \"id\": %d,\n", id);
    fprintf(fp, "    \"curve\": \"%s\"\n", curve);
    fprintf(fp, "  }\n");
    fprintf(fp, "}\n");

    fclose(fp);
    return 0;
}

// Configurator情報を読み込み
static char *load_configurator_curve(int id)
{
    FILE *fp;
    char buffer[4096];
    char *curve = NULL;
    char search_pattern[64];

    fp = fopen(DPP_STATE_FILE, "r");
    if (!fp)
    {
        return NULL;
    }

    snprintf(search_pattern, sizeof(search_pattern), "\"configurator_%d\"", id);

    while (fgets(buffer, sizeof(buffer), fp))
    {
        if (strstr(buffer, search_pattern))
        {
            // 次の行でcurveを探す
            while (fgets(buffer, sizeof(buffer), fp))
            {
                if (strstr(buffer, "\"curve\""))
                {
                    // curveの値を抽出
                    char *start = strchr(buffer, '"');
                    if (start)
                    {
                        start = strchr(start + 1, '"');
                        if (start)
                        {
                            start = strchr(start + 1, '"');
                            if (start)
                            {
                                start++;
                                char *end = strchr(start, '"');
                                if (end)
                                {
                                    *end = '\0';
                                    // 改行文字を除去
                                    char *newline = strchr(start, '\n');
                                    if (newline)
                                    {
                                        *newline = '\0';
                                    }
                                    curve = strdup(start);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
    }

    fclose(fp);
    return curve;
}

// Bootstrap情報を読み込み
static char *load_bootstrap_uri(int id)
{
    FILE *fp;
    char buffer[4096];
    char *uri = NULL;
    char search_pattern[64];

    fp = fopen(DPP_STATE_FILE, "r");
    if (!fp)
    {
        return NULL;
    }

    snprintf(search_pattern, sizeof(search_pattern), "\"bootstrap_%d\"", id);

    while (fgets(buffer, sizeof(buffer), fp))
    {
        if (strstr(buffer, search_pattern))
        {
            // 次の行でURIを探す
            while (fgets(buffer, sizeof(buffer), fp))
            {
                if (strstr(buffer, "\"uri\""))
                {
                    // URIの値を抽出
                    char *start = strchr(buffer, '"');
                    if (start)
                    {
                        start = strchr(start + 1, '"');
                        if (start)
                        {
                            start = strchr(start + 1, '"');
                            if (start)
                            {
                                start++;
                                char *end = strchr(start, '"');
                                if (end)
                                {
                                    *end = '\0';
                                    // 改行文字を除去
                                    char *newline = strchr(start, '\n');
                                    if (newline)
                                    {
                                        *newline = '\0';
                                    }
                                    uri = strdup(start);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
    }

    fclose(fp);
    return uri;
}

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

    printf("DPP Configurator initialized (hostapd mode)\n");
    return ctx;
}

// DPP終了処理（hostapd統合版）
void dpp_configurator_deinit(struct dpp_configurator_ctx *ctx)
{
    if (!ctx)
        return;

    if (ctx->dpp_global)
    {
        dpp_global_deinit(ctx->dpp_global);
    }

    os_free(ctx);
}

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

// auth_init の実装（hostapd統合版）
int cmd_auth_init(struct dpp_configurator_ctx *ctx, char *args)
{
    int peer_id = -1;
    int configurator_id = -1;
    char *conf_type = NULL;
    char *ssid = NULL;
    char *pass = NULL;
    struct dpp_bootstrap_info *peer_bi;
    struct dpp_authentication *auth;
    char config_cmd[512];

    if (ctx->verbose)
    {
        printf("Processing auth_init command: %s\n", args);
    }

    // 引数解析
    char *peer_str = parse_argument(args, "peer");
    char *configurator_str = parse_argument(args, "configurator");
    conf_type = parse_argument(args, "conf");
    ssid = parse_argument(args, "ssid");
    pass = parse_argument(args, "pass");

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

    if (peer_id < 0 || configurator_id < 0 || !conf_type)
    {
        printf("Error: peer, configurator, and conf parameters required\n");
        if (conf_type)
            free(conf_type);
        if (ssid)
            free(ssid);
        if (pass)
            free(pass);
        return -1;
    } // Bootstrap情報取得
    peer_bi = dpp_bootstrap_get_id(ctx->dpp_global, peer_id);
    if (!peer_bi)
    {
        // 保存されたbootstrap情報から復元を試行
        char *saved_uri = load_bootstrap_uri(peer_id);
        if (saved_uri)
        {
            printf("Restoring bootstrap info from saved state...\n");
            peer_bi = dpp_add_qr_code(ctx->dpp_global, saved_uri);
            free(saved_uri);

            if (!peer_bi)
            {
                printf("Error: Failed to restore bootstrap ID %d\n", peer_id);
                if (conf_type)
                    free(conf_type);
                if (ssid)
                    free(ssid);
                if (pass)
                    free(pass);
                return -1;
            }
            printf("Bootstrap ID %d restored successfully (actual ID: %d)\n", peer_id, peer_bi->id);
            // 実際に復元されたIDを使用
            peer_id = peer_bi->id;
        }
        else
        {
            printf("Error: Peer bootstrap ID %d not found\n", peer_id);
            if (conf_type)
                free(conf_type);
            if (ssid)
                free(ssid);
            if (pass)
                free(pass);
            return -1;
        }
    } // 保存されたConfiguratorを復元を試行（毎回実行して確実にする）
    char *saved_curve = load_configurator_curve(configurator_id);
    if (saved_curve)
    {
        printf("Restoring configurator info from saved state...\n");
        char restore_cmd[128];
        snprintf(restore_cmd, sizeof(restore_cmd), "curve=%s", saved_curve);
        int restored_id = dpp_configurator_add(ctx->dpp_global, restore_cmd);
        free(saved_curve);

        if (restored_id < 0)
        {
            printf("Error: Failed to restore configurator ID %d\n", configurator_id);
            if (conf_type)
                free(conf_type);
            if (ssid)
                free(ssid);
            if (pass)
                free(pass);
            return -1;
        }
        printf("Configurator restored with ID: %d\n", restored_id);
        // 実際に復元されたIDを使用
        configurator_id = restored_id;
    }
    else
    {
        printf("Error: Configurator ID %d not found and cannot be restored\n", configurator_id);
        if (conf_type)
            free(conf_type);
        if (ssid)
            free(ssid);
        if (pass)
            free(pass);
        return -1;
    }

    // 認証初期化
    auth = dpp_auth_init(ctx->dpp_global, ctx, peer_bi, NULL,
                         DPP_CAPAB_CONFIGURATOR, 0, NULL, 0);
    if (!auth)
    {
        printf("Error: Failed to initialize authentication\n");
        printf("  peer_bi: %p\n", (void *)peer_bi);
        printf("  peer_bi->id: %d\n", peer_bi ? peer_bi->id : -1);
        printf("  dpp_global: %p\n", (void *)ctx->dpp_global);
        if (conf_type)
            free(conf_type);
        if (ssid)
            free(ssid);
        if (pass)
            free(pass);
        return -1;
    }

    // Configurator設定構築
    snprintf(config_cmd, sizeof(config_cmd), " configurator=%d", configurator_id);

    if (ssid && pass)
    {
        char conf_params[256];
        snprintf(conf_params, sizeof(conf_params), " conf=%s ssid=%s pass=%s",
                 conf_type, ssid, pass);
        strcat(config_cmd, conf_params);
    }
    else
    {
        char conf_params[64];
        snprintf(conf_params, sizeof(conf_params), " conf=%s", conf_type);
        strcat(config_cmd, conf_params);
    }

    if (dpp_set_configurator(auth, config_cmd) < 0)
    {
        printf("Error: Failed to set configurator parameters\n");
        dpp_auth_deinit(auth);
        if (conf_type)
            free(conf_type);
        if (ssid)
            free(ssid);
        if (pass)
            free(pass);
        return -1;
    }

    printf("Authentication initiated for peer %d with configurator %d\n", peer_id, configurator_id);
    printf("Configuration type: %s", conf_type);
    if (ssid)
    {
        printf(", SSID: %s", ssid);
    }
    if (pass)
    {
        printf(", Password: %s", pass);
    }
    printf("\n");

    // メモリ開放
    if (conf_type)
        free(conf_type);
    if (ssid)
        free(ssid);
    if (pass)
        free(pass);

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

// help コマンド（hostapd統合版）
int cmd_help(struct dpp_configurator_ctx *ctx, char *args)
{
    (void)ctx;  // 未使用パラメータの警告を避ける
    (void)args; // 未使用パラメータの警告を避ける

    printf("Available commands (hostapd mode):\n");
    printf("  %-20s %s\n", "configurator_add", "Add configurator");
    printf("  %-20s %s\n", "dpp_qr_code", "Parse QR code and add bootstrap");
    printf("  %-20s %s\n", "bootstrap_get_uri", "Get bootstrap URI");
    printf("  %-20s %s\n", "auth_init", "Initiate authentication");
    printf("  %-20s %s\n", "status", "Show status");
    printf("  %-20s %s\n", "help", "Show help");

    printf("\nExamples:\n");
    printf("  configurator_add curve=prime256v1\n");
    printf("  dpp_qr_code \"DPP:K:MDkwEwYHKoZIzj0CAQYIKoZIzj0DAQc...\"\n");
    printf("  bootstrap_get_uri id=1\n");
    printf("  auth_init peer=2 configurator=1 conf=sta-psk ssid=test pass=test123\n");

    return 0;
}

#endif /* STUB_MODE */
