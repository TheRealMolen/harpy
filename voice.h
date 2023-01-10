#pragma once

#include "mln_core.h"

#include "ks.h"
#include "lowpass1p.h"
#include "highpass1p.h"


class Voice
{
public:
	void Init(float sampleRate);

	float Process(float param1);

	void NoteOn(u8 note, float damping);
	float GetFreq() const { return m_string.GetFreq(); }

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
};

