.SUFFIXES: .c .o
.PHONY: dep all new clean

# Compiler
CC = gcc
# Compile option
# -c: Generate object file
# -W, -Wall: Print warning about all ambigous grammer 
# -Wextra: Print warning out of -W, -Wall
# -O2: Optimization
# -g: Debugging, PLEASE DELETE AFTER PROJECT COMPLETE!
CFLAGS = -c -W -Wall -Wextra -g $(INC)

# Execute program file
CRONTAB = ssu_crontab
CROND = ssu_crond
RSYNC = ssu_rsync
# Source file
COMMON_SRCS = common.c
CRONTAB_SRCS = ssu_crontab.c
CROND_SRCS = ssu_crond.c
RSYNC_SRCS = ssu_rsync.c
CRON_SUPPORT_SRCS = cron_support.c
SRCS = $(COMMON_SRCS) $(CRONTAB_SRCS) $(CROND_SRCS) $(RSYNC_SRCS) $(CRON_SUPPORT_SRCS)
# Object file
COMMON_OBJS = $(COMMON_SRCS:.c=.o)
CRONTAB_OBJS = $(CRONTAB_SRCS:.c=.o)
CROND_OBJS = $(CROND_SRCS:.c=.o)
RSYNC_OBJS = $(RSYNC_SRCS:.c=.o)
CRON_SUPPORT_OBJS = $(CRON_SUPPORT_SRCS:.c=.o)
OBJS = $(COMMON_OBJS) $(CRONTAB_OBJS) $(CROND_OBJS) $(RSYNC_OBJS) $(CRON_SUPPORT_OBJS)
# Library file
LIBS = 
# Include path
INC = 

# Execute file grneration
# $@ = TARGET
# $^ = DEPENDENCY
# make all: Make all execute file
all : $(OBJS)
	$(CC) -o $(CRONTAB) $(COMMON_OBJS) $(CRONTAB_OBJS) $(CRON_SUPPORT_OBJS) $(LIBS)
	$(CC) -o $(CROND) $(CROND_OBJS) $(CRON_SUPPORT_OBJS) $(LIBS) -lpthread
	$(CC) -o $(RSYNC) $(COMMON_OBJS) $(RSYNC_OBJS) $(LIBS)
$(CRONTAB) : $(COMMON_OBJS) $(CRONTAB_OBJS) $(CRON_SUPPORT_OBJS)
	$(CC) -o $@ $^ $(LIBS)
$(CROND) : $(CROND_OBJS) $(CRON_SUPPORT_OBJS)
	$(CC) -o $@ $^ $(LIBS) -lpthread
$(RSYNC) : $(COMMON_OBJS) $(RSYNC_OBJS)
	$(CC) -o $@ $^ $(LIBS)

# Object file generation
$(OBJS):
	$(CC) $(CFLAGS) $(SRCS)

# make dep: Make dependency information file
dep:
	$(CC) -M $(INC) $(SRCS) > .dependency

# make new: Re-generation 
new:
	$(MAKE) clean
	$(MAKE) all

# make clean: Remove all generated file
clean:
	rm -rf $(OBJS) $(CRONTAB) $(CROND) $(RSYNC)

