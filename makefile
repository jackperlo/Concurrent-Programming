#flag di compilazione
COMPILATION_FLAGS= -std=c89 -pedantic -D_GNU_SOURCE
#-Wall -Werror
#definizione di variabili d'ambiente
export SO_WIDTH := 10;
export SO_HEIGHT := 10;

#make all: compila solo i sorgenti che sono stati modificati, secondo le loro dipendenze
all: Taxi.o Source.o Master.o
	gcc -o all Master.o 

Master.o: Master.c
	gcc -c $(COMPILATION_FLAGS) -D SO_HEIGHT=8 -D SO_WIDTH=8 Master.c
Taxi.o: Taxi.c
	gcc -c $(COMPILATION_FLAGS) Taxi.c
Source.o: Source.c
	gcc -c $(COMPILATION_FLAGS) Source.c

#make clean: pulisce la directory dai file oggetto
clean:
	rm –f *.o
#make run: lancia l'eseguibile creato dalla compilazione
run:
	./all 