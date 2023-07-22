#include "harm_osc.h"

namespace mln
{

//----------------------------------------------------------------------------------------------------------------

void SinusoidalOsc::Init(float freq)
{
    m_freq = freq;
    
    m_osin = 0.f;
    m_ocos = 1.f;

    float phaseIncPerSample = freq * kRecipSampRate;
    m_sinPhaseInc = sinf(phaseIncPerSample);
    m_cosPhaseInc = cosf(phaseIncPerSample);
}

//----------------------------------------------------------------------------------------------------------------

float SinusoidalOsc::Process()
{
    // use this identity to replace sinf with 4 mults
    //      sin(x+d) = sin(x) cos(d) + cos(x) sin(d)
    //      cos(x+d) = cos(x) cos(d) - sin(x) sin(d)

    float nsin = (m_osin * m_cosPhaseInc) + (m_ocos * m_sinPhaseInc);
    float ncos = (m_ocos * m_cosPhaseInc) - (m_osin * m_sinPhaseInc);

    m_osin = nsin;
    m_ocos = ncos;

    return m_osin;
}

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------

void HarmonicOsc::Init()
{
    for (uint i=0; i<kMaxHarms; ++i)
    {
        // m_freqs[i] = 0.f;
        // m_phaseIncPerSamples[i] = 0.f;
        // m_phases[i] = 0.f;
        
        m_sinusoids[i].Init(0.f);
    }
}

//----------------------------------------------------------------------------------------------------------------

float HarmonicOsc::Process(float decayExponent)
{
    // map from 0->1 to -1->1 so we can have alternating phases
    decayExponent = (decayExponent * 2.f) - 1.f;
    
    float out = 0.f;
    float amp = 1.f;

    for (uint i=0; i<kMaxHarms; ++i)
    {
        // const float oldPhase = m_phases[i];
        // float newPhase = oldPhase + m_phaseIncPerSamples[i];

        // if (newPhase >= 1.f) [[unlikely]]
        //     newPhase -= 1.f;
        // else if (newPhase <= -1.f) [[unlikely]]
        //     newPhase += 1.f;
        
        // m_phases[i] = newPhase;

        // out += amp * sinf(kTwoPi * newPhase);

        out += amp * m_sinusoids[i].Process();

        amp *= decayExponent;
    }

    out *= 0.1f;
    return out;
}

//----------------------------------------------------------------------------------------------------------------

void HarmonicOsc::NoteOn(float fundamental, float index)
{
    float harmIndex = fmax(ceil(index * 5.f), 1.f);

    for (uint i=0; i<kMaxHarms; ++i)
    {
        float freq = fundamental * ((i * harmIndex) + 1);
        // m_freqs[i] = freq;

        // float phaseInc = freq * kRecipSampRate;
        // m_phaseIncPerSamples[i] = (phaseInc < 0.5f ? phaseInc : 0.f);

        // m_phases[i] = 0.f;
        
        m_sinusoids[i].Init(freq);
    }
}

//----------------------------------------------------------------------------------------------------------------

} // ns

