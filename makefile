#flag di compilazione
COMPILATION_FLAGS= -std=c89 -pedantic -D_GNU_SOURCE
#-Wall -Werror

#object file necessari per produrre l’eseguibile
OBJMASTER = Master.c 
OBJTAXI = Taxi.c 
OBJSOURCE = Source.c
OBJCOMMON = Common.h
TARGET = Master
ifndef SO_HEIGHT
	SO_HEIGHT = 10
endif
ifndef SO_WIDTH
	SO_WIDTH = 10
endif

$(TARGET): $(OBJMASTER) $(OBJTAXI) $(OBJSOURCE) $(OBJCOMMON)
	rm -f *.o *.so $(TARGET) Taxi Source Master *~
	$(CC) $(OBJTAXI) $(COMPILATION_FLAGS) -o Taxi
	$(CC) $(OBJSOURCE) $(COMPILATION_FLAGS) -o Source
	$(CC) -D SO_HEIGHT=$(SO_HEIGHT) -D SO_WIDTH=$(SO_WIDTH) $(OBJMASTER) $(COMPILATION_FLAGS) -o $(TARGET)	

#make all -> compila il progetto con i valori di default: SO_WIDTH=10, SO_HEIGHT=10
#make all -B SO_HEIGHT=... SO_WIDTH=... -> compila il progetto coi valori passati da riga di comando
all: $(TARGET)

#make debug: compila il progetto alcune printf utili per il debugging
debug: $(OBJMASTER) $(OBJTAXI) $(OBJSOURCE) $(OBJCOMMON)
	rm -f *.o *.so $(TARGET) Taxi Source Master *~
	$(CC) -D DEBUG $(OBJTAXI) $(COMPILATION_FLAGS) -o Taxi
	$(CC) -D DEBUG $(OBJSOURCE) $(COMPILATION_FLAGS) -o Source
	$(CC) -D DEBUG $(OBJMASTER) $(COMPILATION_FLAGS) -o $(TARGET)

#make run: lancia l'eseguibile creato dalla compilazione
run:
	./$(TARGET)

#make clean: pulisce la directory dai file oggetto
clean:
	rm –f *.o *.so $(TARGET) Taxi Source Master *~
