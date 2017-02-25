CC = gcc
CFLAGS = -Wall -c
LDFLAGS = -g
PARTICIPANTOBJS = prog3_participant.c
OBSERVEROBJS = prog3_observer.c
SERVEROBJS = prog3_server.c
EXES = prog3_server prog3_participant prog3_observer

all:	$(EXES)
prog3_participant:	$(PARTICIPANTOBJS)
	$(CC) -o $@ $(LDFLAGS) $(PARTICIPANTOBJS)
prog3_observer:	$(OBSERVEROBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBSERVEROBJS)
prog3_server:	$(SERVEROBJS)
	$(CC) -o $@ $(LDFLAGS) $(SERVEROBJS)
%.o:	%.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f *.o $(EXES)
