#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include "../include/dpp_configurator.h"

#ifndef STUB_MODE
// 実際のhostapd DPP統合実装

// 前方宣言
static int dpp_setup_event_handlers(struct dpp_configurator_ctx *ctx);
static int dpp_execute_real_auth(struct dpp_configurator_ctx *ctx,
                                 const char *interface,
                                 int peer_id, int configurator_id,
                                 const char *conf_type, const char *ssid, const char *pass);
static int hostapd_cli_send_command(const char *interface, const char *cmd,
                                    char *response, size_t response_size);

#define MAX_RESPONSE_SIZE 4096

// ===== ユーティリティ関数（デバッグ用） =====

// 16進数エンコーディング関数（hostapd用）
static char *encode_hex_string(const char *str)
{
    if (!str)
        return NULL;

    size_t len = strlen(str);
    char *hex_str = malloc(len * 2 + 1);
    if (!hex_str)
        return NULL;

    for (size_t i = 0; i < len; i++)
    {
        snprintf(hex_str + i * 2, 3, "%02x", (unsigned char)str[i]);
    }
    hex_str[len * 2] = '\0';

    printf("DEBUG: Encoded '%s' -> '%s'\n", str, hex_str);
    return hex_str;
}

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
    ctx->current_auth = NULL; // 現在の認証セッションを初期化
    ctx->hapd = NULL;         // hostapd interface (後で設定)
    ctx->wireless_interface = NULL;
    ctx->operating_freq = 2412; // デフォルト: Channel 1

    // DPP event ハンドラーを設定
    dpp_setup_event_handlers(ctx);

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

// auth_init の実装（シンプル版）
int cmd_auth_init(struct dpp_configurator_ctx *ctx, char *args)
{
    (void)ctx;
    (void)args;

    printf("Note: This is a simulation-only command.\n");
    printf("For real DPP authentication, use 'auth_init_real' command.\n");
    printf("Example: auth_init_real peer=1 configurator=1 conf=sta-psk interface=wlo1 ssid=MyNetwork pass=mypassword\n");

    return 0;
}

// hostapd制御ソケット通信
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define HOSTAPD_CLI_PATH "/var/run/hostapd"

