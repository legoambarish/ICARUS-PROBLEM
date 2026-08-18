CC := gcc
CFLAGS := -O1 -fno-stack-protector -D_FORTIFY_SOURCE=0 -std=c11 -Wall -Iinclude
LDFLAGS := -L./lib/static -lobc_physics -lm
TARGET := obc_sim
SRC := $(shell find src -name '*.c' | sort)
OBJ := $(SRC:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)
