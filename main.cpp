#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <ps3000aApi.h>

#define MHZ(x) ((x)*1000000.0)
#define SAMPLE_FREQUENCY MHZ(62.5)
#define SAMPLE_INTERVAL_PS (int)(((1000000000000.0/SAMPLE_FREQUENCY)+0.5))

#define CHANNELS 1

static volatile bool g_terminate;

void samples_ready(
    int16_t  handle,
    int32_t  noOfSamples,
    uint32_t startIndex,
    int16_t  overflow,
    uint32_t triggerAt,
    int16_t  triggered,
    int16_t  autoStop,
    void     *pParameter
)
{
    printf("idx=%08i overflow=%i samples=%08i\n",
        startIndex, overflow, noOfSamples);
}

void shutdown_request(int sig){
    g_terminate = true;
}

int main(int argc, char *argv[])
{
    PICO_STATUS status;
    int i, max_samples, block_us;
    uint32_t sample_interval;
    int16_t hscope;
    int16_t *buffer;

    /* hook termination handler */
    signal(SIGINT, shutdown_request);

    /* open first available scope */
    if((status = ps3000aOpenUnit(&hscope, NULL)) != PICO_OK)
    {
        printf("Picoscope device open failed [0x%x]\n",(uint32_t)status);
        return 1;
    }

    /* configure channel A */
    if((status = ps3000aSetChannel(
        hscope,
        PS3000A_CHANNEL_A,
        true,
        PS3000A_DC,
        PS3000A_5V,
        0)) != PICO_OK)
    {
        printf("failed to configure channel [0x%X]\n",(uint32_t)status);
        return 2;
    }

    /* disable channel B */
    if((status = ps3000aSetChannel(
        hscope,
        PS3000A_CHANNEL_B,
        CHANNELS>=2,
        PS3000A_DC,
        PS3000A_5V,
        0)) != PICO_OK)
    {
        printf("failed to configure channel [0x%x]\n",(uint32_t)status);
        return 3;
    }

    /* enable dual-buffering */
    if((status = ps3000aMemorySegments(hscope, CHANNELS, &max_samples)) != PICO_OK)
        printf("Failed to set memory segment count [0x%X]\n",(uint32_t)status);
    else
    {
        printf("allocating %i samples for buffer...\n", max_samples);

        /* allocate buffer memories */
        buffer = (int16_t*)
            malloc(max_samples * CHANNELS * sizeof(buffer[0]));
        if(!buffer)
            printf("ERROR: failed to allocate memory!\n");
        else
        {
            /* assign driver buffers to buffer memories */
            for(i=0; i<CHANNELS; i++)
                if((status = ps3000aSetDataBuffer(
                    hscope,
                    PS3000A_CHANNEL_A,
                    &buffer[i * max_samples],
                    max_samples,
                    i,
                    PS3000A_RATIO_MODE_NONE)) != PICO_OK)
                    break;
            if(i!=CHANNELS)
                printf("ERROR: registering channel %i (0x%X)!\n", i+1, status);
            else
            {
                /* run data aquisition */
                sample_interval = SAMPLE_INTERVAL_PS;
                if((status = ps3000aRunStreaming(
                    hscope,
                    &sample_interval,
                    PS3000A_PS,
                    0,
                    max_samples,
                    false,
                    1,
                    PS3000A_RATIO_MODE_NONE,
                    max_samples)) != PICO_OK)
                    printf("ERROR: failed to run streaming (0x%X)\n", status);

                /* show actual sampling rate */
                block_us = (((int64_t)sample_interval)*max_samples)/1000000;
                printf("buffer covers %ims...\n", block_us/1000);
                printf("sampling at freqency %.2fMHz (interval %i)...\n",
                    (1000000.0f / sample_interval),
                    sample_interval
                );

                /* run data aquisition */
                while(!g_terminate && ps3000aGetStreamingLatestValues(
                    hscope,
                    samples_ready,
                    buffer
                    ) == PICO_OK)
                {
                    usleep(block_us/4);
                }

                /* stop data acquisition after control-c */
                if((status = ps3000aStop(hscope)) != PICO_OK)
                    printf("Failed to stop data acquisition (0x%08X)\n", status);
            }
        }
    }

    printf("Terminated...\n");
    /* done */
    ps3000aCloseUnit(hscope);
    return 0;
}