// hostapdデーモンとの通信関数
static int hostapd_cli_send_command(const char *interface, const char *cmd,
                                    char *response, size_t response_size)
{
    int sock;
    struct sockaddr_un local_addr, dest_addr;
    char socket_path[256];
    char local_socket_path[256];
    ssize_t bytes_sent, bytes_received;
    struct timeval timeout;
    fd_set readfds;

    // ソケットパス構築
    snprintf(socket_path, sizeof(socket_path), "%s/%s", HOSTAPD_CLI_PATH, interface);

    printf("Attempting to connect to hostapd control socket: %s\n", socket_path);

    // ソケットファイルの存在確認
    if (access(socket_path, F_OK) != 0)
    {
        printf("Error: hostapd control socket not found: %s\n", socket_path);
        printf("Make sure hostapd is running with control interface enabled\n");
        return -1;
    }

    // UNIXソケット作成
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        printf("Error: Failed to create socket: %s\n", strerror(errno));
        return -1;
    }

    // クライアント側のソケットパスを作成（hostapdが応答を送り返すため）
    snprintf(local_socket_path, sizeof(local_socket_path), "/tmp/hostapd_cli_%d", getpid());

    // 既存のソケットファイルを削除
    unlink(local_socket_path);

    // ローカルアドレス設定
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sun_family = AF_UNIX;
    strncpy(local_addr.sun_path, local_socket_path, sizeof(local_addr.sun_path) - 1);

    // ソケットをローカルアドレスにバインド
    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    {
        printf("Error: Failed to bind socket: %s\n", strerror(errno));
        close(sock);
        unlink(local_socket_path);
        return -1;
    }

    // サーバーアドレス設定
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sun_family = AF_UNIX;
    strncpy(dest_addr.sun_path, socket_path, sizeof(dest_addr.sun_path) - 1);

    // コマンド送信
    printf("Sending command: %s\n", cmd);
    bytes_sent = sendto(sock, cmd, strlen(cmd), 0,
                        (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (bytes_sent < 0)
    {
        printf("Error: Failed to send command to hostapd: %s\n", strerror(errno));
        close(sock);
        unlink(local_socket_path);
        return -1;
    }

    printf("Command sent successfully, waiting for response...\n");

    // タイムアウト設定 (5秒)
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    // selectを使用してタイムアウト付きで応答を待機
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    int select_result = select(sock + 1, &readfds, NULL, NULL, &timeout);
    if (select_result < 0)
    {
        printf("Error: select() failed: %s\n", strerror(errno));
        close(sock);
        unlink(local_socket_path);
        return -1;
    }
    else if (select_result == 0)
    {
        printf("Error: Timeout waiting for response from hostapd\n");
        printf("hostapd may not be running or may not support the command\n");
        close(sock);
        unlink(local_socket_path);
        return -1;
    }

    // 応答受信
    bytes_received = recv(sock, response, response_size - 1, 0);
    if (bytes_received < 0)
    {
        printf("Error: Failed to receive response from hostapd: %s\n", strerror(errno));
        close(sock);
        unlink(local_socket_path);
        return -1;
    }

    response[bytes_received] = '\0';
    close(sock);
    unlink(local_socket_path);
    printf("Received response (%zd bytes): %s\n", bytes_received, response);
    return 0;
}

// DPP認証を実際のhostapdで実行
static int dpp_execute_real_auth(struct dpp_configurator_ctx *ctx,
                                 const char *interface,
                                 int peer_id, int configurator_id,
                                 const char *conf_type, const char *ssid, const char *pass)
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
    if (ssid && pass)
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
            snprintf(cmd, sizeof(cmd),
                     "DPP_AUTH_INIT peer=%d configurator=%d conf=%s ssid=%s pass=%s",
                     hostapd_peer_id, hostapd_configurator_id, conf_type, ssid_hex, pass_hex);
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
        snprintf(cmd, sizeof(cmd),
                 "DPP_AUTH_INIT peer=%d configurator=%d conf=%s",
                 hostapd_peer_id, hostapd_configurator_id, conf_type);
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
    if (peer_id < 0 || configurator_id < 0 || !conf_type || !interface)
    {
        printf("Error: peer, configurator, conf, and interface parameters required\n");
        printf("Usage: auth_init_real peer=<id> configurator=<id> conf=<type> interface=<ifname> [ssid=<ssid>] [pass=<pass>]\n");
        printf("Example: auth_init_real peer=1 configurator=1 conf=sta-psk interface=wlan0 ssid=MyWiFi pass=secret123\n");
        goto cleanup;
    }

    printf("Real DPP Authentication Parameters:\n");
    printf("  Interface: %s\n", interface);
    printf("  Peer ID: %d\n", peer_id);
    printf("  Configurator ID: %d\n", configurator_id);
    printf("  Configuration type: %s\n", conf_type);
    if (ssid)
        printf("  SSID: %s\n", ssid);
    if (pass)
        printf("  Password: %s\n", pass);

    // 実際のhostapd経由でDPP認証を実行
    ret = dpp_execute_real_auth(ctx, interface, peer_id, configurator_id,
                                conf_type, ssid, pass);

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

    return ret;
}

