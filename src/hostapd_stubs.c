/*
 * Stub functions for hostapd-specific functions not needed for DPP CLI operation
 *
 * This file provides minimal stub implementations for hostapd functions that are
 * referenced by the DPP library but not needed for standalone CLI operation.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef STUB_MODE
#include "utils/common.h"
#include "common/dpp.h"

/* ==== Network Address Parsing Stubs ==== */
int hostapd_parse_ip_addr(const char *txt, union hostapd_addr *addr)
{
    (void)txt;
    (void)addr;
    return -1; // Not needed for CLI DPP operation
}

char *hostapd_ip_txt(const union hostapd_addr *addr, char *buf, size_t buflen)
{
    (void)addr;
    (void)buf;
    (void)buflen;
    return NULL; // Not needed for CLI DPP operation
}

/* ==== DPP TCP/Relay Stubs ==== */
void dpp_tcp_init_flush(struct dpp_global *dpp)
{
    (void)dpp;
    // TCP relay not needed for CLI operation
}

void dpp_relay_flush_controllers(struct dpp_global *dpp)
{
    (void)dpp;
    // Relay controllers not needed for CLI operation
}

void dpp_controller_stop(struct dpp_global *dpp)
{
    (void)dpp;
    // Controller not needed for CLI operation
}

/* ==== GAS (Generic Advertisement Service) Stubs ==== */
struct wpabuf *gas_build_initial_req(u8 dialog_token, size_t len)
{
    (void)dialog_token;
    (void)len;
    return NULL; // GAS not needed for CLI operation
}

int gas_query_ap_req(struct gas_query_ap *gas, const u8 *dst, int freq,
                     struct wpabuf *req,
                     void (*cb)(void *ctx, const u8 *dst, const u8 *bssid,
                                const u8 *resp, size_t len, u16 status_code),
                     void *ctx)
{
    (void)gas;
    (void)dst;
    (void)freq;
    (void)req;
    (void)cb;
    (void)ctx;
    return -1; // GAS not needed for CLI operation
}

struct gas_query_ap *gas_query_ap_init(void *ctx, const struct gas_query_ap_cbs *cbs)
{
    (void)ctx;
    (void)cbs;
    return NULL; // GAS not needed for CLI operation
}

void gas_query_ap_deinit(struct gas_query_ap *gas)
{
    (void)gas;
    // GAS not needed for CLI operation
}

/* ==== Asymmetric Key Stubs ==== */
void dpp_free_asymmetric_key(struct dpp_asymmetric_key *key)
{
    (void)key;
    // Asymmetric keys not needed for basic CLI operation
}

/* ==== DPP Certificate/Enrollment Stubs ==== */
struct wpabuf *dpp_build_enveloped_data(struct dpp_authentication *auth)
{
    (void)auth;
    return NULL; // Certificate enrollment not needed for CLI operation
}

int dpp_conf_resp_env_data(struct dpp_authentication *auth,
                           const u8 *env_data, size_t env_data_len)
{
    (void)auth;
    (void)env_data;
    (void)env_data_len;
    return -1; // Envelope data not needed for CLI operation
}

/* ==== Event Loop Stubs ==== */
int eloop_register_read_sock(int sock, void (*handler)(int, void *, void *),
                             void *eloop_data, void *user_data)
{
    (void)sock;
    (void)handler;
    (void)eloop_data;
    (void)user_data;
    return 0; // Event loop not needed for CLI operation
}

void eloop_unregister_read_sock(int sock)
{
    (void)sock;
    // Event loop not needed for CLI operation
}

#endif /* !STUB_MODE */
