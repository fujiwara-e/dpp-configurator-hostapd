/*
 * DPP Configurator - Help Command
 * Help and usage information
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dpp_configurator.h"

// help コマンド（hostapd統合版）
int cmd_help(struct dpp_configurator_ctx *ctx, char *args)
{
    (void)ctx;  // 未使用パラメータの警告を避ける
    (void)args; // 未使用パラメータの警告を避ける

    printf("DPP Configurator CLI Tool (hostapd mode)\n");
    printf("=========================================\n\n");

    printf("Basic Commands:\n");
    printf("  %-25s %s\n", "configurator_add", "Add configurator (curve=prime256v1)");
    printf("  %-25s %s\n", "dpp_qr_code", "Parse QR code and add bootstrap");
    printf("  %-25s %s\n", "bootstrap_get_uri", "Get bootstrap URI by ID");
    printf("  %-25s %s\n", "auth_init_real", "Initiate DPP authentication (REAL wireless)");
    printf("  %-25s %s\n", "status", "Show configurator status");

    printf("\nAuthentication Commands:\n");
    printf("  %-25s %s\n", "auth_status", "Show authentication status");
    printf("  %-25s %s\n", "auth_monitor", "Monitor DPP authentication events");
    printf("  %-25s %s\n", "auth_control", "Control DPP authentication (start/stop/status)");

    printf("\nGAS/Configuration Commands:\n");
    printf("  %-25s %s\n", "gas_server_start", "Start GAS server for Configuration Requests");
    printf("  %-25s %s\n", "gas_server_stop", "Stop GAS server");
    printf("  %-25s %s\n", "config_request_monitor", "Monitor Configuration Request/Response");

    printf("\nUtility Commands:\n");
    printf("  %-25s %s\n", "help", "Show this help");

    printf("\nUsage Examples:\n");
    printf("  Basic Setup:\n");
    printf("    configurator_add curve=prime256v1\n");
    printf("    dpp_qr_code \"DPP:K:MDkwEwYHKoZIzj0CAQYIKoZIzj0DAQc...\"\n");
    printf("    bootstrap_get_uri id=1\n");
    printf("\n");
    printf("  Authentication:\n");
    printf("    auth_init_real peer=1 configurator=1 conf=sta-psk interface=wlo1 ssid=MyNetwork pass=mypassword\n");
    printf("    auth_monitor interface=wlo1 timeout=60\n");
    printf("\n");
    printf("  Control:\n");
    printf("    auth_control interface=wlo1 action=start\n");
    printf("    auth_control interface=wlo1 action=stop\n");
    printf("    auth_control interface=wlo1 action=status\n");
    printf("\n");
    printf("  GAS/Configuration:\n");
    printf("    gas_server_start interface=wlo1\n");
    printf("    config_request_monitor interface=wlo1 timeout=120\n");

    printf("\nNotes:\n");
    printf("  - This tool integrates with hostapd for real DPP wireless communication\n");
    printf("  - Use 'config_request_monitor' to monitor Configuration Request/Response\n");
    printf("  - Use 'auth_control' to start/stop DPP listening mode\n");
    printf("  - SSID and password are automatically hex-encoded for hostapd\n");
    printf("  - Monitor DPP authentication progress with hostapd logs\n");

    printf("\nImportant:\n");
    printf("  - Make sure hostapd is running with DPP support enabled\n");
    printf("  - Ensure control interface is properly configured\n");
    printf("  - Run with appropriate privileges (sudo if needed)\n");

    return 0;
}
