THIS_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

########################
# Set compilation flags
#######################

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

##################
# Build directory
##################
OBJDIR = obj
CFLAGS += -I./$(OBJDIR) #-Wall
CPPFLAGS += -I./$(OBJDIR)

####################
# Files and folders
####################
SRCS_SEC_WRITE = main.c sec_write.c victim.c attacker_helper.c \
mmap_shared_mem.c mmap_synchronisation.c mmap_evict_mem.c \
cache_utils.c

SRCS_CPU_READ = main.c shared_access_cpu_read.c victim.c attacker_helper.c \
mmap_shared_mem.c mmap_synchronisation.c mmap_evict_mem.c \
cache_utils.c

SRCS_CPU_WRITE = main.c cpu_write.c victim.c attacker_helper.c \
mmap_shared_mem.c mmap_synchronisation.c mmap_evict_mem.c \
cache_utils.c

OBJS_CPU_READ = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS_CPU_READ)))
OBJS_CPU_WRITE = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS_CPU_WRITE)))
OBJS_SEC_WRITE = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS_SEC_WRITE)))

##########
# Targets
##########
all: clean app_sec_write app_shared_cpu_read app_cpu_write

cpu_wr: clean app_cpu_write
	./app_cpu_write

shared_cpu_rd: clean app_shared_cpu_read
	./app_shared_cpu_read

sec_wr: clean app_sec_write
	./app_sec_write

clean:
	rm -rf app_shared app_shared_cpu_read app_cpu_write app_sec_write app_ase 
	rm -rf $(OBJDIR) 
	mkdir -p log
	rm -rf ./log/*
	
app_sec_write: $(OBJS_SEC_WRITE)  # Secondary write (Fig 4e)
	$(CC) -o $@ $^ $(LDFLAGS) $(FPGA_LIBS) -lrt -lm -lnuma 

app_shared_cpu_read: $(OBJS_CPU_READ)  # Shared access and CPU read (Fig 4d & 4f)
	$(CC) -o $@ $^ $(LDFLAGS) $(FPGA_LIBS) -lrt -lm -lnuma 

app_cpu_write: $(OBJS_CPU_WRITE)  # CPU write (Fig 4c)
	$(CC) -o $@ $^ $(LDFLAGS) $(FPGA_LIBS) -lrt -lm -lnuma 
	
$(OBJDIR)/%.o: %.c | objdir
	$(CC) $(CFLAGS) -c $< -o $@

objdir:
	@mkdir -p $(OBJDIR) 

.PHONY: all clean
