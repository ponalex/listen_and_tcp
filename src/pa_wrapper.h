#include "portaudio.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define BATCH_SIZE 1024 
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1
#define PA_SAMPLE_TYPE paInt16

struct configuration{
    double sample_rate;
    uint frames_per_buffer;
    PaStreamFlags flags;
};

typedef struct sound_block{    
    float level;
    int value;
}sound_block ;

void quit_port_audio(char* message, PaError err);

int pa_configuration(PaStreamParameters *in_params);

int create_stream(PaStream** stream,
        PaStreamParameters* in_params, 
        struct configuration config,
        void* data);

int quit_stream(PaStream** stream);

