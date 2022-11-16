# Makefile to build the project
# NOTE: This file must not be changed.

# Parameters
CC = gcc
CFLAGS = -Wall

SRC = src/
INCLUDE = headers/
BIN = bin/
CABLE_DIR = cable/

TX_SERIAL_PORT = /dev/ttyS10
RX_SERIAL_PORT = /dev/ttyS11

TX_FILE = src/pinguim.gif
RX_FILE = src/pinguim.gif-r

# Targets
.PHONY: all
all: $(BIN)/reader $(BIN)/writer $(BIN)/cable

$(BIN)/reader: $(SRC)read_application_layer.c
	$(CC) $(CFLAGS) -o $(BIN)/reader $^ -I$(INCLUDE)

$(BIN)/writer: $(SRC)write_application_layer.c
	$(CC) $(CFLAGS) -o $(BIN)/writer $^ -I$(INCLUDE)

$(BIN)/cable: $(CABLE_DIR)/cable.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: run_tx
run_tx: $(BIN)/writer
	./$(BIN)/writer $(TX_SERIAL_PORT) $(TX_FILE)

.PHONY: run_rx
run_rx: $(BIN)/reader
	./$(BIN)/reader $(RX_SERIAL_PORT)

.PHONY: run_cable
run_cable: $(BIN)/cable
	./$(BIN)/cable

.PHONY: check_files
check_files:
	diff -s $(TX_FILE) $(RX_FILE) || exit 0

.PHONY: clean
clean:
	rm -f $(BIN)/reader
	rm -f $(BIN)/writer
	rm -f $(BIN)/cable
	rm -f $(RX_FILE)