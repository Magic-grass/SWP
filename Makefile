# cory and nova use gcc version 3.3.2

TARGET=project2

CC = gcc
DEBUG = -g #-v

LDFLAGS = -lresolv -lnsl -lpthread -lm

OS = LINUX

CCFLAGS = -Wall $(DEBUG) -D$(OS)

# add object file names here
OBJS = main.o util.o input.o communicate.o sender.o receiver.o crc.o receiverUtil.o

all: project2

%.o : %.c
	$(CC) -c $(CCFLAGS) $<

%.o : %.cc
	$(CC) -c $(CCFLAGS) $<

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CCFLAGS) $(LDFLAGS) 
	./.client

clean:
	rm -f $(TARGET) core *.o *~ 

