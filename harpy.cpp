#include "daisy_seed.h"
#include "daisysp.h"

#include "ks.h"
#include "mln_core.h"
#include "daisyclock.h"

#include "voicemgr.h"

#include "filterstack.h"
#include "highpass1p.h"

using DaisyHw = daisy::DaisySeed;
using daisy::GPIO;
using daisy::Pin;

constexpr bool EnableLogging = false;


DaisyHw hw;
daisy::MidiUartHandler midi;

/* NOTE! there a few a few tweaks we might need to the midi implementation....
	1. decrease sysex buffer size to 1 at top of midi.h (unless you plan to use sysex)
	2. add an "else" that resets the parser in the "case ParserSysEx:" code - otherwise any corrupt byte that starts a sysex will likely never recover
*/

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


enum class AdcInputs : u8 { PotNW, PotNE, PotSE, PotSW, Count };
daisy::AdcChannelConfig adcChannelCfgs[u8(AdcInputs::Count)];


VoiceMgr voiceMgr;
float modWheel = 0.f;

mln::FilterStack<4, mln::FilterType::LoPass> outFilter;

mln::RgbLed led1, led2;
GPIO btn1, btn2;
daisy::AnalogControl potNW, potNE, potSE, potSW;


void InitHwIO()
{
	using namespace daisy::seed;

	adcChannelCfgs[u8(AdcInputs::PotNW)].InitSingle(A0);
	adcChannelCfgs[u8(AdcInputs::PotNE)].InitSingle(A1);
	adcChannelCfgs[u8(AdcInputs::PotSE)].InitSingle(A7);
	adcChannelCfgs[u8(AdcInputs::PotSW)].InitSingle(A6);
	hw.adc.Init(adcChannelCfgs, u8(AdcInputs::Count));

	potNW.Init(hw.adc.GetPtr(u8(AdcInputs::PotNW)), hw.AudioCallbackRate());
	potNE.Init(hw.adc.GetPtr(u8(AdcInputs::PotNE)), hw.AudioCallbackRate());
	potSE.Init(hw.adc.GetPtr(u8(AdcInputs::PotSE)), hw.AudioCallbackRate());
	potSW.Init(hw.adc.GetPtr(u8(AdcInputs::PotSW)), hw.AudioCallbackRate());

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

	potNW.Process();
	potNE.Process();
	potSE.Process();
	potSW.Process();

	outFilter.SetFreq(daisysp::fmap(potNE.Value(), 5.f, 20000.f, daisysp::Mapping::EXP));

	for (size_t i = 0; i < size; i++)
	{
		float output = voiceMgr.Process(modWheel, potNW.Value(), potSW.Value());
		output = outFilter.Process(output);
		OUT_L[i] = output;
		OUT_R[i] = output;
	}
}


u8 ledCol = 0;

void HandleNoteOff(u8 chan, u8 note)
{
	voiceMgr.NoteOff(note);
}
void HandleNoteOn(u8 chan, u8 note, u8 vel)
{
	if (vel == 0)
		return HandleNoteOff(chan, note);

	++ledCol;
	if (ledCol >= 8)
		ledCol = 0;
	led1.SetRgb(u8(ledCol & 4), u8(ledCol & 2), u8(ledCol & 1));

	voiceMgr.NoteOn(note, potSE.Value(), modWheel, potNW.Value(), potSW.Value());
}

void HandleCC(u8 chan, u8 cc, u8 val)
{
	float fval = float(val) * (1.f / 127.f);
	if (cc == 1)	// modwheel
		modWheel = fval;
}

void HandleMidiMessage(const daisy::MidiEvent& msg)
{
	if constexpr (EnableLogging)
	{
		if (msg.type != daisy::ChannelPressure && msg.type != daisy::SystemRealTime)
			hw.PrintLine("midi: m=%02x, c=%02x [%d, %d]", int(msg.type), int(msg.channel), int(msg.data[0]), int(msg.data[1]));
	}

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

	if constexpr (EnableLogging)
	{
		hw.StartLog(true);
		hw.PrintLine("heyo dumbass");
	}

	hw.SetAudioBlockSize(8); // number of samples handled per callback
	hw.SetAudioSampleRate(kSampleRateConfig);

	InitHwIO();

	INITPROFILER(hw);

	auto sampleRate = hw.AudioSampleRate();
	voiceMgr.Init(sampleRate);
	outFilter.Init(sampleRate);
	

	midi.StartReceive();

	hw.StartAudio(AudioCallback);
    hw.adc.Start();

	[[maybe_unused]] u32 lastMsgTick = hw.system.GetNow();
	[[maybe_unused]] const u32 msgFreq = 1000;
	for (;;)
	{
		AUTOPROFILE(Total);

#ifdef D_ENABLE_CPU_PROFILING
		u32 now = hw.system.GetNow();
		if ((now - lastMsgTick) > msgFreq)
		{
			auto& clock = DaisyClock::Get();
			auto report = clock.BuildReport();
			clock.Reset();
			PrintReport(hw, report);

		 	lastMsgTick = now;
		}
#endif

		// tick the UART receiver
		midi.Listen();

		// handle any new midi events
		while (midi.HasEvents())
			HandleMidiMessage(midi.PopEvent());
	}
}
