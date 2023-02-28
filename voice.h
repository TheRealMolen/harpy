#pragma once

#include "mln_core.h"

#include "ks.h"
#include "lowpass1p.h"
#include "highpass1p.h"

#define SUPA_LO_BIT


class Voice
{
public:
	void Init(float sampleRate);

	float Process(float param1);

	void NoteOn(u8 note, float damping);

#ifndef SUPA_LO_BIT
	void UseDcBlock(bool use) { m_string.UseDcBlock(use); }

private:

    float ProcessKS();
    float ProcessLofi();

    // ks string ---------------
	mln::String m_string;
	daisysp::AdEnv m_exciteEnv;
	daisysp::WhiteNoise m_noise;
	daisysp::Svf m_lowPass;

    // lofi string -------------
    daisysp::Oscillator m_osc;
    daisysp::Tremolo m_trem;
	daisysp::AdEnv m_lofiEnv;
    mln::LowPass1P m_lofiLP1;
    mln::LowPass1P m_lofiLP2;
    mln::HighPass1P m_lofiHP1;
    mln::HighPass1P m_lofiHP2;
#else
private:
    float ProcessSLB(float wave);

    float m_freq = 220.f;
    float m_phase = 0.f;
    float m_phaseIncPerSample = 220.f * kRecipSampRate;
    float m_samplesPerCycle = kSampleRate / 220.f;
    
	daisysp::AdEnv m_env;
#endif
};

