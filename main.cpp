#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <ps3000aApi.h>

#define MHZ(x) ((x)*1000000.0)
#define SAMPLE_FREQUENCY MHZ(62.5)
#define SAMPLE_INTERVAL_NS (int)(((1000000000.0/SAMPLE_FREQUENCY)+0.5))
#define FILTER_FREQUENCY 1000.0
#define FILTER_INTERVAL_NS (int)(((1000000000.0/FILTER_FREQUENCY)+0.5))

#define CHANNELS 1

static volatile bool g_terminate;
static uint32_t g_sample_interval_ns;
static int g_oversampling;
static volatile int g_buffer_count;

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
    int16_t *buffer;
    static int progress;

    buffer = ((int16_t*)pParameter)+startIndex;
    while(noOfSamples--)
    {
        g_oversampling += *buffer++;
        if(progress < FILTER_INTERVAL_NS)
            progress += g_sample_interval_ns;
        else
        {
            progress -= FILTER_INTERVAL_NS;
            fwrite(&g_oversampling, sizeof(g_oversampling), 1, stdout);
            g_oversampling = 0;
            g_buffer_count++;
        }
    }
}

void shutdown_request(int sig){
    g_terminate = true;
}

int main(int argc, char *argv[])
{
    PICO_STATUS status;
    int i, max_samples, block_us;
    int16_t hscope;
    int16_t *buffer;

    /* hook termination handler */
    signal(SIGINT, shutdown_request);

    /* open first available scope */
    if((status = ps3000aOpenUnit(&hscope, NULL)) != PICO_OK)
    {
        fprintf(stderr,"Picoscope device open failed [0x%x]\n",(uint32_t)status);
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
        fprintf(stderr,"failed to configure channel [0x%X]\n",(uint32_t)status);
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
        fprintf(stderr,"failed to configure channel [0x%x]\n",(uint32_t)status);
        return 3;
    }

    /* enable dual-buffering */
    if((status = ps3000aMemorySegments(hscope, CHANNELS, &max_samples)) != PICO_OK)
        fprintf(stderr,"Failed to set memory segment count [0x%X]\n",(uint32_t)status);
    else
    {
        fprintf(stderr,"allocating %i samples for buffer...\n", max_samples);

        /* allocate buffer memories */
        buffer = (int16_t*)
            malloc(max_samples * CHANNELS * sizeof(buffer[0]));
        if(!buffer)
            fprintf(stderr,"ERROR: failed to allocate memory!\n");
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
                fprintf(stderr,"ERROR: registering channel %i (0x%X)!\n", i+1, status);
            else
            {
                /* run data aquisition */
                g_sample_interval_ns = SAMPLE_INTERVAL_NS;
                if((status = ps3000aRunStreaming(
                    hscope,
                    &g_sample_interval_ns,
                    PS3000A_NS,
                    0,
                    max_samples,
                    false,
                    1,
                    PS3000A_RATIO_MODE_NONE,
                    max_samples)) != PICO_OK)
                    fprintf(stderr,"ERROR: failed to run streaming (0x%X)\n", status);

                /* show actual sampling rate */
                block_us = (g_sample_interval_ns*max_samples)/1000;
                fprintf(stderr,"buffer covers %ims...\n", block_us/1000);
                fprintf(stderr,"sampling at freqency %.2fMHz (interval %ins)...\n",
                    (1000.0f / g_sample_interval_ns),
                    g_sample_interval_ns
                );
                fprintf(stderr,"filter frequency %.2fkHz (~%i samples per %ius interval)...\n",
                    FILTER_FREQUENCY/1000.0,
                    FILTER_INTERVAL_NS / g_sample_interval_ns,
                    FILTER_INTERVAL_NS / 1000
                );

                /* run data aquisition */
                while(!g_terminate && ps3000aGetStreamingLatestValues(
                    hscope,
                    samples_ready,
                    buffer
                    ) == PICO_OK)
                {
                    usleep(block_us/4);

                    fprintf(stderr, "\rsamples=%08i", g_buffer_count);
                    fflush(stderr);
                }
                fprintf(stderr,"\n");

                /* stop data acquisition after control-c */
                if((status = ps3000aStop(hscope)) != PICO_OK)
                    fprintf(stderr,"Failed to stop data acquisition (0x%08X)\n", status);
            }
        }
    }

    fprintf(stderr,"Terminated...\n");
    /* done */
    ps3000aCloseUnit(hscope);
    return 0;
}
