#include "ks.h"

namespace mln
{


float DcBlocker::Process(float in)
{
	constexpr float gain = 0.995f;
	float newOut = in - m_prevIn + (gain * m_prevOut);
	m_prevIn = in;
	m_prevOut = newOut;
	return newOut;
}



void String::Init(float sampleRate)
{
	m_sampleRate = sampleRate;
	m_rSampleRate = 1.f / sampleRate;

	m_filter.Init(sampleRate);
	m_dcBlocker.Init(sampleRate);
	m_delay.Init();
	SetDamping(0.98f);
	SetFilterTrack(1.f);
	SetFreq(220.f);
}


float String::Process(float excite)
{
	const float cyclesPerSample = fclamp(m_freq * m_rSampleRate, 0.f, 0.25f);

	// clamp the delay so we can safely hermite interpolate
	const float samplesPerCycle = fclamp(1.f / cyclesPerSample, 1.f, kMaxDelaySamples - 3.f);
	float out = m_delay.ReadHermite(samplesPerCycle);
	out += excite;

	out = fclamp(out, -10.f, 10.f);

	// without this dc blocker, the filtered exciter can lead us to drift away from zero and the string chokes, particularly at high freqs
	float dcBlocked = m_dcBlocker.Process(out);
	if (m_useDcBlock)
		out = dcBlocked;
	else
		out *= 0.999f;	// ... but this seems to work reasonably well too, and results in a better tone. TODO: tune this! probably wants to be freq-dependent

	out = m_filter.Process(out);

	m_delay.Write(out);

	return out;
}

void String::SetFreq(float hz)
{
	m_freq = hz;
}

void String::SetDamping(float amount)
{
	constexpr float DefaultCutoff = 55.f;
	const float filterRootFreq = flerp(DefaultCutoff, m_freq, m_filterTrack);
	const float filterOctaves = fmaxf(1.f + (amount * amount * 3.f), 1.f);
	const float filterFreqScale = powf(2.f, filterOctaves);
	const float filterFreq = filterFreqScale * filterRootFreq;

	m_filter.SetFreq(filterFreq);
}

}
