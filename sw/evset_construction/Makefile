THIS_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Init compilation flags
COPT     ?= -g -O2
CPPFLAGS ?= -std=c++11
CXX      ?= g++
LDFLAGS  ?=

# stack execution protection
LDFLAGS +=-z noexecstack

# data relocation and projection
LDFLAGS +=-z relro -z now

# stack buffer overrun detection
CFLAGS  +=-fstack-protector

# Position independent execution
CFLAGS  +=-fPIE -fPIC
LDFLAGS +=-pie

# fortify source
CFLAGS +=-D_FORTIFY_SOURCE=2

# format string vulnerabilities
CFLAGS +=-Wformat -Wformat-security
LDFLAGS += -luuid

FPGA_LIBS = -lopae-c

# Build directory
OBJDIR = obj
CFLAGS += -I./$(OBJDIR) -I../evsets
CPPFLAGS += -I./$(OBJDIR)

# Files and folders
SRCS_A = main.c \
mmap_shared_mem.c mmap_synchronisation.c mmap_evict_mem.c \
cache_utils.c
OBJS_A = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS_A)))

# Targets
all: clean app

app: $(OBJS_A)
	$(CC) -o $@ $^ $(LDFLAGS) $(FPGA_LIBS) -lrt -lm -lnuma

$(OBJDIR)/%.o: %.c | objdir
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf app $(OBJDIR)

objdir:
	@mkdir -p $(OBJDIR)

.PHONY: all clean