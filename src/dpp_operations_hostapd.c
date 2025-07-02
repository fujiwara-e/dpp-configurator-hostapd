#include <stdbool.h>
#include "../include/dpp_configurator.h"

#ifndef STUB_MODE
// hostapd統合実装

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

    // 新しい情報を追加（簡単なJSON形式）
    fp = fopen(DPP_STATE_FILE, "w");
    if (!fp)
    {
        return -1;
    }

    // 簡単なJSON形式で保存
    fprintf(fp, "{\n");
    fprintf(fp, "  \"bootstrap_%d\": {\n", id);
    fprintf(fp, "    \"id\": %d,\n", id);
    fprintf(fp, "    \"uri\": \"%s\"\n", uri);
    fprintf(fp, "  }\n");
    fprintf(fp, "}\n");

    fclose(fp);
    return 0;
}

// Bootstrap情報を読み込み
static char *load_bootstrap_uri(int id)
{
    FILE *fp;
    char buffer[4096];
    char *line;
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

    // メモリ開放
    if (key_file)
        free(key_file);
    if (curve)
        free(curve);

    return 0;
}

// bootstrap_gen の実装（hostapd統合版）
int cmd_bootstrap_gen(struct dpp_configurator_ctx *ctx, char *args)
{
    char *type = NULL;
    char *curve = NULL;
    char *key_file = NULL;
    int id;
    char cmd_str[512];

    if (ctx->verbose)
    {
        printf("Processing bootstrap_gen command: %s\n", args);
    }

    // 引数解析
    type = parse_argument(args, "type");
    curve = parse_argument(args, "curve");
    key_file = parse_argument(args, "key");

    if (!curve)
    {
        curve = strdup("prime256v1"); // デフォルト
    }

    if (!type || strcmp(type, "qr") != 0)
    {
        printf("Error: Only type=qr is supported\n");
        if (type)
            free(type);
        if (curve)
            free(curve);
        if (key_file)
            free(key_file);
        return -1;
    }

    // コマンド文字列構築 (hostapd expects "type=qrcode")
    if (key_file)
    {
        snprintf(cmd_str, sizeof(cmd_str), "type=qrcode curve=%s key=%s", curve, key_file);
    }
    else
    {
        snprintf(cmd_str, sizeof(cmd_str), "type=qrcode curve=%s", curve);
    }

    // Bootstrap生成
    id = dpp_bootstrap_gen(ctx->dpp_global, cmd_str);

    if (id < 0)
    {
        printf("Failed to generate bootstrap\n");
        if (type)
            free(type);
        if (curve)
            free(curve);
        if (key_file)
            free(key_file);
        return -1;
    }

    // URIを取得して保存
    const char *uri = dpp_bootstrap_get_uri(ctx->dpp_global, id);
    if (uri)
    {
        save_bootstrap_info(id, uri);
    }

    printf("Bootstrap generated with ID: %d\n", id);
    ctx->bootstrap_count++;

    // メモリ開放
    if (type)
        free(type);
    if (curve)
        free(curve);
    if (key_file)
        free(key_file);

    return 0;
}

// bootstrap_get_uri の実装（hostapd統合版）
int cmd_bootstrap_get_uri(struct dpp_configurator_ctx *ctx, char *args)
{
    int id = -1;
    char *id_str = NULL;
    const char *uri;

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

    // Bootstrap URI取得
    uri = dpp_bootstrap_get_uri(ctx->dpp_global, id);
    if (!uri)
    {
        // 保存された情報から読み込みを試行
        char *saved_uri = load_bootstrap_uri(id);
        if (saved_uri)
        {
            printf("Bootstrap URI: %s\n", saved_uri);
            free(saved_uri);
            return 0;
        }
        else
        {
            printf("Error: Bootstrap ID %d not found\n", id);
            return -1;
        }
    }

    printf("Bootstrap URI: %s\n", uri);
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
    }

    // Bootstrap情報取得
    peer_bi = dpp_bootstrap_get_id(ctx->dpp_global, peer_id);
    if (!peer_bi)
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

    // 認証初期化
    auth = dpp_auth_init(ctx->dpp_global, ctx, peer_bi, NULL,
                         DPP_CAPAB_CONFIGURATOR, 0, NULL, 0);
    if (!auth)
    {
        printf("Error: Failed to initialize authentication\n");
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
    printf("  %-20s %s\n", "bootstrap_gen", "Generate bootstrap");
    printf("  %-20s %s\n", "bootstrap_get_uri", "Get bootstrap URI");
    printf("  %-20s %s\n", "auth_init", "Initiate authentication");
    printf("  %-20s %s\n", "status", "Show status");
    printf("  %-20s %s\n", "test_bootstrap", "Test bootstrap generation + URI");
    printf("  %-20s %s\n", "help", "Show help");

    printf("\nExamples:\n");
    printf("  configurator_add curve=prime256v1\n");
    printf("  bootstrap_gen type=qr curve=prime256v1\n");
    printf("  bootstrap_get_uri id=1\n");
    printf("  auth_init peer=2 configurator=1 conf=sta-psk ssid=test pass=test123\n");
    printf("  test_bootstrap\n");

    return 0;
}

// Test command to demonstrate hostapd integration (generate bootstrap + get URI)
int cmd_test_bootstrap(struct dpp_configurator_ctx *ctx, char *args)
{
    int bootstrap_id;
    const char *uri;
    char cmd_str[256];

    (void)args; // Unused parameter

    printf("Testing hostapd DPP integration...\n");

    // Generate bootstrap
    snprintf(cmd_str, sizeof(cmd_str), "type=qrcode curve=prime256v1");
    bootstrap_id = dpp_bootstrap_gen(ctx->dpp_global, cmd_str);

    if (bootstrap_id < 0)
    {
        printf("Failed to generate bootstrap\n");
        return -1;
    }

    printf("Bootstrap generated with ID: %d\n", bootstrap_id);

    // Get URI
    uri = dpp_bootstrap_get_uri(ctx->dpp_global, bootstrap_id);
    if (!uri)
    {
        printf("Failed to get bootstrap URI\n");
        return -1;
    }

    printf("Bootstrap URI: %s\n", uri);
    return 0;
}

#endif /* STUB_MODE */
