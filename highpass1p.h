#pragma once

#include <cmath>
#include "mln_core.h"

namespace mln
{
    
//----------------------------------------------------------------------------------------------------------------
// HighPass1P -- simple one pole filter - see https://www.earlevel.com/main/2012/12/15/a-one-pole-filter/ & https://www.dspguide.com/ch19/2.htm
//
class HighPass1P
{
public:
	void Init(float sampleRate)
	{
		m_rSampleRate = 1.f / sampleRate;
		m_b1 = 0.f;
		m_a1 = -1.f;
		m_a0 = 1.f;
		m_z1 = 0.f;
        m_in1 = 0.f;
	}

	float Process(float in)
	{
		m_z1 = in * m_a0 + m_in1 * m_a1 + m_z1 * m_b1;
        m_in1 = in;
		return m_z1;
	}

	void SetFreq(float cutoff)
	{
		float samplesPerHz = cutoff * m_rSampleRate;
		m_b1 = expf(-TWOPI_F * samplesPerHz);
		m_a0 = (1.f + m_b1) * 0.5f;
		m_a1 = -m_a0;
	}

private:
	float m_rSampleRate = 1.f / 48000.f;
	float m_a0 = 1.f, m_a1 = -1.f, m_b1 = 0.f;	// coefficients
	float m_z1 = 0.f;       // prev output
    float m_in1 = 0.f;      // prev input
};

}
