/*
 * DPP Configurator - hostapd Core Functions
 * Core hostapd integration and communication functions
 */

#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "../include/dpp_configurator.h"

#define MAX_RESPONSE_SIZE 4096
#define HOSTAPD_CLI_PATH "/var/run/hostapd"

// hostapd制御ソケット通信
int hostapd_cli_send_command(const char *interface, const char *cmd,
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

// 16進数エンコーディング関数（hostapd用）
static char *encode_hex_string_hostapd(const char *str)
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
