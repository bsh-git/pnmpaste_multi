pnmpaste_multi: pnmpaste_multi.o util.o
	$(CC) -o $@ pnmpaste_multi.o util.o -lnetpbm

pnmpaste: pnmpaste.o util.o
	$(CC) -o $@ pnmpaste.o util.o -lnetpbm

pnmpaste_multi.o: pnmpaste_multi.c util.h
pnmpaste.o: pnmpaste.c util.h
util.o: util.c util.h


.PHONY: clean
clean:
	rm -f *.o pnmpaste_multi pnmpaste
