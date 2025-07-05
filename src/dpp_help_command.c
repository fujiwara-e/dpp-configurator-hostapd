/*
 * DPP Configurator - Help Command
 * Help and usage information
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dpp_configurator.h"

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

    printf("\nUtility Commands:\n");
    printf("  %-25s %s\n", "help", "Show this help");

    printf("\nUsage Examples:\n");
    printf("  Basic Setup:\n");
    printf("    configurator_add curve=prime256v1\n");
    printf("    dpp_qr_code \"DPP:C:81/6;M:12:34:56:78:90:ab;K:MDkwEwYH...6DjUD8=;;\"\n");
    printf("    bootstrap_get_uri id=1\n");
    printf("\n");
    printf("  Authentication:\n");
    printf("    auth_init_real peer=1 configurator=1 conf=sta-psk interface=wlo1 ssid=MyNetwork pass=mypassword\n");

    printf("\nNotes:\n");
    printf("  - This tool integrates with hostapd for real DPP wireless communication\n");
    printf("  - SSID and password are automatically hex-encoded for hostapd\n");
    printf("  - Monitor DPP authentication progress with hostapd logs\n");

    printf("\nImportant:\n");
    printf("  - Make sure hostapd is running with DPP support enabled\n");
    printf("  - Ensure control interface is properly configured\n");
    printf("  - Run with appropriate privileges (sudo if needed)\n");

    return 0;
}
