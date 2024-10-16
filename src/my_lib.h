#include <bits/types/struct_iovec.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>


enum log_level{
    CRITICAL = -5,
    ERROR = -4,
    WARNING = -3,
    INFORMATION = -2,
    OK = -1
};

enum destination{
    LOG_FILE,
    CONSOLE,
    BOTH
};

typedef struct{
    int value;
    enum log_level level;
    char* message;
}log_message;

typedef struct{
    char* filename;
    enum log_level level;
    enum destination output;
    log_message* message;
}log_argument;

struct connection{
    char* address;
    short port;
    unsigned int buffer_length;
    void *data;
    size_t data_size;
    log_message* message;
};


void* log_function(void *data);
void* send_function(void* data);
void logging(log_message* message, char* text, enum log_level level, int data);
void timespec_to_str(struct timespec tm, char* buffer, size_t buffer_size);

void lib_init_mutex();
void lib_destroy_mutex();
