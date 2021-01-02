CC=gcc
COMPILATION_FLAGS= -std=c89 -pedantic -Wall -Werror -D_GNU_SOURCE 
DEPENDENCIES = Common.h 

OBJTAXI = Taxi.o Common.o
OBJSOURCE = Source.o Common.o
OBJMASTER = Master.o Common.o

%.o: %.c $(DEPENDENCIES)
	$(CC) -c -o $@ $< $(CFLAGS)

all: Taxi Source Master

Taxi: $(OBJTAXI)
	$(CC) $(COMPILATION_FLAGS) -o Taxi $(OBJTAXI) 

Source: $(OBJSOURCE)
	$(CC) $(COMPILATION_FLAGS) -o Source $(OBJSOURCE) 

Master: $(OBJMASTER)
	$(CC) $(COMPILATION_FLAGS) -o Master $(OBJMASTER) 

run:
	./Master

clean:
	rm –f *.o *.so Taxi Source Master *~
