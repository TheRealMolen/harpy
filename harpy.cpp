#include "daisy_seed.h"
#include "daisysp.h"

#include "ks.h"
#include "mln_core.h"
#include "daisyclock.h"

#include "voicemgr.h"

using DaisyHw = daisy::DaisySeed;
using daisy::GPIO;
using daisy::Pin;


DaisyHw hw;
daisy::MidiUartHandler midi;

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
daisy::AdcChannelConfig adcChannelCfgs[u8(AdcInputs::Count)];


VoiceMgr voiceMgr;
float modWheel = 0.f;

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
	AUTOPROFILE(AudioCallback);

	pot1.Process();
	pot2.Process();

	for (size_t i = 0; i < size; i++)
	{
		float output = voiceMgr.Process(modWheel);
		OUT_L[i] = output;
		OUT_R[i] = output;
	}
}


void HandleNoteOff(u8 chan, u8 note)
{
	voiceMgr.NoteOff(note);
}
void HandleNoteOn(u8 chan, u8 note, u8 vel)
{
	if (vel == 0)
		return HandleNoteOff(chan, note);

	voiceMgr.NoteOn(note, pot2.Value());
}

void HandleCC(u8 chan, u8 cc, u8 val)
{
	float fval = float(val) * (1.f / 127.f);
	if (cc == 1)	// modwheel
		modWheel = fval;
}

void HandleMidiMessage(const daisy::MidiEvent& msg)
{
	//if (msg.type != daisy::ChannelPressure)
	//	hw.PrintLine("midi: m=%02x, c=%02x [%d, %d]", int(msg.type), int(msg.channel), int(msg.data[0]), int(msg.data[1]));

	switch (msg.type)
	{
		case daisy::NoteOn:			return HandleNoteOn(msg.channel, msg.data[0], msg.data[1]);
		case daisy::NoteOff:		return HandleNoteOff(msg.channel, msg.data[0]);
		case daisy::ControlChange:	return HandleCC(msg.channel, msg.data[0], msg.data[1]);
		default: break;
	}
}


int main(void)
{
	hw.Init();
	hw.SetLed(true);

	DaisyClock::Get().Init(hw);

    hw.StartLog(false);
    hw.PrintLine("heyo dumbass");

	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(daisy::SaiHandle::Config::SampleRate::SAI_48KHZ);

	InitHwIO();

	auto sampleRate = hw.AudioSampleRate();
	voiceMgr.Init(sampleRate);

	midi.StartReceive();

	hw.StartAudio(AudioCallback);
    hw.adc.Start();

	[[maybe_unused]] u32 lastMsgTick = hw.system.GetNow();
	[[maybe_unused]] const u32 msgFreq = 1000;
	for (;;)
	{
		AUTOPROFILE(Total);

		u32 now = hw.system.GetNow();
		if ((now - lastMsgTick) > msgFreq)
		{
			auto& clock = DaisyClock::Get();
			auto report = clock.BuildReport();
			clock.Reset();
			PrintReport(hw, report);

		 	lastMsgTick = now;
		}

		// tick the UART receiver
		midi.Listen();

		// handle any new midi events
		while (midi.HasEvents())
			HandleMidiMessage(midi.PopEvent());
	}
}
