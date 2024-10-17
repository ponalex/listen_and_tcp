#include "pa_wrapper.h"
#include "portaudio.h"
#include <time.h>
#include <alsa/asoundlib.h>

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
        return err;
    }
    
    in_params->device = Pa_GetDefaultInputDevice();
    if( in_params->device == paNoDevice){
        Pa_Terminate();
        return paInvalidDevice;
    }
	in_params->channelCount = in_params->channelCount;
	in_params->sampleFormat = in_params->sampleFormat;
	in_params->suggestedLatency = Pa_GetDeviceInfo( in_params->device )->defaultLowInputLatency;
	in_params->hostApiSpecificStreamInfo = NULL;
    
    return err;
}

int create_stream(  PaStream **stream,
                    PaStreamParameters *in_params,
                    struct configuration config,
                    void *data){
    PaError err;
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

int quit_stream(PaStream *stream){
    PaError err;

    err = Pa_StopStream(stream);
    if (err != paNoError) {
        Pa_Terminate();
        return err;
    }

    // Close the stream
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        Pa_Terminate();
        return err;
    }

    // Terminate PortAudio
    Pa_Terminate();

    return err;
}


int list_microphones() {
    int card = -1;
    int counter = 0;
    char *name;
    snd_ctl_t *handle;
    snd_ctl_card_info_t *info;
    snd_pcm_info_t *pcm_info;
    snd_pcm_info_alloca(&pcm_info);
    snd_ctl_card_info_alloca(&info);

    while (snd_card_next(&card) >= 0 && card >= 0) {
        char str[32];
        snprintf(str, sizeof(str), "hw:%d", card);
        if (snd_ctl_open(&handle, str, 0) < 0) {
            continue;
        }

        snd_ctl_card_info(handle, info);
        name = (char*)snd_ctl_card_info_get_name(info);

        for (int device = 0; device < 32; device++) {
            snd_pcm_info_set_device(pcm_info, device);
            snd_pcm_info_set_subdevice(pcm_info, 0);
            snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_CAPTURE);

            if (snd_ctl_pcm_info(handle, pcm_info) >= 0) {
                ++counter;
            }
        }

        snd_ctl_close(handle);
    }
    return counter;
}

