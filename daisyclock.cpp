#include "daisyclock.h"

#ifdef D_ENABLE_CPU_PROFILING

//----------------------------------------------------------------------------------------------------------------

const char* DaisyClock::TimerNames[] =
{
    "ac",   // AudioCallback
    "vm",   // VoiceManager
    "ks",   // Voice_KS
    "lo",   // Voice_LoFi
    "To",   // Total
};
static_assert(sizeof(DaisyClock::TimerNames) / sizeof(DaisyClock::TimerNames[0]) == DaisyClock::NumTimers, "daisyclock timer names have drifted out of sync");

// global clock instance
DaisyClock DaisyClock::g_clock;

//----------------------------------------------------------------------------------------------------------------

#endif // D_ENABLE_CPU_PROFILING
