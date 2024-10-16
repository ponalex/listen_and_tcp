#include "src/portaudio.h"
#include "src/my_lib.h"
#include "src/pa_wrapper.h"
#include <bits/getopt_core.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define GETOPT_OPTIONS "L:i:cfbB:a:p:v:h"
#define LOG_FILENAME "log_file.log"
#define LOG_DEFAULT_LEVEL WARNING
#define LOG_DEFAULT_OUTPUT CONSOLE
#define CONNECT_BUFFER_LENGTH 1024
#define CONNECT_MESSAGE_LENGTH 128
#define CONNECT_DEFAULT_ADDRESS "127.0.0.1"
#define CONNECT_DEFAULT_PORT 10043

#define DELAY_USECONDS 500000
#define SOUND_NUM_CHANNELS 1
#define SOUND_PA_TYPE  ((PaSampleFormat) 0x00000008) //paFloat32
#define SOUND_BUFFER_SIZE 1024
#define SOUND_SAMPLE_RATE 44100
#define SOUND_VOLUME 0.005f

#define SKIP_N_FIRST 2

log_argument logging_arg;
struct connection connection_arg;
float sensivity =SOUND_VOLUME;
PaStream *sound_stream;
pthread_t log_thread;

void stop_log_thread(pthread_t l_thread){
    lib_destroy_mutex();
    pthread_cancel(l_thread);
    pthread_join(l_thread, NULL);
}

pthread_t send_thread;
/*  Options: "L:i:cfbB:a:p:"
 *           -L <filename>
 *           -i <number>
 *           -c or -f or -b where to output
 *           -B <number> number of bytes
 *           -a <address>
 *           -p <number> port number
 */

void get_message_time(char* text, size_t text_length){
    struct timespec current_time;
    timespec_get(&current_time, TIME_UTC);
    timespec_to_str(current_time, text, text_length);
}

void set_configuration(int argc, char** argv){
    logging_arg.filename = LOG_FILENAME; 
    logging_arg.level = LOG_DEFAULT_LEVEL;
    logging_arg.output = LOG_DEFAULT_OUTPUT;
    connection_arg.buffer_length = CONNECT_BUFFER_LENGTH;
    connection_arg.address = CONNECT_DEFAULT_ADDRESS;
    connection_arg.port = CONNECT_DEFAULT_PORT;
    int opt = 0;
    while((opt=getopt(argc, argv, GETOPT_OPTIONS)) != -1){
        switch (opt) {
            case 'L':
                logging_arg.filename = optarg;
                break;
            case 'i':
                logging_arg.level = -1*atoi(optarg);
                if (logging_arg.level > -1 || logging_arg.level < -5){
                    perror("Level of logging should be between 1(OK) and 5(CRITICAL).\n");
                    exit(-2);
                }
                break;
            case 'c':
                logging_arg.output = CONSOLE;
                break;
            case 'f':
                logging_arg.output = LOG_FILE;
                break;
            case 'b':
                logging_arg.output = BOTH;
                break;
            case 'B':
                connection_arg.buffer_length = atoi(optarg);
                break;
            case 'a':
                connection_arg.address = optarg;
                break;
            case 'p':
                connection_arg.port = (short)atoi(optarg);
                break;
            case 'v':
                sensivity = atof(optarg);
                break;
            case 'h':break;
            default: break;
        }
    }
}

int main(int argc, char** argv){
    set_configuration(argc, argv);
    // Init Logging
    log_message log_data= {.value = 0, .level=0, .message = "Ok."};
    logging_arg.message = &log_data;
    connection_arg.message = &log_data;
    // pthread_t   log_thread;
    lib_init_mutex();
    if(pthread_create(&log_thread,
                      NULL,
                      log_function, 
                      (void*)&logging_arg) != 0){
        perror("[CRITICAL] Couldn't create a thread for logging!");
        lib_destroy_mutex();
        return -1;
    }
    // Init PortAudio
    PaStreamParameters input_params;
    int status = pa_configuration(&input_params);
    if(status < 0){
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        logging(&log_data, (char*)Pa_GetErrorText(status), CRITICAL, status);
        stop_log_thread(log_thread);
        return status;
    }
    // Set configuration to Port Audio
    struct configuration sound_confid;
    sound_confid.flags = paClipOff;
    sound_confid.sample_rate = SOUND_SAMPLE_RATE;
    sound_confid.frames_per_buffer = SOUND_BUFFER_SIZE;
    float sound[SOUND_BUFFER_SIZE] = {0};
    sound_block sound_data ={.level = sensivity, .value = 0};
    status = create_stream(&sound_stream, &input_params, sound_confid, (void*)&sound_data);
    if ( status < 0){
        logging(&log_data, (char*)Pa_GetErrorText(status), CRITICAL, status);
        stop_log_thread(log_thread);
        return status;
    }
    // Init Sending to server
    status = pthread_create(&send_thread, NULL, send_function, (void*)&connection_arg);
    // Run do-while loop
    float sum = 0;
    char message_to_server[CONNECT_MESSAGE_LENGTH] = {0};
    uint counter = 0;
    do{
        if(sound_data.value == 0){
            continue;
        }else{
            if(counter < SKIP_N_FIRST){
                sum = 0;
                ++counter;
                continue;
            }
            if (sound_data.value > 0){
                logging(&log_data, "Send data.", INFORMATION, 0);
                get_message_time(message_to_server,CONNECT_MESSAGE_LENGTH);
                connection_arg.data = message_to_server;
                connection_arg.data_size =strlen(message_to_server)+1;
                if(pthread_create(&send_thread, NULL, send_function, (void*)&connection_arg)!=0){
                    logging(&log_data, "Couldn't send message to server.", WARNING, 0);
                }
                usleep(DELAY_USECONDS);
            }
            sound_data.value = 0;
        }
    }while(1);
    //  Check if pthread for sending messages is running
    //  Stop it
    //  Stop PortAddress
    //  Stop Logging

    printf("Filename: %s\n", logging_arg.filename);
    printf("Level of logging: %d\n", logging_arg.level);
    printf("Output to : %d\n", logging_arg.output);

    printf("%d\n",connection_arg.buffer_length);
    printf("Address: %s\n", connection_arg.address);
    printf("Port: %d\n", connection_arg.port);
    
    status = quit_stream(&sound_stream);
    if (status < 0){
        logging(&log_data, (char*)Pa_GetErrorText(status), WARNING, status);
        usleep(DELAY_USECONDS);
    }
    stop_log_thread(log_thread);

    return 0;
}
