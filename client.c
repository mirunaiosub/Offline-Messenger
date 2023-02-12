#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main(int argc, char *argv[])
{
  int sd;                    // descriptorul de socket
  struct sockaddr_in server; // structura folosita pentru conectare
  char msg[1000];           // mesajul

  if (argc != 3)
  {
    printf("Syntax: %s <server_address> <port>\n", argv[0]);
    return -1;
  }

  /* stabilim portul */
  port = atoi(argv[2]);

  /* cream socketul */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Socket ERROR.\n");
    return errno;
  }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons(port);

  /* ne conectam la server */
  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[client]Connect ERROR.\n");
    return errno;
  }

  while (1)
  {
    /* citirea mesajului */
    printf("[client]Enter your command: ");
    fflush(stdout);
    memset(msg, 0, sizeof(msg));
    read(0, msg, sizeof(msg));
    msg[strlen(msg) - 1] = 0;

    /* trimiterea mesajului la server */
    if (write(sd, msg, sizeof(msg)) <= 0)
    {
      perror("[client]WRITE ERROR to server.\n");
      return errno;
    }

    if (strncmp(msg, "quit", 4) == 0)
      return 0;
    /* apel blocant pana cand serverul raspunde */

    if (read(sd, msg, sizeof(msg)) < 0)
    {
      perror("[client]READ ERROR from server.\n");
      return errno;
    }

    /* afisam mesajul primit */
    printf("[client]%s\n", msg);
  }

/* inchidem conexiunea, am terminat */
close(sd);
}