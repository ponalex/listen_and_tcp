#include "my_lib.h"
#include <bits/types/struct_iovec.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

pthread_mutex_t mutex;

void timespec_to_str(struct timespec tmspc, char *buffer, size_t buffer_size){
    time_t seconds = tmspc.tv_sec;
    // Convert nanoseconds to milliseconds
    unsigned int m_seconds = tmspc.tv_nsec / 1000000;
    struct tm *ptr_tm  = localtime(&seconds);
    size_t time_size = strftime(buffer, buffer_size, "%e-%b-%Y %H:%M:%S", ptr_tm);
    snprintf(buffer+strlen(buffer), buffer_size-strlen(buffer), ".%03u", m_seconds); 
}

void lib_init_mutex(){
    pthread_mutex_init(&mutex, NULL);
}

void lib_destroy_mutex(){
    pthread_mutex_destroy(&mutex); 
}

void log_to_file(char* filename,
                enum log_level level,
                log_message* message){ 
    if(message->level > level){
        return;
    }
    if(strlen(filename)==0){
        perror("[ERROR] There is no a filename!\n");
        return;
    }
    FILE* log_file = fopen(filename,"a");
    if(log_file == NULL){
        perror("[ERROR] Could not open the file.!\n");
        return;
    }
    int result = 0;
    switch (message->level){
        case CRITICAL: 
            fprintf(log_file, "[CRITICAL]%s, Error: %d\n", message->message, message->value);
            break;
        case ERROR:
            fprintf(log_file, "[ERROR]%s, : %d\n", message->message, message->value);
            break;
        case WARNING:
            fprintf(log_file, "[WARNING]%s, : %d\n", message->message, message->value);
            break;
        case INFORMATION:
            fprintf(log_file, "[INFO]%s \n", message->message);
            break;
        case OK: 
            fprintf(log_file, "[OK]%s \n", message->message);
            break;
        default:
            fprintf(log_file, "[UNKNOWN ERROR] Something went wrong!\n");
            break;
    }
    fclose(log_file);
    result = message->level;
    message->value = 0;
    message->value =0;
}

void log_to_console(enum log_level level,
                    log_message* message){
    if(message->level > level){
        message->level = 0;
        message->value = 0;
        return;
    }
    int result = 0;
    switch (message->level){
        case CRITICAL:
            printf("[CRITICAL]%s, Error: %d\n", message->message, message->value);
            break;
        case ERROR:
            printf("[ERROR]%s, Error: %d\n", message->message, message->value);
            break;
        case WARNING:
            printf("[WARNING]%s: %d\n", message->message, message->value);
            break;
        case INFORMATION:
            printf("[INFO]%s \n", message->message);
            break;
        case OK: 
            printf("[OK]%s \n", message->message);
            break;
        default:
            printf("[UNKNOWN ERROR] Something went wrong!\n");
            break;
    }
    result = message->level;
    message->level = 0;
    message->value = 0;
}

void* send_function(void* data){
    struct connection *config= (struct connection* )data;
    
    log_message* log_mess = config->message;
    int sock;
    struct sockaddr_in server_address;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    char* message = (char*) config->data;
    size_t message_size = config->data_size;
    // Creating socket
    if(sock < 0){
        logging(log_mess, "Couldn't open the socket.", WARNING, sock);
        return NULL;
    }
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(config->port);
    // String address to binary
    if(inet_pton(AF_INET, config->address, &server_address.sin_addr) <= 0){
        logging(log_mess, "Wrong address of server.", WARNING, -1);
        close(sock);
        return NULL;
    }
    // Connection to the server 
    int res = connect(sock, (struct sockaddr*)&server_address, sizeof(server_address));
    if(res < 0){
        logging(log_mess, strerror(errno), WARNING, errno);
        close(sock);
        return NULL;
    }
    // Sending message to the server
    if(send(sock, message, message_size, MSG_DONTROUTE)<0){
        logging(log_mess, "Couldn't deliver a message.", WARNING, -3);
        close(sock);
        return NULL;
    }
    close(sock);
    logging(log_mess, message, INFORMATION, 0);
    return 0;
}


void* log_function(void *data){
    log_argument* arguments = (log_argument* )data;
    log_message* message = (log_message*)calloc(1, sizeof(log_message));
    if( message == NULL){
        perror("[CRITICAL]Couldn't allocate memory for storing \'log_message\'");
        return NULL;
    }
    int status = 0;
    while(status > ERROR){
        pthread_mutex_lock(&mutex);
        if(message->level == 0 || message->level < arguments->message->level){
            memcpy(message, arguments->message, sizeof(log_message));
            arguments->message->level = 0;
            arguments->message->value = 0;
        }
        pthread_mutex_unlock(&mutex);
        status = message->level;
        switch(arguments->output){
            case LOG_FILE:
                log_to_file(arguments->filename, arguments->level, message);
                break;
            case CONSOLE:
                log_to_console(arguments->level, message);
                break;
            default:
                log_to_console(arguments->level, message);
                log_to_file(arguments->filename, arguments->level, message);
                break;
        }       
    }
    free(message);
    printf("Stop logging.\n");
    return NULL;
}

void logging(log_message* message, char* text, enum log_level level, int data){
    pthread_mutex_lock(&mutex);
    if(level < message->level){
        message->message = text;
        message->value = data;
        message->level = level;
    }
    pthread_mutex_unlock(&mutex);
}
