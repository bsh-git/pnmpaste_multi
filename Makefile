pnmpaste: pnmpaste.o util.o
	$(CC) -o $@ pnmpaste.o util.o -lnetpbm

pnmpaste.o: pnmpaste.c util.h
util.o: util.c util.h
