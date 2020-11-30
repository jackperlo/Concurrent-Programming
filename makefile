app: app.o molt.o somma.o
	gcc app.c -o app molt.c somma.c

run:
	./app