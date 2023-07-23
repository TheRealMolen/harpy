#include "harm_osc.h"

namespace mln
{

//----------------------------------------------------------------------------------------------------------------

void SinusoidalOsc::Init(float freq)
{
    m_freq = freq;
    
    m_osin = 0.f;
    m_ocos = 1.f;

    float phaseIncPerSample = kTwoPi * freq * kRecipSampRate;
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
        m_sinusoids[i].Init(0.f);
    }
}

//----------------------------------------------------------------------------------------------------------------

float HarmonicOsc::Process(float decayExponent)
{
    float out = 0.f;
    float amp = 1.f;
    float scale = 0.f;

    for (uint i=0; i<kMaxHarms; ++i)
    {
        out += amp * m_sinusoids[i].Process();
        scale += amp;

        amp *= decayExponent;
    }

    if (scale > 0.01f)
        out /= scale;

    out *= 0.2f;
    return out;
}

//----------------------------------------------------------------------------------------------------------------

void HarmonicOsc::NoteOn(float fundamental, float index)
{
    float harmIndex = fmax(ceil(index * 16.f), 1.f);

    for (uint i=0; i<kMaxHarms; ++i)
    {
        float freq = fundamental * ((i * harmIndex) + 1);        
        m_sinusoids[i].Init(freq);
    }
}

//----------------------------------------------------------------------------------------------------------------

} // ns

