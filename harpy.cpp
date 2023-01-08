#include "daisy_seed.h"
#include "daisysp.h"

#include "ks.h"
#include "mln_core.h"

using namespace daisy;
using namespace daisysp;

using DaisyHw = DaisySeed;

DelayLine<float, 1>a;
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
			string.SetFreq(fmap(pot1.Value(), 55.f, 110.f * 16.f, Mapping::EXP));
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

	lowPass.SetFreq(2000.f);
	lowPass.SetRes(0.f);

	exciteEnv.SetTime(ADENV_SEG_ATTACK, 0.0001f);
	exciteEnv.SetTime(ADENV_SEG_DECAY, 0.005f);

	string.SetFreq(110.f);

	hw.StartAudio(AudioCallback);
    hw.adc.Start();

	u8 col = 0;
	while(1)
	{
		hw.DelayMs(500);

		if (!btn1.Read())
		{
			string.UseDcBlock(true);
			led2.SetRgb(0.f, 1.f, 0.f);
		}
		else if (!btn2.Read())
		{
			string.UseDcBlock(false);
			led2.SetRgb(1.f, 0.f, 0.f);
		}

		led1.SetRgb(u8(col & 4), u8(col & 2), u8(col & 1));
		++col;
		//led2.SetRgb(btn1.Read() ? 1.f : 0.f, pot1.Value(), pot2.Value());
	}
}
