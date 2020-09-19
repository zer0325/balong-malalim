balong-malalim: parts.o patcher.o balong-malalim.c

	c89 parts.o patcher.o balong-malalim.c -o balong-malalim

parts.o: parts.c

	c89 -c parts.c

patcher.o: patcher.c

	c89 -c patcher.c
