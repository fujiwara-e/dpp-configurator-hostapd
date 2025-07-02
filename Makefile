# 簡素版 DPP Configurator Makefile (スタブ版)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2 -D_GNU_SOURCE
LDFLAGS = 

# ソースファイル
SRCS = src/main.c \
       src/dpp_operations.c \
       src/utils.c

TARGET = dpp-configurator

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -I./include -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -D $(TARGET) /usr/local/bin/$(TARGET)

test: $(TARGET)
	@echo "Running basic test..."
	./$(TARGET) help

.PHONY: all clean install test
