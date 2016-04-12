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

#include <time.h>

static char buffer[1024];

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
dump(char *p, ssize_t s)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	printf("%09ld: ", ts.tv_nsec);
	for (; s > 0; s--)
		printf("%02x ", *p++);
	putchar('\n');
}

void
copydata(int fdfrom)
{
  ssize_t br;
  br = read(fdfrom, buffer, 1024);
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
    dump(buffer, br);
  }
}

int main(int argc, char* argv[])
{
  char master1[1024];
  char slave1[1024];

  int fd1, slave;

  fd_set rfds;
  int retval;

  fd1=ptym_open(master1,slave1,1024);
  printf("(%s)\n",slave1);

  if ((slave = open(slave1, 0)) < 0)
    perror("open slave");

  conf_ser(fd1);

  while(1)
  {
    FD_ZERO(&rfds);
    FD_SET(fd1, &rfds);

    retval = select(fd1 + 1, &rfds, NULL, NULL, NULL);
    if (retval == -1)
    {
      perror("select");
      return 1;
    }
    if (FD_ISSET(fd1, &rfds))
    {
      copydata(fd1);
    }
  }

  close(fd1);
  close(slave);

  return EXIT_SUCCESS;
}
