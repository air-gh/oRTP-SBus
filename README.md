
oRTP-SBus: Futaba S.Bus over the Internet
=========================================

oRTP-SBus is Futaba S.Bus over RTP sender+receiver using oRTP library.
S.Bus emulator source+sink is also included for testing without any
S.Bus devices.

oRTP-SBus sender+receiver is based on oRTP samples at `src/tests`.
S.Bus emulator source+sink is based on pts version of tty0tty.

    S.Bus generator -> /dev/pts/a -> RTP -> /dev/pts/b -> discard
                ptssend      rtpsend   rtprecv      ptsrecv

oRTP library
------------

Before building oRTP-SBus, you should make oRTP library with
posixtimer_interval=1000, because S.Bus interval is 7ms.
posixtimer_interval is defined at `configure.ac` of oRTP.

oRTP on linphone-3.6.1 may be required because of inter-version
API incompatilibity.

Getting sources
---------------

    $ git clone https://github.com/air-gh/oRTP-SBus.git

Make
----

Set real oRTP library compiled by posixtimer_interval=1000 into Makefile.
And, just

    $ make

Usage example with S.Bus emulator
---------------------------------

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

Not tested with real S.Bus devices.
I'll try

    S.Bus receiver -> /dev/ttya -> RTP -> /dev/ttyb -> R/C controller
                            rtpsend   rtprecv

Air
