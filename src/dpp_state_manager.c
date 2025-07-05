/*
 * DPP Configurator - State Management
 * Bootstrap and Configurator state persistence functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dpp_configurator.h"

#ifndef STUB_MODE

// Bootstrap情報の永続化
#define DPP_STATE_FILE "/tmp/dpp_configurator_state.json"

// Bootstrap情報を保存
int save_bootstrap_info(int id, const char *uri)
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
int save_configurator_info(int id, const char *curve)
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
char *load_configurator_curve(int id)
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
char *load_bootstrap_uri(int id)
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

#endif /* STUB_MODE */
