  /*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <ortp/ortp.h>
#include <signal.h>
#include <stdlib.h>

#ifndef _WIN32 
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#endif

#define SBUS
#ifdef SBUS
#include <ortp/port.h>
#include <ortp/payloadtype.h>
#include <time.h>
#include <termios.h>

#define SBUS_FRAME_SIZE 25
#define SBUS_FIND_RETRY 10
#define SBUS_HZ 10000   /* 10KHz = 100us */

PayloadType payload_type_sbus={
	PAYLOAD_OTHER, /*type */
	SBUS_HZ,
	0,
	NULL,
	0,
	0,
	"SBUS",
	0,
	0
};

ORTP_VAR_PUBLIC PayloadType payload_type_sbus;

struct timespec tsnew, tsold;
#endif /* SBUS */

int runcond=1;

void stophandler(int signum)
{
	runcond=0;
}

static const char *help="usage: rtpsend	filename dest_ip4addr dest_port [ --with-clockslide <value> ] [ --with-jitter <milliseconds>]\n";

#ifdef SBUS /* original source */
/* https://gist.github.com/diabloneo/9619917 */
void timespec_diff(const struct timespec *start, const struct timespec *stop,
                   struct timespec *result)
{
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
	} else {
		result->tv_sec = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}

	return;
}

uint32_t clock_diff(const struct timespec *start, const struct timespec *stop, int hz)
{
	struct timespec result;

	timespec_diff(start, stop, &result);
	return (result.tv_sec * 1000 * 1000 * 1000 + result.tv_nsec) / (1000 * 1000 * 1000 / hz);
}

unsigned char sbus_flush_buf(FILE *fp)
{
	unsigned char buf[1] = { 0 };

	printf("flush tty buffer...");
	tcflush(fileno(fp), TCIFLUSH);

	clock_gettime(CLOCK_REALTIME, &tsold);
	while (fread(buf, 1, 1, fp) > 0 && runcond) {
		clock_gettime(CLOCK_REALTIME, &tsnew);
		if (clock_diff(&tsold, &tsnew, SBUS_HZ) > 50) {   /* 5ms */
			break;
		}
		tsold = tsnew;
	}
	printf("done.\n");
	return buf[0];
}

int sbus_find_frame(FILE *fp, int retry)
{
	unsigned char buf[SBUS_FRAME_SIZE * 2];
	int i, j;

	for (i = 0; i < retry; i++) {
		buf[0] = sbus_flush_buf(fp);
		printf("search SBus frame...");
		if (fread(buf + 1, 1, SBUS_FRAME_SIZE * 2 - 1, fp) < SBUS_FRAME_SIZE * 2 - 1 || !runcond) {
			return 0;   /* error */
		}
		clock_gettime(CLOCK_REALTIME, &tsold);
		for (j = 0; j < SBUS_FRAME_SIZE; j++) {
			if (buf[j] == 0x0f && buf[j + SBUS_FRAME_SIZE - 1] == 0x00) {
				/* found */
				printf("found.  shift = %d\n", j);
				if (j != 0) {
					if (fread(buf, 1, j, fp) < j || !runcond) {   /* skip remain data */
						return 0;   /* error */
					}
				}
				return 1;
			}
		}
		putchar('\n');
	}
	printf("reached %d retry, not found.\n", i);
	return 0;   /* not found */
}
#endif /* SBUS */

int main(int argc, char *argv[])
{
	RtpSession *session;
#ifndef SBUS
	unsigned char buffer[160];
#else
	unsigned char buffer[SBUS_FRAME_SIZE];
#endif
	int i;
	FILE *infile;
	char *ssrc;
	uint32_t user_ts=0;
	int clockslide=0;
	int jitter=0;
#ifdef SBUS
	RtpProfile prof;
#endif
	if (argc<4){
		printf("%s", help);
		return -1;
	}
	for(i=4;i<argc;i++){
		if (strcmp(argv[i],"--with-clockslide")==0){
			i++;
			if (i>=argc) {
				printf("%s", help);
				return -1;
			}
			clockslide=atoi(argv[i]);
			ortp_message("Using clockslide of %i milisecond every 50 packets.",clockslide);
		}else if (strcmp(argv[i],"--with-jitter")==0){
			ortp_message("Jitter will be added to outgoing stream.");
			i++;
			if (i>=argc) {
				printf("%s", help);
				return -1;
			}
			jitter=atoi(argv[i]);
		}
	}
	
	ortp_init();
	ortp_scheduler_init();
	ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR);
	session=rtp_session_new(RTP_SESSION_SENDONLY);	
	
	rtp_session_set_scheduling_mode(session,1);
	rtp_session_set_blocking_mode(session,1);
	rtp_session_set_connected_mode(session,TRUE);
	rtp_session_set_remote_addr(session,argv[2],atoi(argv[3]));
#ifndef SBUS
	rtp_session_set_payload_type(session,0);
#else
	rtp_profile_clear_all(&prof);
	//rtp_profile_set_name(&prof, "SBUS");
	rtp_profile_set_payload(&prof, 71, &payload_type_sbus);
	rtp_session_set_profile(session, &prof);
	rtp_session_set_payload_type(session, 71);
#endif

	ssrc=getenv("SSRC");
	if (ssrc!=NULL) {
		printf("using SSRC=%i.\n",atoi(ssrc));
		rtp_session_set_ssrc(session,atoi(ssrc));
	}
		
	#ifndef _WIN32
	infile=fopen(argv[1],"r");
	#else
	infile=fopen(argv[1],"rb");
	#endif

	if (infile==NULL) {
		perror("Cannot open file");
		return -1;
	}

	signal(SIGINT,stophandler);
#ifdef SBUS
	setvbuf(infile, NULL, _IONBF, 0);
	if (!sbus_find_frame(infile, SBUS_FIND_RETRY)) {
		return -1;   /* error */
	}
#endif

#ifndef SBUS
	while( ((i=fread(buffer,1,160,infile))>0) && (runcond) )
#else
	while( ((i=fread(buffer,1,SBUS_FRAME_SIZE,infile))>0) && (runcond) )
#endif
	{
#ifdef SBUS
		clock_gettime(CLOCK_REALTIME, &tsnew);
		if (buffer[0] != 0x0f || buffer[SBUS_FRAME_SIZE - 1] != 0x00) {
			printf("out of sync.\n");
			if (sbus_find_frame(infile, SBUS_FIND_RETRY)) {
				continue;   /* re-synced */
			} else {
				return -1;   /* error */
			}
		}
#endif
		rtp_session_send_with_ts(session,buffer,i,user_ts);
#ifndef SBUS
		user_ts+=160;
#else
		user_ts += clock_diff(&tsold, &tsnew, SBUS_HZ);   /* 10KHz timestamp */
		tsold = tsnew;
#endif

		if (clockslide!=0 && user_ts%(160*50)==0){
			ortp_message("Clock sliding of %i miliseconds now",clockslide);
			rtp_session_make_time_distorsion(session,clockslide);
		}
		/*this will simulate a burst of late packets */
		if (jitter && (user_ts%(8000)==0)) {
			struct timespec pausetime, remtime;
			ortp_message("Simulating late packets now (%i milliseconds)",jitter);
			pausetime.tv_sec=jitter/1000;
			pausetime.tv_nsec=(jitter%1000)*1000000;
			while(nanosleep(&pausetime,&remtime)==-1 && errno==EINTR){
				pausetime=remtime;
			}
		}
	}

	fclose(infile);
	rtp_session_destroy(session);
	ortp_exit();
	ortp_global_stats_display();

	return 0;
}
