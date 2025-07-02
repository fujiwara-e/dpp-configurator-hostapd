#define _GNU_SOURCE
#include "../include/dpp_configurator.h"

// コマンド一覧
static struct dpp_command commands[] = {
    { "configurator_add", cmd_configurator_add, "Add configurator" },
    { "bootstrap_gen", cmd_bootstrap_gen, "Generate bootstrap" },
    { "bootstrap_get_uri", cmd_bootstrap_get_uri, "Get bootstrap URI" },
    { "auth_init", cmd_auth_init, "Initiate authentication" },
    { "status", cmd_status, "Show status" },
    { "help", cmd_help, "Show help" },
    { NULL, NULL, NULL }
};

int main(int argc, char *argv[]) {
    struct dpp_configurator_ctx *ctx;
    int ret = 0;
    char *args_str = "";
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // DPP初期化
    ctx = dpp_configurator_init();
    if (!ctx) {
        printf("Error: Failed to initialize DPP\n");
        return 1;
    }
    
    // verbose モード処理
    int cmd_idx = 1;
    if (argc > 1 && strcmp(argv[1], "-v") == 0) {
        ctx->verbose = true;
        cmd_idx = 2;
        if (argc < 3) {
            print_usage(argv[0]);
            dpp_configurator_deinit(ctx);
            return 1;
        }
    }
    
    // 引数を結合
    if (argc > cmd_idx + 1) {
        size_t total_len = 0;
        for (int i = cmd_idx + 1; i < argc; i++) {
            total_len += strlen(argv[i]) + 1; // +1 for space
        }
        
        args_str = malloc(total_len + 1);
        if (args_str) {
            args_str[0] = '\0';
            for (int i = cmd_idx + 1; i < argc; i++) {
                if (i > cmd_idx + 1) {
                    strcat(args_str, " ");
                }
                strcat(args_str, argv[i]);
            }
        }
    }
    
    // コマンド実行
    ret = execute_command(ctx, argv[cmd_idx], args_str);
    
    // クリーンアップ
    if (args_str && strlen(args_str) > 0) {
        free(args_str);
    }
    dpp_configurator_deinit(ctx);
    
    return ret;
}

int execute_command(struct dpp_configurator_ctx *ctx, const char *cmd, char *args) {
    int i;
    
    for (i = 0; commands[i].name; i++) {
        if (strcmp(cmd, commands[i].name) == 0) {
            return commands[i].handler(ctx, args);
        }
    }
    
    printf("Unknown command: %s\n", cmd);
    printf("Use 'help' to see available commands\n");
    return -1;
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [-v] <command> [args...]\n", prog_name);
    printf("Commands: configurator_add, bootstrap_gen, bootstrap_get_uri, auth_init, status, help\n");
    printf("Options:\n");
    printf("  -v    Verbose mode\n");
}
