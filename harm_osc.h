#pragma once

#include "mln_core.h"


namespace mln
{

//----------------------------------------------------------------------------------------------------------------

// fixed-frequency sinusoidal oscillator
// based on https://stackoverflow.com/a/9287072
// TODO: implement an even faster one
class SinusoidalOsc
{
public:
    void Init(float freq);

    float Process();

private:
    float m_freq;

    // the previous sin/cos output
    float m_osin;
    float m_ocos;

    // sin(phaseIncPerSample)
    float m_sinPhaseInc;
    float m_cosPhaseInc;
};

//----------------------------------------------------------------------------------------------------------------

class HarmonicOsc
{
    static const u8 kMaxHarms = 20;

public:
	void Init();

	float Process(float decayExponent);

	void NoteOn(float fundamental, float index);

private:
    // float m_freqs[kMaxHarms];
    // float m_phaseIncPerSamples[kMaxHarms];
    // float m_phases[kMaxHarms];
    
    SinusoidalOsc m_sinusoids[kMaxHarms];
};

//----------------------------------------------------------------------------------------------------------------

}

