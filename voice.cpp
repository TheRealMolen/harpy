#include "voice.h"

#include "daisyclock.h"


void Voice::Init(float sampleRate)
{
#ifndef SUPA_LO_BIT
	m_string.Init(sampleRate);
	m_exciteEnv.Init(sampleRate);
	m_noise.Init();
	m_lowPass.Init(sampleRate);

	m_lowPass.SetFreq(2000.f);
	m_lowPass.SetRes(0.f);

	m_exciteEnv.SetTime(daisysp::ADENV_SEG_ATTACK, 0.0001f);
	m_exciteEnv.SetTime(daisysp::ADENV_SEG_DECAY, 0.005f);

    m_osc.Init(sampleRate);
    m_lofiEnv.Init(sampleRate);
    m_lofiLP1.Init(sampleRate);
    m_lofiLP2.Init(sampleRate);
    m_lofiHP1.Init(sampleRate);
    m_lofiHP2.Init(sampleRate);
    m_trem.Init(sampleRate);

    m_osc.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
	m_lofiEnv.SetTime(daisysp::ADENV_SEG_ATTACK, 0.002f);
	m_lofiEnv.SetTime(daisysp::ADENV_SEG_DECAY, 0.5f);
    m_lofiLP1.SetFreq(2000.f);
    m_lofiLP2.SetFreq(2000.f);
    m_lofiHP1.SetFreq(200.f);
    m_lofiHP2.SetFreq(200.f);
    m_trem.SetDepth(0.3f);
    m_trem.SetFreq(5.f);
#else

    m_env.Init(sampleRate);
	m_env.SetTime(daisysp::ADENV_SEG_ATTACK, 0.002f);
	m_env.SetTime(daisysp::ADENV_SEG_DECAY, 0.5f);

#endif
}

float Voice::Process(float param1)
{
#ifndef SUPA_LO_BIT
    float ks = ProcessKS();
    float lofi = ProcessLofi();
    return flerp(ks, lofi, param1);

#else
    return ProcessSLB(param1);
#endif
}


#ifndef SUPA_LO_BIT
float Voice::ProcessKS()
{
    AUTOPROFILE(Voice_KS);

	float exciteEnvAmount = m_exciteEnv.Process();
	float excite = exciteEnvAmount * exciteEnvAmount * m_noise.Process();
	float rawString = m_string.Process(excite);

	m_lowPass.Process(rawString);
	float filtered = m_lowPass.Low();

	return filtered;
}

float Voice::ProcessLofi()
{
    AUTOPROFILE(Voice_Lofi);
    
    float out = m_osc.Process();
    out *= (1.f / 4.f);    // approx match volume with ks

    out = m_trem.Process(out);

    m_lofiEnv.Process();
    out *= m_lofiEnv.GetValue();

    out = m_lofiHP1.Process(out);
    out = m_lofiHP2.Process(out);

    out = m_lofiLP1.Process(out);
    out = m_lofiLP2.Process(out);

    return out;
}
#else


/*
    this sounds quite nice but is *not* a triangle...

        const float negOnePoint = (0.25f * triSlope);
        const float onePoint = 1.f - negOnePoint;
        float tri;
        if (m_phase < negOnePoint)
            tri = m_phase * (-1.f / negOnePoint);
        else if (m_phase < onePoint)
            tri = -1.f + 2.f * ((m_phase - negOnePoint) / (onePoint - negOnePoint));
        else
            tri = (onePoint - m_phase) * (1.f / negOnePoint);
*/


// returns the value to subtract from a [-1,1] signal to smooth out a positive edge when phase crosses 1->0
// phase in [0,1)
inline float calcPolyBlepCorrection(float phase, float phaseIncPerSample, float samplesPerCycle)
{
    // just before a transition
    if (phase >= (1.f - phaseIncPerSample)) [[unlikely]]
    {
        float t = (phase - 1.f) * samplesPerCycle;
        return (t * t) + (2.f * t) + 1.f;
    }

    // just after transition
    if (phase < phaseIncPerSample)  [[unlikely]]
    {
        float t = phase * samplesPerCycle;
        return (2.f * t) - (t * t) - 1.f;
    }

    return 0.f;
}


inline float calcSawTri(float phase, float triangleness)
{
    const float negOnePoint = 0.5f * (2.f - triangleness);

    // TODO: blep if triangleness is so low that we rise from ~-1 to ~1 in one sample
    
    if (phase <= negOnePoint)
        return 1.f - 2.f * (phase / negOnePoint);
    
    return -1.f + 2.f * ((phase - negOnePoint) / (1.f - negOnePoint));
}


float Voice::ProcessSLB(float wave)
{
    m_phase += m_phaseIncPerSample;
    if (m_phase >= 1.f)
        m_phase -= 1.f;
    
    constexpr float squawCutoff = 0.6f;
    float out = 0.f;
    if (wave <= squawCutoff + 0.02f)
    {
        const float squawSlope = wave * (1.f / squawCutoff);  // 0 is square, 1 is saw

        float squaw;
        float phase180;
        if (m_phase < 0.5f)
        {
            squaw = 1.f - (squawSlope * 2.f * m_phase);
            phase180 = m_phase + 0.5f;
        }
        else
        {
            squaw = (2.f * squawSlope * (1.f - m_phase)) - 1.f;
            phase180 = m_phase - 0.5f;
        }
      
        squaw += calcPolyBlepCorrection(m_phase, m_phaseIncPerSample, m_samplesPerCycle);

        const float midCorrection = calcPolyBlepCorrection(phase180, m_phaseIncPerSample, m_samplesPerCycle);
        squaw -= (1.f - squawSlope) * midCorrection;

        out = squaw * 0.1f;
    }
    else
    {
        const float triSlope = (wave - squawCutoff) * (1.f / (1.f - squawCutoff));   // 0 is saw, 1 is tri
        const float tri = calcSawTri(m_phase, triSlope);
        out = tri * 0.1f;
    }

    m_env.Process();
    out *= m_env.GetValue();

    return out;
}

#endif

void Voice::NoteOn(u8 note, float damping)
{
    float freq = daisysp::mtof(note);

#ifndef SUPA_LO_BIT
	m_exciteEnv.Trigger();
	m_string.SetFreq(freq);
	m_string.SetDamping(damping);

    m_lofiEnv.Trigger();
    m_osc.SetFreq(freq);
    m_lofiEnv.SetTime(daisysp::ADENV_SEG_DECAY, 0.2f + 1.1f * damping);
#else
    m_phaseIncPerSample = freq * kRecipSampRate;
    m_samplesPerCycle = kSampleRate / freq;
    
    m_env.SetTime(daisysp::ADENV_SEG_DECAY, 0.2f + 1.1f * damping);
    m_env.Trigger();
#endif
}
