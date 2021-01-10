#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../comm.c"

#define BUF_SIZE 500
#define IN 0
#define OUT 1

extern void comm(int tfd, int nfd);

int main(int argc, char *argv[]) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int sfd, s, j;
  ssize_t bytes_read;
  ssize_t bytes_written;
  char buf[BUF_SIZE];

  if (argc < 2) {
    fprintf(stderr, "Usage: %s host port...\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* Obtain address(es) matching host/port */

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;          /* Any protocol */

  s = getaddrinfo(argv[1], argv[2], &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype,
                 rp->ai_protocol);
    if (sfd == -1) {
      continue;
    }
    if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
      break;
    }
    close(sfd);
  }

  if (rp == NULL) {
    fprintf(stderr, "Could not connect\n");
    exit(EXIT_FAILURE);
  }

  comm(IN, sfd);
  close(sfd);
  exit(EXIT_SUCCESS);
}
