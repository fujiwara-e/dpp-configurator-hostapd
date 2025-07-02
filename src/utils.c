#define _GNU_SOURCE
#include "../include/dpp_configurator.h"

// 引数解析ユーティリティ
char *parse_argument(char *args, const char *key) {
    if (!args || !key) {
        return NULL;
    }
    
    char *args_copy = strdup(args);
    if (!args_copy) {
        return NULL;
    }
    
    char *result = NULL;
    char key_pattern[64];
    snprintf(key_pattern, sizeof(key_pattern), "%s=", key);
    size_t key_len = strlen(key_pattern);
    
    char *token = strtok(args_copy, " ");
    while (token) {
        if (strncmp(token, key_pattern, key_len) == 0) {
            result = strdup(token + key_len);
            break;
        }
        token = strtok(NULL, " ");
    }
    
    free(args_copy);
    return result;
}
