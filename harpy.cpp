#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using DaisyHw = DaisySeed;


constexpr float lerp(float a, float b, float t)
{
	return a + (b-a) * t;
}


namespace mln
{

//----------------------------------------------------------------------------------------------------------------
// RgbLed -- helper for addressing an RGB LED connected via 3 pins
//
class RgbLed
{
public:
	void Init(DaisyHw& hw, const Pin& rPin, const Pin& gPin, const Pin& bPin)
	{
		m_rPin.Init(rPin, GPIO::Mode::OUTPUT);
		m_gPin.Init(gPin, GPIO::Mode::OUTPUT);
		m_bPin.Init(bPin, GPIO::Mode::OUTPUT);

		SetRgb(u8(0), u8(0), u8(0));
	}

	void SetRgb(float r, float g, float b)
	{
		m_rPin.Write((r < 0.25f) ? 0 : 1);
		m_gPin.Write((g < 0.25f) ? 0 : 1);
		m_bPin.Write((b < 0.25f) ? 0 : 1);
	}

	void SetRgb(u8 r, u8 g, u8 b)
	{
		m_rPin.Write(r);
		m_gPin.Write(g);
		m_bPin.Write(b);
	}

private:
	GPIO m_rPin, m_gPin, m_bPin;
};


//----------------------------------------------------------------------------------------------------------------
// LowPass1P -- simple one pole filter - see https://www.earlevel.com/main/2012/12/15/a-one-pole-filter/
//
class LowPass1P
{
public:
	void Init(float sampleRate)
	{
		m_rSampleRate = 1.f / sampleRate;
		m_b1 = 0.f;
		m_a0 = 1.f;
		m_z1 = 0.f;
	}

	float Process(float in)
	{
		m_z1 = in * m_a0 + m_z1 * m_b1;
		return m_z1;
	}

	void SetFreq(float cutoff)
	{
		float samplesPerHz = cutoff * m_rSampleRate;
		m_b1 = expf(-TWOPI_F * samplesPerHz);
		m_a0 = 1.f - m_b1;
	}

private:
	float m_rSampleRate = 1.f / 48000.f;
	float m_a0 = 1.f, m_b1 = 0.f;	// coefficients
	float m_z1 = 0.f;
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

private:
	static const u32 kMaxSampRate = 48'000;
	static const u32 kMinNativeFreq = 20;
	static const u32 kMaxDelaySamples = (kMaxSampRate + kMinNativeFreq-1) / kMinNativeFreq;

    DelayLine<float, kMaxDelaySamples>	m_delay;

	float m_sampleRate = 48000.f;
	float m_rSampleRate = 1.f / 48000.f;
	float m_freq = 220.f;
	float m_filterTrack = 1.f;
	LowPass1P m_filter;
};


void String::Init(float sampleRate)
{
	m_sampleRate = sampleRate;
	m_rSampleRate = 1.f / sampleRate;

	m_filter.Init(sampleRate);
	m_delay.Init();
	SetDamping(0.98f);
	SetFilterTrack(1.f);
	SetFreq(220.f);
}

float String::Process(float excite)
{
	float clampedFreq = fclamp(m_freq * m_rSampleRate, 0.f, 0.25f);

	// clamp the delay so we can safely hermite interpolate
	float delayInSamples = fclamp(1.f / clampedFreq, 4.f, kMaxDelaySamples - 4.f);
	float out = m_delay.ReadHermite(delayInSamples);
	out += excite;

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
	constexpr float DefaultCutoff = 200.f;
	float filterFreq = lerp(DefaultCutoff, m_freq, m_filterTrack);
	m_filter.SetFreq(filterFreq * (1.f + amount * 20.f));
}

}

enum class AdcInputs : u8 { Pot1, Pot2, Count };
AdcChannelConfig adcChannelCfgs[u8(AdcInputs::Count)];


DaisyHw hw;

Metro metro;
mln::String string;
AdEnv exciteEnv;
WhiteNoise noise;
Overdrive drive;
Svf lowPass;

mln::RgbLed led1, led2;
GPIO btn1, btn2;
AnalogControl pot1, pot2;


void InitHwIO()
{
	using namespace seed;

	adcChannelCfgs[u8(AdcInputs::Pot1)].InitSingle(hw.GetPin(21));
	adcChannelCfgs[u8(AdcInputs::Pot2)].InitSingle(hw.GetPin(15));
	hw.adc.Init(adcChannelCfgs, u8(AdcInputs::Count));

	pot1.Init(hw.adc.GetPtr(u8(AdcInputs::Pot1)), hw.AudioCallbackRate());
	pot2.Init(hw.adc.GetPtr(u8(AdcInputs::Pot2)), hw.AudioCallbackRate());

	btn1.Init(D27, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
	btn2.Init(D28, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);

	led1.Init(hw, D20, D19, D18);
	led2.Init(hw, D17, D24, D23);
}


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	pot1.Process();
	pot2.Process();

	for (size_t i = 0; i < size; i++)
	{
		if (metro.Process())
		{
			exciteEnv.Trigger();
			string.SetFreq(fmap(pot1.Value(), 55.f, 880.f, Mapping::EXP));
			string.SetDamping(pot2.Value());
		}

		float exciteEnvAmount = exciteEnv.Process();
		float excite = exciteEnvAmount * exciteEnvAmount * noise.Process();
		float rawString = string.Process(excite);

		[[maybe_unused]] float driven = drive.Process(rawString);

		lowPass.Process(rawString);
		[[maybe_unused]] float filtered = lowPass.Low();

		float output = filtered;
		OUT_L[i] = output;
		OUT_R[i] = output;
	}
}

int main(void)
{
	hw.Init();

    //hw.StartLog(false);
    //hw.PrintLine("heyo dumbass");

	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

	InitHwIO();

	auto sampleRate = hw.AudioSampleRate();
	metro.Init(2.f, sampleRate);
	exciteEnv.Init(sampleRate);
	string.Init(sampleRate);
	noise.Init();
	drive.Init();
	lowPass.Init(sampleRate);

	lowPass.SetFreq(3000.f);
	lowPass.SetRes(0.f);

	exciteEnv.SetTime(ADENV_SEG_ATTACK, 0.001f);
	exciteEnv.SetTime(ADENV_SEG_DECAY, 0.01f);

	string.SetFreq(110.f);

	hw.StartAudio(AudioCallback);
    hw.adc.Start();

	u8 col = 0;
	while(1)
	{
		hw.DelayMs(500);

		led1.SetRgb(u8(col & 4), u8(col & 2), u8(col & 1));
		++col;
		led2.SetRgb(btn1.Read() ? 1.f : 0.f, pot1.Value(), pot2.Value());
	}
}
