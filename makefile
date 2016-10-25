CC=g++
#CFLAGS+=-g
CFLAGS+=`pkg-config --cflags opencv` -lpigpio -lrt -lpthread -O3 -lncurses
LDFLAGS+=`pkg-config --libs opencv` -lpigpio -lrt -lpthread -lncurses 

PROG=teste

OBJS=main.o pid.o

.PHONY: all clean
$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CC) -c $(CFLAGS) $<

all: $(PROG)

clean:
	rm -f $(OBJS) $(PROG)
