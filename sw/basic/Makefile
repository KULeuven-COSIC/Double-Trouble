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

SRCS_APP = main.c attacker_helper.c victim.c 								   \
mmap_shared_mem.c mmap_synchronisation.c mmap_evict_mem.c      \
cache_utils.c

SRCS_BASIC      = $(SRCS_APP) attacker_basic.c 
SRCS_HISTOGRAM  = $(SRCS_APP) attacker_histogram.c 
SRCS_EVS_CANDID = $(SRCS_APP) attacker_evscandid.c
SRCS_EVS_REDUC  = $(SRCS_APP) attacker_evictionreduced.c
SRCS_REP_POLICY = $(SRCS_APP) attacker_reppolicy.c

OBJS_BASIC 	    = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS_BASIC)))
OBJS_HISTOGRAM  = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS_HISTOGRAM)))
OBJS_EVS_CANDID = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS_EVS_CANDID)))
OBJS_EVS_REDUC  = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS_EVS_REDUC)))
OBJS_REP_POLICY = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRCS_REP_POLICY)))

##########
# Targets
##########
all: clean app_basic 

# Basic experiments
app_basic: $(OBJS_BASIC)
	$(CC) -o $@ $^ $(LDFLAGS) $(FPGA_LIBS) -lrt -lm -lnuma 

# Histogram of cache access timings
histogram: clean app_histogram
app_histogram: $(OBJS_HISTOGRAM)  
	$(CC) -o $@ $^ $(LDFLAGS) $(FPGA_LIBS) -lrt -lm -lnuma

# Eviction set candidate
evscandid: clean app_evscandid
app_evscandid: $(OBJS_EVS_CANDID)  
	$(CC) -o $@ $^ $(LDFLAGS) $(FPGA_LIBS) -lrt -lm -lnuma

# Eviction with reduced eviction set
evsred: clean app_evsred
app_evsred: $(OBJS_EVS_REDUC)  
	$(CC) -o $@ $^ $(LDFLAGS) $(FPGA_LIBS) -lrt -lm -lnuma

# For replacement policy examples
reppolicy: clean app_reppolicy
app_reppolicy: $(OBJS_REP_POLICY)  
	$(CC) -o $@ $^ $(LDFLAGS) $(FPGA_LIBS) -lrt -lm -lnuma

$(OBJDIR)/%.o: %.c | objdir
	$(CC) $(CFLAGS) -c $< -o $@

objdir:
	@mkdir -p $(OBJDIR) 

.PHONY: clean
clean:
	rm -rf ./app*
	rm -rf $(OBJDIR) *_gen.c *_gen.h
	mkdir -p log
	rm -rf ./log/*
	