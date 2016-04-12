
oRTP-SBUS
=========
oRTP-SBUS is Futaba S.Bus over RTP sender+receiver using oRTP library.
SBUS source+sink using pts is also included for testing without
real SBUS devices.
oRTP-SBUS sender+receiver is based on oRTP sample at src/tests.
SBUS source+sink is based on tty0tty-1.2 using pts.

    SBUS generator -> /dev/pts/a -> RTP -> /dev/pts/b -> throw away
                ptssend      rtpsend   rtprecv      ptsrecv

oRTP library
------------
Before building oRTP-SBUS, you should make oRTP library with
posixtimer_interval=1000, because SBUS interval is 7ms.
posixtimer_interval is defined at "configure.ac" of oRTP.
oRTP on linphone-3.6.1 is required because of inter-version
API incompatilibity.

Make
----
Set real oRTP library compiled by posixtimer_interval=1000
into Makefile.  And,

    $ make

Usage
-----
### receiver side: rtp:1234 -> /dev/pts/3
    $ ptsrecv
    (/dev/pts/3)
    $ rtprecv /dev/pts/3 1234

### sender side: /dev/pts/4 -> rtp:localhost:1234
    $ ptssend
    (/dev/pts/4)
    $ rtpsend /dev/pts/4 localhost 1234 --with-jitter 500

Bugs
----
Not tested at real S.Bus devices.

Air
