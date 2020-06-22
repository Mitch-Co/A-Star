# all and clean are not file names
.PHONY = all clean 

CC=gcc
CFLAGS=-std=c99 -I ./src -I ./src/headers

HED_DIR=./src/headers
SRC_DIR=./src
BIN_DIR=./bin

HEDS := $(wildcard $(HED_DIR)/*.h)
SRCS := $(wildcard $(SCR_DIR)/*.c)
OBJS := $(addprefix $(BIN_DIR)/,$(notdir $(HEDS:%.h=%.o)))
PROG_NAME=solver
PROG_BIN=$(BIN_DIR)/$(PROG_NAME)
PROG_SRC=$(SRC_DIR)/$(PROG_NAME).c

all: ${OBJS} $(PROG_BIN)

$(PROG_BIN): ${OBJS} $(PROG_SRC)
	$(CC) $^ $(CFLAGS) -o $@

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c $(HED_DIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(BIN_DIR)/*.o

