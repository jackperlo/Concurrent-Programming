CC=gcc
COMPILATION_FLAGS= -std=c89 -pedantic -Wall -Werror
DEPENDENCIES = Common.h Communication.h Cleaner.h

OBJTAXI = Taxi.o Communication.o Cleaner.o 
OBJSOURCE = Source.o Communication.o Cleaner.o
OBJMASTER = Master.o Communication.o Cleaner.o

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
