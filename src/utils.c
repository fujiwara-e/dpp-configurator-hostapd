#include "../include/dpp_configurator.h"

// 引数解析ユーティリティ
char *parse_argument(char *args, const char *key)
{
    if (!args || !key)
    {
        return NULL;
    }

    char *args_copy = strdup(args);
    if (!args_copy)
    {
        return NULL;
    }

    char *result = NULL;
    char key_pattern[64];
    snprintf(key_pattern, sizeof(key_pattern), "%s=", key);
    size_t key_len = strlen(key_pattern);

    char *token = strtok(args_copy, " ");
    while (token)
    {
        if (strncmp(token, key_pattern, key_len) == 0)
        {
            result = strdup(token + key_len);
            break;
        }
        token = strtok(NULL, " ");
    }

    free(args_copy);
    return result;
}

// 文字列を16進数エンコードする関数
char *encode_hex_string(const char *str)
{
    if (!str)
    {
        return NULL;
    }

    size_t len = strlen(str);
    char *hex_str = malloc(len * 2 + 1);
    if (!hex_str)
    {
        return NULL;
    }

    for (size_t i = 0; i < len; i++)
    {
        sprintf(hex_str + i * 2, "%02x", (unsigned char)str[i]);
    }
    hex_str[len * 2] = '\0';

    return hex_str;
}

// 16進数文字列をデコードする関数
char *decode_hex_string(const char *hex_str)
{
    if (!hex_str)
    {
        return NULL;
    }

    size_t hex_len = strlen(hex_str);
    if (hex_len % 2 != 0)
    {
        return NULL; // 奇数長は無効
    }

    size_t str_len = hex_len / 2;
    char *str = malloc(str_len + 1);
    if (!str)
    {
        return NULL;
    }

    for (size_t i = 0; i < str_len; i++)
    {
        unsigned int byte;
        if (sscanf(hex_str + i * 2, "%2x", &byte) != 1)
        {
            free(str);
            return NULL;
        }
        str[i] = (char)byte;
    }
    str[str_len] = '\0';

    return str;
}

// 文字列が16進数かどうかを判定する関数
bool is_hex_string(const char *str)
{
    if (!str || strlen(str) == 0)
    {
        return false;
    }

    for (size_t i = 0; i < strlen(str); i++)
    {
        if (!((str[i] >= '0' && str[i] <= '9') ||
              (str[i] >= 'a' && str[i] <= 'f') ||
              (str[i] >= 'A' && str[i] <= 'F')))
        {
            return false;
        }
    }
    return strlen(str) % 2 == 0; // 偶数長である必要がある
}
