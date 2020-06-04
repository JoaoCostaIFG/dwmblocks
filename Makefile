PREFIX ?= /usr/local

CC=gcc

output: dwmblocks.c config.h util.c util.h
	${CC} dwmblocks.c util.c -o dwmblocks `pkg-config --cflags x11` `pkg-config --libs x11`
clean:
	rm -f *.o *.gch dwmblocks
install: output
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f dwmblocks $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/dwmblocks
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/dwmblocks

.PHONY: output clean install uninstall
