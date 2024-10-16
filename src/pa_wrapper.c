#include "pa_wrapper.h"
#include "portaudio.h"
#include <time.h>

#define NORMALISATION 32768.0f

static int audio_callback(
            const void *input_buffer, 
            void *output_buffer,
            unsigned long frames_per_buffer,
            const PaStreamCallbackTimeInfo *timeInfo,
            PaStreamCallbackFlags statusFlags,
            void *user_data         
        ){
    short *in = (short*)input_buffer;
    sound_block *data = (sound_block *) user_data;
    unsigned long int counter = 0;
    float sum = 0;
    float temp = 0;
    for( ;  counter < frames_per_buffer ;
            ++counter ){
        temp = *in++/NORMALISATION;
        sum = temp*temp;
    }
    sum = sqrt(sum/(float)frames_per_buffer);
    if(data->value == 0 ){
        if ( sum > data->level){
            data->value = 1;
        }else{
            data->value =-1;
        }
    }

    return paContinue;
}

void quit_port_audio(char* message, PaError err){
   Pa_Terminate();
   fprintf(stderr, "%s\n", message);
   fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
}

int pa_configuration(PaStreamParameters *in_params){
    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return err;
    }
    
    in_params->device = Pa_GetDefaultInputDevice();
    if( in_params->device == paNoDevice){
        quit_port_audio("[Cannot open a default input device]\n", paInvalidDevice);
        return -1;
    }
	in_params->channelCount = NUM_CHANNELS;
	in_params->sampleFormat = PA_SAMPLE_TYPE;
	in_params->suggestedLatency = Pa_GetDeviceInfo( in_params->device )->defaultLowInputLatency;
	in_params->hostApiSpecificStreamInfo = NULL;
    
    return err;
}

int create_stream(  PaStream **stream,
                    PaStreamParameters *in_params,
                    struct configuration config,
                    void *data){
    PaError err;
    sound_block *block = (sound_block*)data;
    err = Pa_OpenStream (stream,
                        in_params,                     // input channels
                        NULL,          // output channels
                        config.sample_rate,
                        config.frames_per_buffer,
                        config.flags,
                        audio_callback,
                        data);
    if (err != paNoError) {
        Pa_Terminate();
        return err;
    }

    err = Pa_StartStream(*stream);
    if (err != paNoError) {
        Pa_CloseStream(*stream);
        Pa_Terminate();
        return err;
    }
    return err;
}

int quit_stream(PaStream **stream){
    PaError err;

    err = Pa_StopStream(*stream);
    if (err != paNoError) {
        Pa_Terminate();
        return err;
    }

    // Close the stream
    err = Pa_CloseStream(*stream);
    if (err != paNoError) {
        Pa_Terminate();
        return err;
    }

    // Terminate PortAudio
    Pa_Terminate();

    return err;
}
