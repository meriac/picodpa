#include <stdio.h>
#include <stdint.h>
#include <ps3000aApi.h>

#define MHZ(x) ((x)*1000000UL)

#define SAMPLE_FREQUENCY MHZ(10)
#define SAMPLE_INTERVAL_NS (1000000000UL/SAMPLE_FREQUENCY)

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
}

int main(int argc, char *argv[])
{
    PICO_STATUS status;
    int max_samples;
    int16_t hscope;

    /* open first available scope */
    if((status = ps3000aOpenUnit(&hscope, NULL)) != PICO_OK)
    {
        printf("Picoscope device open failed [0x%x]\n",(uint32_t)status);
        return 1;
    }

    /* enable dual-buffering */
    if((status = ps3000aMemorySegments(hscope, 2, &max_samples)) != PICO_OK)
        printf("Failed to set memory segment count [0x%x]\n",(uint32_t)status);
    else
    {
        printf("allocating %i samples per buffer (%ims per block)...\n",
            max_samples,
            (int)((SAMPLE_INTERVAL_NS*max_samples)/1000000));
#if 0
        ps3000aSetDataBuffer(
            g_hscope,
            PS3000A_CHANNEL_A,

    sample_interval = SAMPLE_INTERVAL_NS;
    if((status = ps3000aRunStreaming(
        g_hscope,
        &sample_interval,
        PS3000A_NS,
        0,
        SAMPLE_COUNT,
        FALSE,
        0,
        PS3000A_RATIO_MODE_NONE,
#endif
    }


    /* done */
    ps3000aCloseUnit(hscope);
    return 0;
}
