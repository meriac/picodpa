#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <ps3000aApi.h>

#define MHZ(x) ((x)*1000000UL)

#define SAMPLE_FREQUENCY MHZ(10)
#define SAMPLE_INTERVAL_NS (1000000000UL/SAMPLE_FREQUENCY)

#define CHANNELS 1

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
    printf("idx=%i samples=%i\n", startIndex, noOfSamples);
}

int main(int argc, char *argv[])
{
    PICO_STATUS status;
    int i, max_samples, block_us;
    uint32_t sample_interval;
    int16_t hscope;
    int16_t *buffer;

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
        return 1;
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
        return 1;
    }

    /* enable dual-buffering */
    if((status = ps3000aMemorySegments(hscope, CHANNELS, &max_samples)) != PICO_OK)
        printf("Failed to set memory segment count [0x%X]\n",(uint32_t)status);
    else
    {
        block_us = (SAMPLE_INTERVAL_NS*max_samples)/1000; 
        printf("allocating %i samples per buffer (%ims per block)...\n",
            max_samples, block_us/1000);

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
                sample_interval = SAMPLE_INTERVAL_NS;
                if((status = ps3000aRunStreaming(
                    hscope,
                    &sample_interval,
                    PS3000A_NS,
                    0,
                    max_samples,
                    false,
                    1,
                    PS3000A_RATIO_MODE_NONE,
                    max_samples)) != PICO_OK)
                    printf("ERROR: failed to run streaming (0x%X)\n", status);

                /* show actual sampling rate */
                printf("sampling at freqency %.2fMHz (interval %i)...\n",
                    (1000.0f / sample_interval),
                    sample_interval
                );

                /* run data aquisition */
               while(ps3000aGetStreamingLatestValues(
                    hscope,
                    samples_ready,
                    buffer
                    ) == PICO_OK)
               {
                    usleep(block_us/4);
               }
            }
        }
    }


    /* done */
    ps3000aCloseUnit(hscope);
    return 0;
}