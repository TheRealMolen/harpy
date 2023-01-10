//
// d a i s y c l o c k
//
// super lightweight cpu profiler for daisyseed
// prioritises low-cost over accuracy
// 
// (i realise it's dandelions that have clocks, not daisies. c'est la vie.)
//

#pragma once

#include <cstdint>
#include "Drivers/CMSIS/Device/ST/STM32H7xx/Include/stm32h750xx.h"
#include "daisy_seed.h"

#define D_ENABLE_CPU_PROFILINGx


#ifdef D_ENABLE_CPU_PROFILING

//----------------------------------------------------------------------------------------------------------------

class DaisyClock
{
public:
    enum class Timer
    {
        // add user timers here:
        AudioCallback,
        VoiceManager,
        Voice_KS,
        Voice_Lofi,

        // leave these ones alone for the system
        Total,          // the full time from one call to Reset to the next
        Count,
    };
    static const int NumTimers = int(Timer::Count);
    static const char* TimerNames[];

    // all times are reported in microseconds
    using micros_t = uint32_t;
    struct Report
    {
        micros_t timersUs[NumTimers];
    };

public:
    static DaisyClock& Get() { return g_clock; }

    // call this once at startup time
    template<typename DaisyHardware = daisy::DaisySeed>
    void Init(DaisyHardware& hw, TIM_TypeDef* hwTimer = TIM2)
    {
        m_hwTimer = hwTimer;
        m_hwTimerFreq = hw.system.GetTickFreq();

        const uint32_t micros = 1'000'000'000ul;
        m_hwTimerNanoScale = micros / m_hwTimerFreq;

        hw.PrintLine("--- CPU Profiling initialised, freq=%u, scale=%u ---", m_hwTimerFreq, m_hwTimerNanoScale);
    }

    Report BuildReport() const
    {
        Report report;
        for (int t=0; t<DaisyClock::NumTimers; ++t)
            report.timersUs[t] = CalcMicroDuration(m_timersHw[t]);
        return report;
    }

    // call this to reset all the timers - typically straight after calling GetReport()
    void Reset()
    {
        for (auto& t : m_timersHw)
            t = 0;
    }

    inline void AccumulateTime(Timer timer, uint32_t startTimepoint, uint32_t endTimepoint)
    {
        uint64_t durationHw = endTimepoint - startTimepoint;
        m_timersHw[int(timer)] += durationHw;
    }

    // get the tick count right now from the hardware timer
    [[nodiscard]] inline uint32_t GetHwTimePoint() const
    {
        return m_hwTimer->CNT;
    }

private:
    TIM_TypeDef*    m_hwTimer = TIM2;
    uint32_t        m_hwTimerFreq = 200'000'000;
    uint32_t        m_hwTimerNanoScale = 5; // multiply a timepoint delta by this amount to get microseconds

    uint32_t        m_timersHw[NumTimers];  // stored in hw timer increments -- multi


    [[nodiscard]] inline timer_t CalcMicroDuration(uint64_t durationHw) const
    {
        return timer_t(durationHw * m_hwTimerNanoScale);
    }
private:
    // single global instance
    static DaisyClock g_clock;
};


//----------------------------------------------------------------------------------------------------------------

class ProfileAutoTime
{
    ProfileAutoTime() = delete;
public:
    explicit ProfileAutoTime(DaisyClock::Timer timer)
        : m_timer(timer)
        , m_startTicks(DaisyClock::Get().GetHwTimePoint())
    { /**/ }

    ~ProfileAutoTime()
    {
        auto endTicks = DaisyClock::Get().GetHwTimePoint();
        DaisyClock::Get().AccumulateTime(m_timer, m_startTicks, endTicks);
    }

private:
    DaisyClock::Timer m_timer;
    uint32_t m_startTicks;
};

//----------------------------------------------------------------------------------------------------------------

// commonly used like:
//      auto& clock = DaisyClock::Get();
//      auto report = clock.BuildReport(report);
//      PrintReport(seed, report);
//      clock.Reset();
template<typename DaisyHardware = daisy::DaisySeed>
void PrintReport(DaisyHardware& hw, const DaisyClock::Report& report)
{
    hw.Print("--- CPU: ");
    for (int t=0; t<DaisyClock::NumTimers; ++t)
    {
        timer_t time = report.timersUs[t];
        const char* units = "us";

        if (time > 100'000)
        {
            units = "ms";
            time /= 1'000;
        }

        uint32_t fraction = uint32_t(time % 1000);
        uint32_t whole = uint32_t(time / 1000);
        hw.Print("%s=%u.%03u%s, ", DaisyClock::TimerNames[t], whole, fraction, units);
    }
    hw.PrintLine(" ---");
}

//----------------------------------------------------------------------------------------------------------------

#define INITPROFILER(hw)        DaisyClock::Get().Init(hw)
#define AUTOPROFILE(timer)      ProfileAutoTime autoprofile_##timer(DaisyClock::Timer:: timer)

#else

#define INITPROFILER(hw)        do{}while(0)
#define AUTOPROFILE(timer)      do{}while(0)

#endif // D_ENABLE_CPU_PROFILING
