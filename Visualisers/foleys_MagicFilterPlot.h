/*
 ==============================================================================
    Copyright (c) 2019 Foleys Finest Audio Ltd. - Daniel Walz
    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
    OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
    OF THE POSSIBILITY OF SUCH DAMAGE.
 ==============================================================================
 */

#pragma once

namespace foleys
{

/**
 This will plot the frequency responce for a juce IIR filter. To use it, add it to
 the MagicPluginState. It will automatically update each time you set new coefficients
 using setIIRCoefficients.
 */
class MagicFilterPlot : public MagicPlotSource
{
public:

    MagicFilterPlot();

    /**
     Set new coefficients to calculate the frequency response from.

     @param coefficients the coefficients to calculate the frequency response for
     @param sampleRate is the sampleRate the processing is happening with
     @param maxDB is the maximum level in dB, that the curve will display
     */
    void setIIRCoefficients (juce::dsp::IIR::Coefficients<float>::Ptr coefficients, float maxDB);

    /**
     Does nothing in this class
     */
    void pushSamples (const juce::AudioBuffer<float>& buffer) override;

    /**
     This is the callback that draws the frequency plot.

     @param g the Graphics context to draw onto
     @param bounds the bounds of the plot
     @param component grants access to the plot component, e.g. to find the colours from it
     */
    void drawPlot (juce::Graphics& g, juce::Rectangle<float> bounds, MagicPlotComponent& component) override;

    void prepareToPlay (double sampleRate, int samplesPerBlockExpected) override;

private:
    juce::ReadWriteLock     plotLock;
    bool                    plotChanged = true;
    juce::Path              path;
    juce::Rectangle<float>  lastBounds;

    std::vector<double>     frequencies;
    std::vector<double>     magnitudes;
    float                   maxDB      = 100.0f;
    double                  sampleRate = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagicFilterPlot)
};

} // namespace foleys