#pragma once

#include "mln_core.h"

#include "inplace_vector.h"
#include "voice.h"


//----------------------------------------------------------------------------------------------------------------
// VoiceMgr -- polyphonic voice manager
//
class VoiceMgr
{
public:
	void Init(float sampleRate)
	{
		for (VoiceId i=0; i<MaxVoices; ++i)
		{
			m_voices[i].Init(sampleRate);
			m_released.push_back(i);
		}
	}

	float Process()
	{
		float out = 0.f;
		for (Voice& voice : m_voices)
			out += voice.Process();
		return out;
	}

	void NoteOn(u8 note, float damping)
	{
		const VoiceId id = PickVoice();
		//hw.PrintLine("picked voice %u, nplaying=%u, nreleased=%u", u32(id), u32(m_playing.size()), u32(m_released.size()));
		m_voices[id].NoteOn(note, damping);
		m_playing.push_back(VoiceIdNotePair{id, note});

		//for (VoiceId i=0; i<MaxVoices; ++i)
		//	hw.Print("[v%d] %dHz; ", int(i), int(m_voices[i].GetFreq()));
		//hw.PrintLine("--");
	}
	void NoteOff(u8 note)
	{
		auto itFound = std::find_if(begin(m_playing), end(m_playing), [=](auto& idNote) { return idNote.second == note; });
		m_released.push_back(itFound->first);
		m_playing.erase(itFound);
	}

private:
	using VoiceId = u8;
	static const VoiceId MaxVoices = 8;
	using VoiceIdList = inplace_vector<VoiceId, MaxVoices>;
	using VoiceIdNotePair = pair<VoiceId, u8>;
	using PlayingVoiceList = inplace_vector<VoiceIdNotePair, MaxVoices>;

	VoiceId PickVoice()
	{
		if (!m_released.empty())
		{
			auto id = m_released.front();
			m_released.erase_front();
			return id;
		}

		auto idNote = m_playing.front();
		m_playing.erase_front();
		return idNote.first;
	}

	Voice m_voices[MaxVoices];

	// voices in order of age - the first is the oldest in each state
	VoiceIdList m_released;
	PlayingVoiceList m_playing;
};
