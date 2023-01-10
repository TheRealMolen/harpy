#include "voice.h"

#include "daisyclock.h"


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

    m_osc.Init(sampleRate);
    m_lofiEnv.Init(sampleRate);
    m_lofiLP.Init(sampleRate);
    m_lofiHP.Init(sampleRate);
    m_trem.Init(sampleRate);

    m_osc.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
	m_lofiEnv.SetTime(daisysp::ADENV_SEG_ATTACK, 0.05f);
	m_lofiEnv.SetTime(daisysp::ADENV_SEG_DECAY, 0.5f);
    m_lofiLP.SetFreq(2000.f);
    m_lofiHP.SetFreq(200.f);
    m_trem.SetDepth(0.1f);
    m_trem.SetFreq(10.f);
}

float Voice::Process(float param1)
{
    float ks = ProcessKS();
    float lofi = ProcessLofi();
    return flerp(ks, lofi, param1);
}

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
    out *= (1.f / 6.f);    // approx match volume with ks

    //out = m_trem.Process(out);

    m_lofiEnv.Process();
    out *= m_lofiEnv.GetValue();

    m_lofiHP.Process(out);
    out = m_lofiHP.High();

    m_lofiLP.Process(out);
    out = m_lofiLP.Low();

    return out;
}


void Voice::NoteOn(u8 note, float damping)
{
    float freq = daisysp::mtof(note);

	m_exciteEnv.Trigger();
	m_string.SetFreq(freq);
	m_string.SetDamping(damping);

    m_lofiEnv.Trigger();
    m_osc.SetFreq(freq);
    m_lofiEnv.SetTime(daisysp::ADENV_SEG_DECAY, 0.2f + 1.1f * damping);
}
