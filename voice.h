#pragma once

#include "mln_core.h"

#include "ks.h"


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
    daisysp::Svf m_lofiLP;
    daisysp::Svf m_lofiHP;
};

