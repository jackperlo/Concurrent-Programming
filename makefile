COMPILATION_FLAGS=-Wall -Werror -std=c89 -pedantic -D_GNU_SOURCE

#make all: compila solo i sorgenti che sono stati modificati, secondo le loro dipendenze
all: Taxi.o Source.o Master.o
	gcc -o all Taxi.o Source.o Master.o

Master.o: Master.c Source.h Taxi.h
	gcc -c $(COMPILATION_FLAGS) -D SO_WIDTH=8 -D SO_HEIGHT=8 Master.c
Taxi.o: Taxi.c Taxi.h
	gcc -c $(COMPILATION_FLAGS) Taxi.c
Source.o: Source.c Source.h
	gcc -c $(COMPILATION_FLAGS) Source.c


#make clean: pulisce la directory dai file oggetto
clean:
	rm –f *.o
#make run: lancia l'eseguibile creato dalla compilazione
run:
	./all 5 1
#parametri d'esecuzione: SO_HOLES, SO_SOURCES