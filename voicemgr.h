#pragma once

#include "mln_core.h"

#include "inplace_vector.h"
#include "voice.h"

#include "daisyclock.h"


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

	float Process(float param0, float param1, float param2)
	{
		AUTOPROFILE(VoiceManager);

		float out = 0.f;
		for (Voice& voice : m_voices)
			out += voice.Process(param0, param1, param2);
		return out;
	}

	void NoteOn(u8 note, float damping, float param0, float param1, float param2)
	{
		//hw.Print("vm: noteon: %u voices released, %u voices playing. ", u32(m_released.size()), u32(m_playing.size()));

		const VoiceId id = PickVoice(note);

		//hw.PrintLine("picked voice %u", u32(id));

		m_voices[id].NoteOn(note, damping, param0, param1, param2);
		m_playing.push_back(VoiceIdNotePair{id, note});

		// hw.Print("....playing: ");
		// for (const auto& idNote : m_playing)
		// 	hw.Print("%u, ", u32(idNote.first));
		// hw.PrintLine("--");
		// hw.Print("....released: ");
		// for (u8 id : m_released)
		// 	hw.Print("%u, ", u32(id));
		// hw.PrintLine("--");
	}
	void NoteOff(u8 note)
	{
		auto itFound = std::find_if(begin(m_playing), end(m_playing), [=](auto& idNote) { return idNote.second == note; });
        if (itFound != end(m_playing))
        {
			const VoiceId id = itFound->first;

			// TODO: add to fade out list to fix clicks when voice stealing

			m_voices[id].NoteOff();

            m_released.push_back(id);
            m_playing.erase(itFound);
        }
	}

private:
	using VoiceId = u8;
	static const VoiceId MaxVoices = 6;
	using VoiceIdList = inplace_vector<VoiceId, MaxVoices>;
	using VoiceIdNotePair = pair<VoiceId, u8>;
	using PlayingVoiceList = inplace_vector<VoiceIdNotePair, MaxVoices>;
	
	VoiceId PickVoice(u8 note)
	{
		if (!m_released.empty())
		{
			auto id = m_released.front();
			m_released.erase_front();
			return id;
		}

#if 0
        // see if the note is already playing and reuse that voice
		auto itFound = std::find_if(begin(m_playing), end(m_playing), [=](auto& idNote) { return idNote.second == note; });
        if (itFound != end(m_playing))
        {
            auto id = VoiceId(itFound - begin(m_playing));
            m_playing.erase(itFound);
            return id;
        }
#else
        (void)note;
#endif

		auto idNote = m_playing.front();
		m_playing.erase_front();
		return idNote.first;
	}

	Voice m_voices[MaxVoices];

	// voices in order of age - the first is the oldest in each state
	VoiceIdList m_released;
	PlayingVoiceList m_playing;
};
