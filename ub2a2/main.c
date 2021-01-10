#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <wait.h>
#include "../WordCheck.c"

#define MAX_CONN 1

extern void serve(int fd);

void print_ip_addr(const struct sockaddr *sock_add);
static void handler_sigchld(int signum);

// Total Client count
int client_count = 0;

int main(int argc, char *argv[]) {
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int socket_fd, s;

  if (argc != 2) {
    exit(EXIT_FAILURE);
  }

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  s = getaddrinfo(NULL, argv[1], &hints, &result);
  if (s != 0) {
    exit(EXIT_FAILURE);
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
    socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
      exit(EXIT_FAILURE);
    }
    if (socket_fd == -1) {
      continue;
    }
    // bind the socket to a local port
    if (bind(socket_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      break;                  /* Success */
    }
    close(socket_fd);
  }

  // No address worked
  if (rp == NULL) {
    exit(EXIT_FAILURE);
  }
  if (listen(socket_fd, MAX_CONN) < 0) {
    exit(EXIT_FAILURE);
  }

  /* Wait for incoming connection requests */
  struct sigaction sa;
  sa.sa_handler = handler_sigchld;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  int running = 1;

  while (running) {
    int new_client = accept(socket_fd, rp->ai_addr, &rp->ai_addrlen);
    // Problem when accepting connections
    // Stop Server
    if (new_client < 0) {
      running = 0;
    } else {
      client_count++;
      int pid = fork();
      // If pid == 0, then this is the child
      if (pid == 0) {
        close(socket_fd);
        serve(new_client);
        close(new_client);
        exit(0); // Child is finished
      } else if (pid > 0) {
        printf("Client Count: %d\n", client_count); // Print the client count
        close(new_client);
      } else {
        // fork was not successful
        exit(EXIT_FAILURE);
      }
    }
  }
  close(socket_fd);
}

static void handler_sigchld(int signum) {
  int child_process_id, status;
  int saved_errno = errno;
  while ((child_process_id = waitpid(-1, &status , WNOHANG) > 0));
  errno = saved_errno;
  client_count--;
}

// Here for debugging
// Maybe delete
void print_ip_addr(const struct sockaddr *sock_add) {
  switch (sock_add->sa_family) {
    case AF_INET: {
      char str_addr[INET_ADDRSTRLEN];
      struct sockaddr_in *sa = (struct sockaddr_in *) sock_add;
      inet_ntop(AF_INET, &(sa->sin_addr), str_addr, INET_ADDRSTRLEN);
      printf("%s:%d\n", str_addr, ntohs(sa->sin_port));
      break;
    }
    case AF_INET6: {
      char str_addr[INET6_ADDRSTRLEN];
      struct sockaddr_in6 *sa = (struct sockaddr_in6 *) sock_add;
      inet_ntop(AF_INET6, &((sa)->sin6_addr), str_addr, INET6_ADDRSTRLEN);
      printf("%s:%d\n", str_addr, ntohs(sa->sin6_port));
      break;
    }
    default:
      fprintf(stderr, "Address Family not implemented\n");
  }
}
