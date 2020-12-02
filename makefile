#flag di compilazione
COMPILATION_FLAGS= -std=c89 -pedantic -D_GNU_SOURCE
#-Wall -Werror

#definizione di variabili d'ambiente
export SO_WIDTH := 10;
export SO_HEIGHT := 10;

#object file necessari per produrre l’eseguibile
OBJMASTER = Master.c 
OBJTAXI = Taxi.c 
OBJSOURCE = Source.c

TARGET = Master

$(TARGET): $(OBJMASTER) $(OBJTAXI) $(OBJSOURCE)
	rm -f *.o $(TARGET) Taxi Source *~
	$(CC) $(OBJTAXI) $(COMPILATION_FLAGS) -o Taxi
	$(CC) $(OBJSOURCE) $(COMPILATION_FLAGS) -o Source
	$(CC) $(OBJMASTER) $(COMPILATION_FLAGS) -o $(TARGET)

#make all: compila il progetto
all: $(TARGET)

#make debug: compila il progetto alcune printf utili per il debugging
debug: $(OBJMASTER) $(OBJTAXI) $(OBJSOURCE)
	rm -f *.o $(TARGET) Taxi Source *~
	$(CC) -D DEBUG $(OBJTAXI) $(COMPILATION_FLAGS) -o Taxi
	$(CC) -D DEBUG $(OBJSOURCE) $(COMPILATION_FLAGS) -o Source
	$(CC) -D DEBUG $(OBJMASTER) $(COMPILATION_FLAGS) -o $(TARGET)

#make run: lancia l'eseguibile creato dalla compilazione
run:
	./$(TARGET)

#make clean: pulisce la directory dai file oggetto
clean:
	rm –f *.o $(TARGET) Taxi Source *~