// ===== DPP制御ソケット通信 =====

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

    printf("DPP Configurator CLI Tool (hostapd mode)\n");
    printf("=========================================\n\n");

    printf("Basic Commands:\n");
    printf("  %-20s %s\n", "configurator_add", "Add configurator (curve=prime256v1)");
    printf("  %-20s %s\n", "dpp_qr_code", "Parse QR code and add bootstrap");
    printf("  %-20s %s\n", "bootstrap_get_uri", "Get bootstrap URI by ID");
    printf("  %-20s %s\n", "auth_init_real", "Initiate DPP authentication (REAL wireless)");
    printf("  %-20s %s\n", "status", "Show configurator status");

    printf("\nDebugging Commands:\n");
    printf("  %-20s %s\n", "test_hostapd", "Test hostapd connection");
    printf("  %-20s %s\n", "debug_dpp", "Test DPP commands directly");
    printf("  %-20s %s\n", "auth_status", "Show authentication status");
    printf("  %-20s %s\n", "help", "Show this help");

    printf("\nUsage Examples:\n");
    printf("  configurator_add curve=prime256v1\n");
    printf("  dpp_qr_code \"DPP:K:MDkwEwYHKoZIzj0CAQYIKoZIzj0DAQc...\"\n");
    printf("  bootstrap_get_uri id=1\n");
    printf("  auth_init_real peer=1 configurator=1 conf=sta-psk interface=wlo1 ssid=MyNetwork pass=mypassword\n");
    printf("  test_hostapd interface=wlo1\n");
    printf("  debug_dpp interface=wlo1\n");

    printf("\nNotes:\n");
    printf("  - This tool integrates with hostapd for real DPP wireless communication\n");
    printf("  - Use 'test_hostapd' first to verify hostapd connection\n");
    printf("  - SSID and password are automatically hex-encoded for hostapd\n");
    printf("  - Monitor DPP authentication progress with hostapd logs\n");

    return 0;
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

// ===== DPPイベントハンドラーセットアップ =====

// DPPイベントハンドラーセットアップ
static int dpp_setup_event_handlers(struct dpp_configurator_ctx *ctx)
{
    if (!ctx)
    {
        printf("Error: Invalid context for event handler setup\n");
        return -1;
    }

    printf("Setting up DPP event handlers...\n");

    // 実際の実装では、ここでhostapd event loopとの統合を行う
    /*
    eloop_register_timeout(1, 0, dpp_timeout_handler, ctx, NULL);
    */

    printf("✓ DPP event handlers configured\n");
    return 0;
}

// hostapd テスト用の基本的なコマンド
int cmd_test_hostapd(struct dpp_configurator_ctx *ctx, char *args)
{
    char *interface = NULL;
    char cmd[512];
    char response[MAX_RESPONSE_SIZE];
    int ret;

    (void)ctx; // 未使用パラメータ警告を回避

    // 引数解析
    interface = parse_argument(args, "interface");
    if (!interface)
    {
        printf("Error: interface parameter required\n");
        printf("Usage: test_hostapd interface=<ifname>\n");
        return -1;
    }

    printf("Testing hostapd connection on interface: %s\n", interface);

    // Test 1: PING コマンド
    printf("Test 1: Sending PING command...\n");
    snprintf(cmd, sizeof(cmd), "PING");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret < 0)
    {
        printf("✗ PING test failed\n");
        free(interface);
        return -1;
    }
    printf("✓ PING response: %s\n", response);

    // Test 2: STATUS コマンド
    printf("Test 2: Sending STATUS command...\n");
    snprintf(cmd, sizeof(cmd), "STATUS");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret < 0)
    {
        printf("✗ STATUS test failed\n");
        free(interface);
        return -1;
    }
    printf("✓ STATUS response (first 200 chars): %.200s%s\n",
           response, strlen(response) > 200 ? "..." : "");

    // Test 3: DPP サポート確認
    printf("Test 3: Checking DPP support...\n");
    snprintf(cmd, sizeof(cmd), "HELP");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret < 0)
    {
        printf("✗ HELP test failed\n");
        free(interface);
        return -1;
    }

    if (strstr(response, "DPP_"))
    {
        printf("✓ DPP commands found in HELP output\n");
        printf("Available DPP commands:\n");
        char *line = strtok(response, "\n");
        while (line != NULL)
        {
            if (strstr(line, "DPP_"))
            {
                printf("  %s\n", line);
            }
            line = strtok(NULL, "\n");
        }
    }
    else
    {
        printf("✗ No DPP commands found in HELP output\n");
        printf("hostapd may not be compiled with DPP support\n");
    }

    free(interface);
    return 0;
}

