CC = gcc
CFLAGS = -D_REENTRANT -g -Wall -Wno-unknown-pragmas
LDFLAGS = -lpthread -pthread

##################### Compilation Targets #####################

web_server: server.o util.c util.h
	${CC} $(CFLAGS) -o web_server server.o util.c ${LDFLAGS}

###############################################################

##################### Testing Targets #########################
test_prelim:
	@# Port: passed_in | Path: ./testing | num_dispatcher: 1 | num_workers: 1
	@# queue_length: 50 | cache_size: 50

	@read -p "Enter a random port between 1024 & 65536: " sock_in; \
	./web_server $$sock_in ./testing

test:
	@# Port: passed_in | Path: ./testing | num_dispatcher: 5 | num_workers: 5
	@# queue_length: 50 | cache_size: 50

	@read -p "Enter a random port between 1024 & 65536: " sock_in; \
	./web_server $$sock_in ./testing

test_full:
	@# Port: passed_in | Path: ./testing | num_dispatcher: 5 | num_workers: 5
	@# queue_length: 50 | cache_size: 50

	@read -p "Enter a random port between 1024 & 65536: " sock_in; \
	./web_server $$sock_in ./testing

####################################################################

##################### Solution Running Targets #####################

prelim_solution:
	@# Port: passed_in | Path: ./testing | num_dispatcher: 1 | num_workers: 1
	@# queue_length: 50 | cache_size: 50
	@chmod +x server_prelim_sol
	@read -p "Enter a random port between 1024 & 65536: " sock_in; \
	./server_prelim_sol $$sock_in testing

solution:
	@# Port: passed_in | Path: ./testing | num_dispatcher: 5 | num_workers: 5
	@# queue_length: 50 | cache_size: 50
	@chmod +x web_server_sol
	@read -p "Enter a random port between 1024 & 65536: " sock_in; \
	./web_server_sol $$sock_in ./testing

####################################################################

##################### Utility Targets #####################

force_kill:
	@echo -n "Force Killing web_server proc PID: "; ps -C "web_server" -o pid=
	@ps -C "web_server" -o pid= | xargs kill -9
	@echo -n "Force Killing parent proc PID: "; ps -C "make" -o pid=
	@ps -C "make" -o pid= | xargs kill -9

clean:
	rm -f web_server webserver_log *.tar.gz
	@clear
	@echo "Succesful Clean"

####################################################################
