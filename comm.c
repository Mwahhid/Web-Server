/*
 * comm.c - communication helper routines
 * author: larkinsb@rhodes.edu
 *
 */

#include "httpserver.h"


/*
 * check_buf - checks to see if a buffer contains a full HTTP request
 *
 * @param buf buffer containing a possibly partial HTTP request
 * @param len size of the buffer
 * @returns 0 if the buffer has a full request header, 0 otherwise
 */
int check_buf(char *buf, int len) {
  // loop over all bytes in the buffer looking for a blank line
  for (int i=0; i<len-3; i++) {
    if ((buf[i] == '\r') &&
        (buf[i+1] == '\n') &&
        (buf[i+2] == '\r') &&
        (buf[i+3] == '\n')) {
      return 0;
    }
  }
  return 1;
}



/*
 * set_timeout - sets a timeout on a socket to handle keep-alive messages
 *               timeout length is set in TIMEOUT_SECS in httpserver.h
 *
 * @param fd socket to set the timeout on
 */
void set_timeout(int fd) {
  struct timeval tv;
  tv.tv_sec = TIMEOUT_SECS;
  tv.tv_usec = 0;
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
}



/*
 * set_reuse - marks a TCP listen socket as reusable to avoid hold-down timer
 *
 * @param fd socket to mark as re-usable
 */
void set_reuse(int fd) {
  int flag = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
}
