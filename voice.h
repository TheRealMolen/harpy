#pragma once

#include "mln_core.h"

#include "fm_operator.h"
#include "harm_osc.h"
#include "ks.h"
#include "filterstack.h"
#include "lowpass1p.h"
#include "highpass1p.h"

#define VOICE_HARP          (1)
#define VOICE_SUPA_LO_BIT   (2)
#define VOICE_FM            (3)
#define VOICE_HARM          (4)

#define V_VOICE_MODE    VOICE_HARM


class Voice
{
public:
	void Init(float sampleRate);

	float Process(float param0, float param1, float param2);

	void NoteOn(u8 note, float damping, float param0, float param1, float param2);
    void NoteOff();

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
    float m_freq = 220.f;
    float m_phase = 0.f;
    float m_phaseIncPerSample = 220.f * kRecipSampRate;
    float m_samplesPerCycle = kSampleRate / 220.f;

    daisysp::Tremolo m_trem;
	daisysp::AdEnv m_lofiEnv;
    mln::FilterStack<2> m_lofiLP;
    mln::FilterStack<2, mln::FilterType::HiPass> m_lofiHP;
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

#elif V_VOICE_MODE == VOICE_HARM
private:
    float ProcessHarm(float p0, float p1, float p2);

    mln::HarmonicOsc m_osc;
	daisysp::Adsr m_env;
    bool m_gate;
#else
#error unknown voice mode
#endif
};

