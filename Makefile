CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -g -D_GNU_SOURCE
LDFLAGS = 

SRC_DIR = src
INC_DIR = include

SRCS = \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/chroot_cmd.c \
	$(SRC_DIR)/run_cmd.c \
	$(SRC_DIR)/cgroup.c \
	$(SRC_DIR)/image.c \
	$(SRC_DIR)/util.c

OBJS = $(SRCS:.c=.o)

TARGET = minictl

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
