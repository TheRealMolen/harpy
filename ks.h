#pragma once

#include "Utility/delayline.h"
#include "Utility/dsp.h"

#include "lowpass1p.h"
#include "mln_core.h"


namespace mln
{

class DcBlocker
{
public:
	void Init(float sampleRate) {/**/}
	float Process(float in);

private:
	float m_prevIn = 0.f;
	float m_prevOut = 0.f;
};

//----------------------------------------------------------------------------------------------------------------
// String -- wangled ks stringish implementation
//
class String
{
public:
	void Init(float sampleRate);

	float Process(float excite);

	void SetFreq(float hz);
	void SetDamping(float amount);
	void SetFilterTrack(float track)	{ m_filterTrack = track; }

	float GetFreq() const { return m_freq; }

	void UseDcBlock(bool use) { m_useDcBlock = use; }

private:
	static const u32 kMaxSampRate = 48'000;
	static const u32 kMinNativeFreq = 40;
	static const u32 kMaxDelaySamples = daisysp::get_next_power2((kMaxSampRate + kMinNativeFreq-1) / kMinNativeFreq);

    daisysp::DelayLine<float, kMaxDelaySamples>	m_delay;

	float m_sampleRate = 48000.f;
	float m_rSampleRate = 1.f / 48000.f;
	float m_freq = 220.f;
	float m_filterTrack = 1.f;
	bool m_useDcBlock = false;
	LowPass1P m_filter;
    DcBlocker m_dcBlocker;
};

}
