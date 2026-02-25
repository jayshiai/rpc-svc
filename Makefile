CC = gcc
RPCGEN = rpcgen
UNAME_S := $(shell uname -s)

SRC_DIR = src
GEN_DIR = generated
STUB_DIR = stubs
BIN_DIR = bin

CFLAGS = -Wall -I$(GEN_DIR)
LDFLAGS =

ifeq ($(UNAME_S),Darwin)
    CFLAGS += -Wno-deprecated-non-prototype
endif

ifeq ($(UNAME_S),Linux)
    CFLAGS += -I/usr/include/tirpc
    LDFLAGS = -ltirpc
endif

X_FILE       = $(SRC_DIR)/filemanager.x
GEN_H        = $(GEN_DIR)/filemanager.h
GEN_SVC      = $(GEN_DIR)/filemanager_svc.c
GEN_XDR      = $(GEN_DIR)/filemanager_xdr.c
SRV_SRC      = $(SRC_DIR)/filemanager_server.c
SRV_STUB_SRC = $(STUB_DIR)/filemanager_svc_stub.c
CLI_SRC      = $(SRC_DIR)/filemanager_client.c
CLI_STUB_SRC = $(STUB_DIR)/filemanager_clnt_stub.c
SERVER_BIN   = $(BIN_DIR)/filemanager_server
CLIENT_BIN   = $(BIN_DIR)/filemanager_client

.PHONY: all clean dirs

all: dirs $(SERVER_BIN) $(CLIENT_BIN)

dirs:
	@mkdir -p $(GEN_DIR) $(BIN_DIR)

$(GEN_H): $(X_FILE) | dirs
	$(RPCGEN) -h $< -o $@

$(GEN_XDR): $(X_FILE) | dirs
	$(RPCGEN) -c $< -o $@

$(GEN_SVC): $(X_FILE) | dirs
	$(RPCGEN) -m $< -o $@

$(SERVER_BIN): $(GEN_SVC) $(GEN_XDR) $(GEN_H) $(SRV_SRC) $(SRV_STUB_SRC)
	$(CC) $(CFLAGS) -o $@ $(GEN_SVC) $(SRV_SRC) $(SRV_STUB_SRC) $(GEN_XDR) $(LDFLAGS)

$(CLIENT_BIN): $(GEN_XDR) $(GEN_H) $(CLI_SRC) $(CLI_STUB_SRC)
	$(CC) $(CFLAGS) -o $@ $(CLI_STUB_SRC) $(CLI_SRC) $(GEN_XDR) $(LDFLAGS)

clean:
	rm -rf $(GEN_DIR) $(BIN_DIR)
