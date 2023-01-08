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

	float Process(float p1)
	{
		float out = 0.f;
		for (Voice& voice : m_voices)
			out += voice.Process(p1);
		return out;
	}

	void NoteOn(u8 note, float damping)
	{
		const VoiceId id = PickVoice(note);
		m_voices[id].NoteOn(note, damping);
		m_playing.push_back(VoiceIdNotePair{id, note});
	}
	void NoteOff(u8 note)
	{
		auto itFound = std::find_if(begin(m_playing), end(m_playing), [=](auto& idNote) { return idNote.second == note; });
        if (itFound != end(m_playing))
        {
            m_released.push_back(itFound->first);
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