balong-malalim: parts.o patcher.o prepare_loader.o send_loader.o open_port.o balong-malalim.c

	c89 parts.o patcher.o prepare_loader.o send_loader.o open_port.o balong-malalim.c -o balong-malalim

parts.o: parts.c

	c89 -c parts.c

patcher.o: patcher.c

	c89 -c patcher.c

prepare_loader.o: prepare_loader.c 

	c89 -c prepare_loader.c

send_loader.o: send_loader.c

	c89 -c send_loader.c

open_port.o: open_port.c

	c89 -c open_port.c
	
