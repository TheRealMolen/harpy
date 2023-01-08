#pragma once

#include "mln_core.h"

#include "ks.h"


class Voice
{
public:
	void Init(float sampleRate);

	float Process();

	void NoteOn(u8 note, float damping);
	float GetFreq() const { return m_string.GetFreq(); }

	void UseDcBlock(bool use) { m_string.UseDcBlock(use); }

private:
	mln::String m_string;
	daisysp::AdEnv m_exciteEnv;
	daisysp::WhiteNoise m_noise;
	daisysp::Svf m_lowPass;
};

void Voice::Init(float sampleRate)
{
	m_string.Init(sampleRate);
	m_exciteEnv.Init(sampleRate);
	m_noise.Init();
	m_lowPass.Init(sampleRate);

	m_lowPass.SetFreq(2000.f);
	m_lowPass.SetRes(0.f);

	m_exciteEnv.SetTime(daisysp::ADENV_SEG_ATTACK, 0.0001f);
	m_exciteEnv.SetTime(daisysp::ADENV_SEG_DECAY, 0.005f);
}

float Voice::Process()
{
	float exciteEnvAmount = m_exciteEnv.Process();
	float excite = exciteEnvAmount * exciteEnvAmount * m_noise.Process();
	float rawString = m_string.Process(excite);

	m_lowPass.Process(rawString);
	float filtered = m_lowPass.Low();

	return filtered;
}

void Voice::NoteOn(u8 note, float damping)
{
	m_exciteEnv.Trigger();
	m_string.SetFreq(daisysp::mtof(note));
	m_string.SetDamping(damping);
}

