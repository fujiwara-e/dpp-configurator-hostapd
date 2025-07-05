# DPP Configurator with hostapd integration

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2 -D_GNU_SOURCE
LDFLAGS = -lcrypto -lssl

# hostapd library paths
HOSTAPD_DIR = /home/fujiwara-e/git/hostap
DPP_LIB_DIR = $(HOSTAPD_DIR)/src/common
UTILS_LIB_DIR = $(HOSTAPD_DIR)/src/utils
CRYPTO_LIB_DIR = $(HOSTAPD_DIR)/src/crypto

# Include paths
INCLUDES = -I./include \
           -I$(DPP_LIB_DIR) \
           -I$(UTILS_LIB_DIR) \
           -I$(CRYPTO_LIB_DIR) \
           -I$(HOSTAPD_DIR)/src

# Source files
SRCS = src/main.c \
       src/utils.c

# hostapd integration sources
HOSTAPD_SRCS = src/dpp_operations_hostapd.c \
               src/dpp_hostapd_core.c \
               src/dpp_state_manager.c \
               src/dpp_basic_commands.c \
               src/dpp_auth_commands.c \
               src/dpp_monitoring_commands.c \
               src/dpp_diagnostic_commands.c \
               src/dpp_help_command.c \
               src/hostapd_stubs.c

# Stub mode sources
STUB_SRCS = src/dpp_operations.c

TARGET = dpp-configurator

# Build modes
.PHONY: all stub hostapd clean test install check-hostapd

all: stub

# Stub mode (current implementation)
stub: CFLAGS += -DSTUB_MODE
stub: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS) $(STUB_SRCS) $(LDFLAGS) -o $(TARGET)

# hostapd integration mode
hostapd: CFLAGS := $(filter-out -DSTUB_MODE,$(CFLAGS))
hostapd: CFLAGS += -DCONFIG_DPP -DCONFIG_DPP2 -DCONFIG_HMAC_SHA256_KDF -DCONFIG_HMAC_SHA384_KDF -DCONFIG_HMAC_SHA512_KDF -DCONFIG_JSON -DCONFIG_ECC -DCONFIG_SHA256 -DCONFIG_SHA384 -DCONFIG_SHA512 -Wno-unused-parameter
hostapd: LDFLAGS += $(shell pkg-config --libs libnl-3.0 libnl-genl-3.0)
hostapd: 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET)-hostapd \
		$(SRCS) $(HOSTAPD_SRCS) \
		$(DPP_LIB_DIR)/dpp.c \
		$(DPP_LIB_DIR)/dpp_auth.c \
		$(DPP_LIB_DIR)/dpp_crypto.c \
		$(DPP_LIB_DIR)/ieee802_11_common.c \
		$(UTILS_LIB_DIR)/common.c \
		$(UTILS_LIB_DIR)/wpabuf.c \
		$(UTILS_LIB_DIR)/os_unix.c \
		$(UTILS_LIB_DIR)/base64.c \
		$(UTILS_LIB_DIR)/wpa_debug.c \
		$(UTILS_LIB_DIR)/json.c \
		$(CRYPTO_LIB_DIR)/crypto_openssl.c \
		$(CRYPTO_LIB_DIR)/random.c \
		$(CRYPTO_LIB_DIR)/aes-siv.c \
		$(CRYPTO_LIB_DIR)/aes-ctr.c \
		$(CRYPTO_LIB_DIR)/sha256-kdf.c \
		$(CRYPTO_LIB_DIR)/sha384-kdf.c \
		$(CRYPTO_LIB_DIR)/sha512-kdf.c \
		$(LDFLAGS)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -D $(TARGET) /usr/local/bin/$(TARGET)

test: $(TARGET)
	@echo "Running basic test..."
	./$(TARGET) help

# Development targets
check-hostapd:
	@echo "Checking hostapd paths..."
	@test -f "$(DPP_LIB_DIR)/dpp.c" && echo "✓ dpp.c found" || echo "✗ dpp.c not found"
	@test -f "$(UTILS_LIB_DIR)/common.c" && echo "✓ common.c found" || echo "✗ common.c not found"  
	@test -f "$(CRYPTO_LIB_DIR)/crypto_openssl.c" && echo "✓ crypto_openssl.c found" || echo "✗ crypto_openssl.c not found"
	@echo "hostapd paths check completed."
