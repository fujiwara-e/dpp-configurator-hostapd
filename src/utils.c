#include "../include/dpp_configurator.h"

// 引数解析ユーティリティ（JSON対応版）
char *parse_argument(char *args, const char *key)
{
    if (!args || !key)
    {
        return NULL;
    }

    char key_pattern[64];
    snprintf(key_pattern, sizeof(key_pattern), "%s=", key);
    size_t key_len = strlen(key_pattern);

    char *pos = strstr(args, key_pattern);
    if (!pos)
    {
        return NULL;
    }

    // キーの位置を見つけたので、値の開始位置を特定
    char *value_start = pos + key_len;
    
    // 引用符で囲まれているかチェック
    if (*value_start == '"' || *value_start == '\'')
    {
        char quote_char = *value_start;
        value_start++; // 引用符をスキップ
        
        // 対応する閉じ引用符を探す
        char *value_end = value_start;
        while (*value_end && *value_end != quote_char)
        {
            // エスケープされた引用符をスキップ
            if (*value_end == '\\' && *(value_end + 1))
            {
                value_end += 2;
            }
            else
            {
                value_end++;
            }
        }
        
        if (*value_end == quote_char)
        {
            size_t value_len = value_end - value_start;
            char *result = malloc(value_len + 1);
            if (result)
            {
                strncpy(result, value_start, value_len);
                result[value_len] = '\0';
            }
            return result;
        }
    }
    
    // 引用符で囲まれていない場合は、従来の処理
    char *value_end = value_start;
    while (*value_end && *value_end != ' ')
    {
        value_end++;
    }
    
    size_t value_len = value_end - value_start;
    char *result = malloc(value_len + 1);
    if (result)
    {
        strncpy(result, value_start, value_len);
        result[value_len] = '\0';
    }
    
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

// Matter PINが有効かどうかを判定する関数
bool is_valid_matter_pin(const char *pin)
{
    if (!pin)
    {
        return false;
    }

    // 長さチェック（正確に8桁）
    if (strlen(pin) != 8)
    {
        return false;
    }

    // 数字のみチェック
    for (size_t i = 0; i < 8; i++)
    {
        if (pin[i] < '0' || pin[i] > '9')
        {
            return false;
        }
    }

    return true;
}
