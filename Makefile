CC = gcc
CFLAGS = -Wall

webserver: server.o functions.o
	$(CC) server.o functions.o -o webserver $(CFLAGS)

server.o: server.c
	$(CC) -c server.c -o server.o $(CFLAGS)

functions.o: functions.c
	$(CC) -c functions.c $(CFLAGS)

clean:
	@echo -e "\n# cleaning project"
	@find . -maxdepth 1 -type f -name "webserver" | xargs rm -vf
	@find . -type f -name "*.o" -o -name "*.out" | xargs rm -vf
	@find /usr/bin/ -maxdepth 1 -type f -name "webserver" | xargs rm -vf
	@echo -e "# clean completed\n"

install:
	@echo -e "\n# installing project"
	@cp webserver /usr/bin -v
	@echo -e "# install completed\n"

.PHONY: clean
