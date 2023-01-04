#include "daisy_seed.h"
#include "daisysp.h"

#include "Control/adenv.h"
#include "Effects/overdrive.h"
#include "PhysicalModeling/KarplusString.h"
#include "Noise/whitenoise.h"
#include "Filters/svf.h"

using namespace daisy;
using namespace daisysp;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using DaisyHw = DaisySeed;


namespace mln
{

class RgbLed
{
public:
	void Init(DaisyHw& hw, u8 rPinIx, u8 gPinIx, u8 bPinIx)
	{
		m_r.pin = hw.GetPin(rPinIx);
		m_g.pin = hw.GetPin(gPinIx);
		m_b.pin = hw.GetPin(bPinIx);

		m_r.mode = DSY_GPIO_MODE_OUTPUT_PP;
		m_g.mode = DSY_GPIO_MODE_OUTPUT_PP;
		m_b.mode = DSY_GPIO_MODE_OUTPUT_PP;

		dsy_gpio_init(&m_r);
		dsy_gpio_init(&m_g);
		dsy_gpio_init(&m_b);

		SetRgb(0.f, 0.f, 0.f);
	}

	void SetRgb(float r, float g, float b)
	{
		dsy_gpio_write(&m_r, (r < 0.25f) ? 0 : 1);
		dsy_gpio_write(&m_g, (g < 0.25f) ? 0 : 1);
		dsy_gpio_write(&m_b, (b < 0.25f) ? 0 : 1);
	}

	void SetRgb(u8 r, u8 g, u8 b)
	{
		dsy_gpio_write(&m_r, r);
		dsy_gpio_write(&m_g, g);
		dsy_gpio_write(&m_b, b);
	}

private:
	dsy_gpio m_r, m_g, m_b;
};
}

enum class AdcInputs : u8 { Pot1, Pot2, Count };
AdcChannelConfig adcChannelCfgs[u8(AdcInputs::Count)];


DaisyHw hw;

Metro metro;
String string;
AdEnv exciteEnv;
WhiteNoise noise;
Overdrive drive;
Svf lowPass;

mln::RgbLed led1, led2;
Switch btn1, btn2;
AnalogControl pot1, pot2;


void InitHwIO()
{
	adcChannelCfgs[u8(AdcInputs::Pot1)].InitSingle(hw.GetPin(21));
	adcChannelCfgs[u8(AdcInputs::Pot2)].InitSingle(hw.GetPin(15));
	hw.adc.Init(adcChannelCfgs, u8(AdcInputs::Count));

	pot1.Init(hw.adc.GetPtr(u8(AdcInputs::Pot1)), hw.AudioCallbackRate());
	pot2.Init(hw.adc.GetPtr(u8(AdcInputs::Pot2)), hw.AudioCallbackRate());

	btn1.Init(hw.GetPin(27));
	btn2.Init(hw.GetPin(28));

	led1.Init(hw, 20, 19, 18);
	led2.Init(hw, 17, 24, 23);
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
			//string.SetFreq(btn1.RawState() ? 180.f : 55.f);
			string.SetFreq(fmap(pot1.Value(), 55.f, 880.f, Mapping::EXP));
			string.SetDamping(pot2.Value());
		}

		float excite = exciteEnv.Process() * noise.Process();
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
	exciteEnv.SetTime(ADENV_SEG_DECAY, 0.02f);

	string.SetFreq(110.f);

	hw.StartAudio(AudioCallback);
    hw.adc.Start();

	u8 col = 0;
	while(1)
	{
		hw.DelayMs(500);

		led1.SetRgb(u8(col & 4), u8(col & 2), u8(col & 1));
		++col;
		led2.SetRgb(0.f, pot1.Value(), pot2.Value());
	}
}
