#include "daisy_seed.h"
#include "daisysp.h"

#include "ks.h"
#include "mln_core.h"

using DaisyHw = daisy::DaisySeed;
using daisy::GPIO;
using daisy::Pin;

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


class Voice
{
public:
	void Init(float sampleRate);

	float Process();

	void NoteOn(u8 note, float damping);

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


enum class AdcInputs : u8 { Pot1, Pot2, Count };
daisy::AdcChannelConfig adcChannelCfgs[u8(AdcInputs::Count)];


DaisyHw hw;
daisy::MidiUartHandler midi;

Voice voice;

mln::RgbLed led1, led2;
GPIO btn1, btn2;
daisy::AnalogControl pot1, pot2;


void InitHwIO()
{
	using namespace daisy::seed;

	adcChannelCfgs[u8(AdcInputs::Pot1)].InitSingle(hw.GetPin(21));
	adcChannelCfgs[u8(AdcInputs::Pot2)].InitSingle(hw.GetPin(15));
	hw.adc.Init(adcChannelCfgs, u8(AdcInputs::Count));

	pot1.Init(hw.adc.GetPtr(u8(AdcInputs::Pot1)), hw.AudioCallbackRate());
	pot2.Init(hw.adc.GetPtr(u8(AdcInputs::Pot2)), hw.AudioCallbackRate());

	btn1.Init(D27, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
	btn2.Init(D28, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);

	led1.Init(hw, D20, D19, D18);
	led2.Init(hw, D17, D24, D23);

    daisy::MidiUartHandler::Config midi_config;
    midi.Init(midi_config);
}


void AudioCallback(daisy::AudioHandle::InputBuffer in, daisy::AudioHandle::OutputBuffer out, size_t size)
{
	pot1.Process();
	pot2.Process();

	for (size_t i = 0; i < size; i++)
	{
		float output = voice.Process();
		OUT_L[i] = output;
		OUT_R[i] = output;
	}
}


void HandleNoteOff(u8 chan, u8 note)
{
}
void HandleNoteOn(u8 chan, u8 note, u8 vel)
{
	if (vel == 0)
		return HandleNoteOff(chan, note);

	voice.NoteOn(note, pot2.Value());
}

void HandleMidiMessage(const daisy::MidiEvent& msg)
{
	hw.PrintLine("midi: m=%02x, c=%02x [%d, %d]", int(msg.type), int(msg.channel), int(msg.data[0]), int(msg.data[1]));

	switch (msg.type)
	{
		case daisy::NoteOn:		return HandleNoteOn(msg.channel, msg.data[0], msg.data[1]);
		case daisy::NoteOff:	return HandleNoteOff(msg.channel, msg.data[0]);
		default: break;
	}
}


int main(void)
{
	hw.Init();

 //   hw.StartLog(false);
 //   hw.PrintLine("heyo dumbass");

	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);

	InitHwIO();

	auto sampleRate = hw.AudioSampleRate();
	voice.Init(sampleRate);

	midi.StartReceive();

	hw.StartAudio(AudioCallback);
    hw.adc.Start();

	[[maybe_unused]] u32 lastMsgTick = hw.system.GetNow();
	[[maybe_unused]] const u32 msgFreq = 1000;
	for (;;)
	{
		// u32 now = hw.system.GetNow();
		// if ((now - lastMsgTick) > msgFreq)
		// {
		// 	hw.PrintLine("still here...");
		// 	lastMsgTick = now;
		// }

		// tick the UART receiver
		midi.Listen();

		// handle any new midi events
		while (midi.HasEvents())
			HandleMidiMessage(midi.PopEvent());

		if (!btn1.Read())
		{
			voice.UseDcBlock(true);
			led2.SetRgb(0.f, 1.f, 0.f);
		}
		else if (!btn2.Read())
		{
			voice.UseDcBlock(false);
			led2.SetRgb(1.f, 0.f, 0.f);
		}

		//led2.SetRgb(btn1.Read() ? 1.f : 0.f, pot1.Value(), pot2.Value());
	}
}
