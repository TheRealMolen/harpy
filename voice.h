#pragma once

#include "mln_core.h"

#include "fm_operator.h"
#include "ks.h"
#include "lowpass1p.h"
#include "highpass1p.h"

#define VOICE_HARP          (1)
#define VOICE_SUPA_LO_BIT   (2)
#define VOICE_FM            (3)

#define V_VOICE_MODE    VOICE_FM


class Voice
{
public:
	void Init(float sampleRate);

	float Process(float param0, float param1, float param2);

	void NoteOn(u8 note, float damping);

#if V_VOICE_MODE == VOICE_HARP
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
#elif V_VOICE_MODE == VOICE_SUPA_LO_BIT
private:
    float ProcessSLB(float wave);

    float m_freq = 220.f;
    float m_phase = 0.f;
    float m_phaseIncPerSample = 220.f * kRecipSampRate;
    float m_samplesPerCycle = kSampleRate / 220.f;
    
	daisysp::AdEnv m_env;
#elif V_VOICE_MODE == VOICE_FM
private:
    float ProcessFM(float p0, float p1, float p2);

    mln::FmOperator m_op1;
    mln::FmOperator m_op2;
    
	daisysp::AdEnv m_envCarrier;
	daisysp::AdEnv m_env;
#else
#error unknown voice mode
#endif
};