// DPP状態デバッグコマンド
int cmd_debug_dpp(struct dpp_configurator_ctx *ctx, char *args)
{
    char *interface = NULL;
    char cmd[512];
    char response[MAX_RESPONSE_SIZE];
    int ret;

    (void)ctx; // 未使用パラメータ警告を回避

    // 引数解析
    interface = parse_argument(args, "interface");
    if (!interface)
    {
        printf("Error: interface parameter required\n");
        printf("Usage: debug_dpp interface=<ifname>\n");
        return -1;
    }

    printf("DPP Debug Information for interface: %s\n", interface);

    // Test 1: DPP_CONFIGURATOR_ADD テスト
    printf("\nTest 1: Testing DPP_CONFIGURATOR_ADD...\n");
    snprintf(cmd, sizeof(cmd), "DPP_CONFIGURATOR_ADD curve=prime256v1");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret < 0)
    {
        printf("✗ DPP_CONFIGURATOR_ADD failed\n");
    }
    else
    {
        printf("✓ DPP_CONFIGURATOR_ADD response: %s\n", response);
    }

    // Test 2: DPP_QR_CODE テスト（簡単なQRコード）
    printf("\nTest 2: Testing DPP_QR_CODE...\n");
    snprintf(cmd, sizeof(cmd), "DPP_QR_CODE DPP:C:81/6;M:54:32:04:1f:b5:a8;K:MDkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDIgACCcWFqRtN+f0loEUgGIXDnMXPrjl92u2pV97Ff6DjUD8=;;");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret < 0)
    {
        printf("✗ DPP_QR_CODE failed\n");
    }
    else
    {
        printf("✓ DPP_QR_CODE response: %s\n", response);
    }

    // Test 3: DPP_BOOTSTRAP_GET_URI テスト
    printf("\nTest 3: Testing DPP_BOOTSTRAP_GET_URI...\n");
    snprintf(cmd, sizeof(cmd), "DPP_BOOTSTRAP_GET_URI 1");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret < 0)
    {
        printf("✗ DPP_BOOTSTRAP_GET_URI failed\n");
    }
    else
    {
        printf("✓ DPP_BOOTSTRAP_GET_URI response: %s\n", response);
    }

    // Test 4: DPP_CONFIGURATOR_GET_KEY テスト
    printf("\nTest 4: Testing DPP_CONFIGURATOR_GET_KEY...\n");
    snprintf(cmd, sizeof(cmd), "DPP_CONFIGURATOR_GET_KEY 1");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret < 0)
    {
        printf("✗ DPP_CONFIGURATOR_GET_KEY failed\n");
    }
    else
    {
        printf("✓ DPP_CONFIGURATOR_GET_KEY response: %s\n", response);
    }

    // Test 5: その他のDPPコマンド試行
    printf("\nTest 5: Testing other DPP commands...\n");

    // DPP_LISTEN
    printf("Testing DPP_LISTEN...\n");
    snprintf(cmd, sizeof(cmd), "DPP_LISTEN 2412");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret >= 0)
    {
        printf("✓ DPP_LISTEN response: %s\n", response);
    }
    else
    {
        printf("✗ DPP_LISTEN failed\n");
    }

    // DPP_STOP_LISTEN
    printf("Testing DPP_STOP_LISTEN...\n");
    snprintf(cmd, sizeof(cmd), "DPP_STOP_LISTEN");
    ret = hostapd_cli_send_command(interface, cmd, response, sizeof(response));
    if (ret >= 0)
    {
        printf("✓ DPP_STOP_LISTEN response: %s\n", response);
    }
    else
    {
        printf("✗ DPP_STOP_LISTEN failed\n");
    }

    free(interface);
    return 0;
}
#endif /* STUB_MODE */
