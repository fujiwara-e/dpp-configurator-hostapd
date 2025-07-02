#include "../include/dpp_configurator.h"

// スタブ実装: DPPライブラリが利用できない場合のフォールバック

// DPP初期化関数（スタブ）
struct dpp_configurator_ctx *dpp_configurator_init(void)
{
    struct dpp_configurator_ctx *ctx;

    ctx = malloc(sizeof(*ctx));
    if (!ctx)
        return NULL;

    ctx->dpp_global = NULL; // スタブとして NULL を設定
    ctx->configurator_count = 0;
    ctx->bootstrap_count = 0;
    ctx->verbose = false;

    printf("DPP Configurator initialized (stub mode)\n");
    return ctx;
}

// DPP終了処理
void dpp_configurator_deinit(struct dpp_configurator_ctx *ctx)
{
    if (!ctx)
        return;

    // スタブ実装では何もしない

    free(ctx);
}

// configurator_add の実装（スタブ）
int cmd_configurator_add(struct dpp_configurator_ctx *ctx, char *args)
{
    char *key_file = NULL;
    char *curve = "prime256v1"; // デフォルト
    int id = 1;                 // スタブとして固定値

    if (ctx->verbose)
    {
        printf("Processing configurator_add command: %s\n", args);
    }

    // 引数解析
    key_file = parse_argument(args, "key");
    char *curve_arg = parse_argument(args, "curve");
    if (curve_arg)
    {
        curve = curve_arg;
    }

    printf("Adding configurator with curve=%s", curve);
    if (key_file)
    {
        printf(", key=%s", key_file);
        free(key_file);
    }
    printf(" (stub implementation)\n");

    if (curve_arg)
    {
        free(curve_arg);
    }

    printf("Configurator added with ID: %d\n", id);
    ctx->configurator_count++;
    return 0;
}

// bootstrap_gen の実装（スタブ）
int cmd_bootstrap_gen(struct dpp_configurator_ctx *ctx, char *args)
{
    char *type = NULL;
    char *curve = "prime256v1";
    char *key_file = NULL;
    int id = 1; // スタブとして固定値

    if (ctx->verbose)
    {
        printf("Processing bootstrap_gen command: %s\n", args);
    }

    // 引数解析
    type = parse_argument(args, "type");
    char *curve_arg = parse_argument(args, "curve");
    key_file = parse_argument(args, "key");

    if (curve_arg)
    {
        curve = curve_arg;
    }

    if (!type || strcmp(type, "qr") != 0)
    {
        printf("Error: Only type=qr is supported\n");
        if (type)
            free(type);
        if (curve_arg)
            free(curve_arg);
        if (key_file)
            free(key_file);
        return -1;
    }

    printf("Generating bootstrap with type=%s, curve=%s", type, curve);
    if (key_file)
    {
        printf(", key=%s", key_file);
    }
    printf(" (stub implementation)\n");

    // メモリ開放
    if (type)
        free(type);
    if (curve_arg)
        free(curve_arg);
    if (key_file)
        free(key_file);

    printf("Bootstrap generated with ID: %d\n", id);
    ctx->bootstrap_count++;
    return 0;
}

// bootstrap_get_uri の実装（スタブ）
int cmd_bootstrap_get_uri(struct dpp_configurator_ctx *ctx, char *args)
{
    int id = -1;
    char *id_str = NULL;

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

    // スタブのURI
    const char *stub_uri = "DPP:K:MDkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDIgACxXREuPOan...STUB";
    printf("Bootstrap URI: %s\n", stub_uri);
    return 0;
}

// auth_init の実装（スタブ）
int cmd_auth_init(struct dpp_configurator_ctx *ctx, char *args)
{
    int peer_id = -1;
    int configurator_id = -1;
    char *conf_type = NULL;
    char *ssid = NULL;
    char *pass = NULL;

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
    printf(" (stub implementation)\n");

    // メモリ開放
    if (conf_type)
        free(conf_type);
    if (ssid)
        free(ssid);
    if (pass)
        free(pass);

    return 0;
}

// status コマンド
int cmd_status(struct dpp_configurator_ctx *ctx, char *args)
{
    printf("DPP Configurator Status (stub mode):\n");
    printf("  Configurators: %d\n", ctx->configurator_count);
    printf("  Bootstrap entries: %d\n", ctx->bootstrap_count);
    printf("  Verbose mode: %s\n", ctx->verbose ? "enabled" : "disabled");
    return 0;
}

// help コマンド
int cmd_help(struct dpp_configurator_ctx *ctx, char *args)
{
    printf("Available commands:\n");
    printf("  %-20s %s\n", "configurator_add", "Add configurator");
    printf("  %-20s %s\n", "bootstrap_gen", "Generate bootstrap");
    printf("  %-20s %s\n", "bootstrap_get_uri", "Get bootstrap URI");
    printf("  %-20s %s\n", "auth_init", "Initiate authentication");
    printf("  %-20s %s\n", "status", "Show status");
    printf("  %-20s %s\n", "help", "Show help");

    printf("\nExamples:\n");
    printf("  configurator_add curve=prime256v1\n");
    printf("  bootstrap_gen type=qr curve=prime256v1\n");
    printf("  bootstrap_get_uri id=1\n");
    printf("  auth_init peer=2 configurator=1 conf=sta-psk ssid=test pass=test123\n");

    return 0;
}
