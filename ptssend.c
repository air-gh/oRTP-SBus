/* ########################################################################

   tty0tty - linux null modem emulator 

   ########################################################################

   Copyright (c) : 2013  Luis Claudio Gambôa Lopes and Maximiliano Pin max.pin@bitroit.com

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   For e-mail suggestions :  lcgamboa@yahoo.com
   ######################################################################## */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>

#include <termio.h>

#include <signal.h>
#include <time.h>

static char buffer[1024] = {
	0x0f, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x00, 0x00 };

int fd1, slave;

int
ptym_open(char *pts_name, char *pts_name_s , int pts_namesz)
{
    char    *ptr;
    int     fdm;

    strncpy(pts_name, "/dev/ptmx", pts_namesz);
    pts_name[pts_namesz - 1] = '\0';

    fdm = posix_openpt(O_RDWR | O_NONBLOCK);
    if (fdm < 0)
        return(-1);
    if (grantpt(fdm) < 0) 
    {
        close(fdm);
        return(-2);
    }
    if (unlockpt(fdm) < 0) 
    {
        close(fdm);
        return(-3);
    }
    if ((ptr = ptsname(fdm)) == NULL) 
    {
        close(fdm);
        return(-4);
    }
    
    strncpy(pts_name_s, ptr, pts_namesz);
    pts_name[pts_namesz - 1] = '\0';

    return(fdm);        
}


int
conf_ser(int serialDev)
{

int rc;
struct termios params;

// Get terminal atributes
rc = tcgetattr(serialDev, &params);

// Modify terminal attributes
cfmakeraw(&params);

rc = cfsetispeed(&params, B9600);

rc = cfsetospeed(&params, B9600);

// CREAD - Enable port to read data
// CLOCAL - Ignore modem control lines
params.c_cflag |= (B9600 |CS8 | CLOCAL | CREAD);

// Make Read Blocking
//fcntl(serialDev, F_SETFL, 0);

// Set serial attributes
rc = tcsetattr(serialDev, TCSANOW, &params);

// Flush serial device of both non-transmitted
// output data and non-read input data....
tcflush(serialDev, TCIOFLUSH);


  return EXIT_SUCCESS;
}

void
copydata(int fdto)
{
  ssize_t br, bw;
  char *pbuf = buffer;
  br = 25;
  if (br < 0)
  {
    if (errno == EAGAIN || errno == EIO)
    {
      br = 0;
    }
    else
    {
      perror("read");
      exit(1);
    }
  }
  if (br > 0)
  {
    do
    {
      do
      {
        bw = write(fdto, pbuf, br);
        if (bw > 0)
        {
          pbuf += bw;
          br -= bw;
        }
      } while (br > 0 && bw > 0);
    } while (bw < 0 && errno == EAGAIN);
    if (bw <= 0)
    {
      // kernel buffer may be full, but we can recover
      fprintf(stderr, "Write error, br=%zu bw=%zu\n", br, bw);
      usleep(500000);
    }
  }
  else
  {
    usleep(100000);
  }
}

void handler(int signo)
{
  copydata(fd1);
}

int main(int argc, char* argv[])
{
  char master1[1024];
  char slave1[1024];

  struct sigaction sigact;
  timer_t timer;
  struct itimerspec itval;

  fd1=ptym_open(master1,slave1,1024);
  printf("(%s)\n",slave1);

  if ((slave = open(slave1, 0)) < 0)
    perror("open slave");

  conf_ser(fd1);

  sigact.sa_handler = handler;
  sigact.sa_flags = 0;
  sigemptyset(&sigact.sa_mask);

  if (sigaction(SIGALRM, &sigact, NULL) < 0)
	  perror("sigaction");

  itval.it_interval.tv_sec  = 0;
  itval.it_interval.tv_nsec = 7 * 1000 * 1000;
  itval.it_value.tv_sec  = 0;
  itval.it_value.tv_nsec = 7 * 1000 * 1000;

  if (timer_create(CLOCK_REALTIME, NULL, &timer) < 0)
    perror("timer_create");
  if (timer_settime(timer, 0, &itval, NULL) < 0)
    perror("timer_settime");

  while(1)
  {
    // usleep(7 * 1000);
    // copydata(fd1);
    pause();
  }

  close(fd1);
  close(slave);

  return EXIT_SUCCESS;
}
