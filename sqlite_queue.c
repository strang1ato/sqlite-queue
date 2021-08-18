#include <pthread.h>
#include <signal.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

sqlite3 *db;

int socket_fd;

pthread_t handlers[40];

char *messages[100];  /* 100 is basically size of queue referneced in program name */

pthread_mutex_t lock;

void *execute_queries(void *);
void *handle_connection(void *);

__attribute__((destructor)) void destroy(void)
{
  sqlite3_close(db);
  pthread_mutex_destroy(&lock);
  close(socket_fd);
  unlink("/tmp/sqlite-queue.socket");
}

int main(int argc, char *argv[])
{
  void sigint_hack(int signum) {
    exit(EXIT_SUCCESS);
  }
  signal(SIGINT, sigint_hack);

  if (argc != 2) {
    dprintf(2, "Incorrect amount of arguments\n");
    exit(EXIT_FAILURE);
  }
  if (sqlite3_open(argv[1], &db) != SQLITE_OK) {
    dprintf(2, "sqlite3_open failed\n");
    exit(EXIT_FAILURE);
  }

  socket_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (socket_fd == -1) {
    dprintf(2, "socket failed\n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_un socket_address;
  socket_address.sun_family = AF_UNIX;
  strcpy(socket_address.sun_path, "/tmp/sqlite-queue.socket");

  if (bind(socket_fd, (const struct sockaddr *) &socket_address, sizeof(socket_address)) == -1) {
    dprintf(2, "bind failed\n");
    exit(EXIT_FAILURE);
  }

  if (listen(socket_fd, 40) == -1) {
    dprintf(2, "listen failed\n");
    exit(EXIT_FAILURE);
  }

  if (pthread_mutex_init(&lock, NULL) != 0) {
    dprintf(2, "pthread_mutex_init failed\n");
    exit(EXIT_FAILURE);
  }

  pthread_t executor;
  if (pthread_create(&executor, NULL, &execute_queries, NULL)) {
    dprintf(2, "pthread_create failed\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    int connected_socket_fd = accept(socket_fd, NULL, NULL);
    for (int i = 0; i < 40; i++) {
      if (!handlers[i]) {
        int *socket_fd_and_handler_index = malloc(2 * sizeof(int));
        socket_fd_and_handler_index[0] = connected_socket_fd;
        socket_fd_and_handler_index[1] = i;
        if (pthread_create(&handlers[i], NULL, &handle_connection, (void *)socket_fd_and_handler_index)) {
          dprintf(2, "pthread_create failed\n");
          exit(EXIT_FAILURE);
        }
        break;
      }
    }
  }
}

void *execute_queries(void *arg)
{
  while (1) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < 100; i++) {
      if (messages[i]) {
#ifdef DEBUG
        char *errmsg;
        if (sqlite3_exec(db, messages[i], NULL, NULL, &errmsg) != SQLITE_OK) {
          dprintf(2, "sqlite error: %s\n", errmsg);
        }
#else
        sqlite3_exec(db, messages[i], NULL, NULL, NULL);
#endif

        free(messages[i]);
        messages[i] = 0;
      }
    }
    pthread_mutex_unlock(&lock);
    sleep(1);
  }
  return NULL;
}

void *handle_connection(void *arg)
{
  int *socket_fd_and_handler_index = (int *)arg;
  while (1) {
    char query_len_str[10];
    int ret = read(socket_fd_and_handler_index[0], query_len_str, 10);
    if (ret == -1) {
      dprintf(2, "read failed\n");
      exit(EXIT_FAILURE);
    }

    int end = 0;
    for (int i = 0; i < 10; i++) {
      if (query_len_str[i] == 'e' || end) {
        query_len_str[i] = 0;
        end = 1;
      }
    }

    int query_len = atoi(query_len_str);
    if (!query_len) {
      dprintf(2, "atoi failed\n");
      exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&lock);
    int i;
    for (i = 0; i < 100; i++) {
      if (!messages[i]) {
        messages[i] = calloc(query_len, 1);
        if (read(socket_fd_and_handler_index[0], messages[i], query_len) == -1) {
          dprintf(2, "read failed\n");
          exit(EXIT_FAILURE);
        }
        break;
      }
    }

    if (!strcmp(messages[i], "exit")) {
      free(messages[i]);
      messages[i] = 0;
      pthread_mutex_unlock(&lock);
      break;
    }
    pthread_mutex_unlock(&lock);
  }
  close(socket_fd_and_handler_index[0]);
  handlers[socket_fd_and_handler_index[1]] = 0;

  free(socket_fd_and_handler_index);
  return NULL;
}
