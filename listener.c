#include "src/portaudio.h"
#include "src/my_lib.h"
#include "src/pa_wrapper.h"
#include <bits/getopt_core.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define GETOPT_OPTIONS "L:i:cfbB:a:p:v:h"
#define LOG_FILENAME "log_file.log"
#define LOG_DEFAULT_LEVEL WARNING
#define LOG_DEFAULT_OUTPUT BOTH
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

//  The global variable to configure  a programm
log_argument logging_arg;
struct connection connection_arg;
float sensivity =SOUND_VOLUME;
PaStream *sound_stream;
pthread_t log_thread;
pthread_t send_thread;

//  Declare this funtion to exit on pressing CTR-C
void stop_and_leave(){
    exit (0);
}

//  Declare one by one function to stop processes according
//  to their declaration but in reverse order
//  Stop logging
void stop_log_thread(){
    printf("Stop logging!\n");
    stop_logging();
    pthread_join(log_thread, NULL);
}
//  Stop listening the volume
void stop_pa_thread(){
    printf("Stop pa_thread!\n");
    quit_stream(sound_stream);
}


/*  Options: "L:i:cfbB:a:p:"
 *           -L <filename>
 *           -i <number>
 *           -c or -f or -b where to output
 *           -B <number> number of bytes
 *           -a <address>
 *           -p <number> port number
 */

//  Set configuration using function optarg
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
    //  Declare signal for exit
    signal(SIGINT, stop_and_leave);
    //  Checking microphone
    if(list_microphones() == 0 ){
        perror("There is no any microphone!\n");
        return -1;
    }
    // Init variable for logging
    log_message log_data= {.value = 0, .level=0, .message = "Ok."};
    logging_arg.message = &log_data;
    connection_arg.message = &log_data;
    // pthread_t   log_thread;
    lib_init_mutex();
    // Launch the thread which loggs 
    if(pthread_create(&log_thread,
                      NULL,
                      log_function,
                      (void*)&logging_arg) != 0){
        perror("[CRITICAL] Couldn't create a thread for logging!");
        lib_destroy_mutex();
        exit (-1);
    }

    atexit(stop_log_thread);
    // Init PortAudio
    //  This should be placed in the separate file
    PaStreamParameters input_params;
    input_params.channelCount = SOUND_NUM_CHANNELS;
    input_params.sampleFormat = SOUND_PA_TYPE;
    //  Initialization port audio
    int status = pa_configuration(&input_params);
    if(status < 0){
        logging(&log_data, (char*)Pa_GetErrorText(status), CRITICAL, status);
        exit(status);
    }
    // Set configuration device to Port Audio
    struct configuration sound_confid;
    sound_confid.flags = paClipOff;
    sound_confid.sample_rate = SOUND_SAMPLE_RATE;
    sound_confid.frames_per_buffer = SOUND_BUFFER_SIZE;
    sound_block sound_data ={.level = sensivity, .value = 0};
    status = create_stream(&sound_stream, &input_params, sound_confid, (void*)&sound_data);
    if ( status < 0){
        logging(&log_data, (char*)Pa_GetErrorText(status), CRITICAL, status);
        exit (status);
    }
    atexit(stop_pa_thread);               //  Stop reading microphone
    // Init Sending to server
    // status = pthread_create(&send_thread, NULL, send_function, (void*)&connection_arg);
    // Run do-while loop
    char message_to_server[CONNECT_MESSAGE_LENGTH] = {0};
    // It is used to skip 2 first buffer due to it has a big level of noise
    uint counter = 0;
    do{
        if(sound_data.value == 0){
            continue;
        }else{
            if(counter < SKIP_N_FIRST){
                ++counter;
                continue;
            }
            if (sound_data.value > 0){
//                logging(&log_data, "Send data.", INFORMATION, 0);
                //  Get time to send to the server
                get_message_time(message_to_server,CONNECT_MESSAGE_LENGTH);
                //  Push text to the block which will be sent to the server
                connection_arg.data = message_to_server;
                connection_arg.data_size =strlen(message_to_server)+1;
                //  Sending to server/ If error occurs the will be noticed in the logger
                if(pthread_create(&send_thread, NULL, send_function, (void*)&connection_arg)!=0){
                    logging(&log_data, "Couldn't send message to server.", WARNING, 0);
                }
                //  Sleep to avoid multiply sending
                usleep(DELAY_USECONDS);
            }
            sound_data.value = 0;
        }
    }while(1);
    
    status = quit_stream(&sound_stream);
    if (status < 0){
        logging(&log_data, (char*)Pa_GetErrorText(status), WARNING, status);
        usleep(DELAY_USECONDS);
    }
    stop_log_thread();

    return 0;
}
