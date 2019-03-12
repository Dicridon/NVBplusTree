TARGET = perf
CC = gcc
CFLAGS = -g -std=gnu99
LIBS = -lpmemobj
SRCS = $(shell ls *c)
OBJS = $(SRCS:.c=.o)

perf:$(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS) 

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o $(TARGET) -f
