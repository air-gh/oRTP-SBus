
CC=gcc

FLAGS= -O2 -D_GNU_SOURCE

all: rtprecv rtpsend ptsrecv ptssend

rtprecv: rtprecv.c
	$(CC) $(FLAGS) $< -o $@ -lortp -pthread
rtpsend: rtpsend.c
	$(CC) $(FLAGS) $< -o $@ -lortp -pthread
ptsrecv: ptsrecv.c
	$(CC) $(FLAGS) $< -o $@ -lrt
ptssend: ptssend.c
	$(CC) $(FLAGS) $< -o $@ -lrt

clean:
	rm -rf rtprecv rtpsend ptsrecv ptssend *.o core
