/*
 * DPP Configurator - Diagnostic Commands
 * Network and hostapd diagnostic commands
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/dpp_configurator.h"

#ifndef STUB_MODE

#define MAX_RESPONSE_SIZE 4096

// External functions
extern int hostapd_cli_send_command(const char *interface, const char *cmd,
                                    char *response, size_t response_size);

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
