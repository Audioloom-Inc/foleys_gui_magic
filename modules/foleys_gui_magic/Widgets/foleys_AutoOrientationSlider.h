/*
 ==============================================================================
    Copyright (c) 2019-2023 Foleys Finest Audio - Daniel Walz
    All rights reserved.

    **BSD 3-Clause License**

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

 ==============================================================================

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

#include <juce_gui_basics/juce_gui_basics.h>

namespace foleys
{

/**
 This is a Slider, that holds an attachment to the AudioProcessorValueTreeState
 */
class AutoOrientationSlider  : public juce::Slider
{
public:

    AutoOrientationSlider() = default;
    AutoOrientationSlider(juce::Slider::TextEntryBoxPosition textEntryBoxPosition)
      : juce::Slider ((juce::Slider::SliderStyle)0, textEntryBoxPosition)
    {
    }

    class StyleListener
    {
    public:
        virtual ~StyleListener () = default;
        virtual void sliderStyleChanged (AutoOrientationSlider&, juce::Slider::SliderStyle layout) = 0;
    };

    void addStyleListener (StyleListener* listener)
    {
        layoutListeners.add (listener);
    }

    void removeStyleListener (StyleListener* listener)
    {
        layoutListeners.remove (listener);
    }

    void setAutoOrientation (bool shouldAutoOrient)
    {
        autoOrientation = shouldAutoOrient;
        resized();
    }

    void paint (juce::Graphics& g) override
    {
        if (filmStrip.isNull() || numImages == 0)
        {
            juce::Slider::paint (g);
        }
        else
        {
            auto index = juce::roundToInt ((numImages - 1) * valueToProportionOfLength (getValue()));
            auto knobArea = getLookAndFeel().getSliderLayout(*this).sliderBounds;

            if (horizontalFilmStrip)
            {
                auto w = filmStrip.getWidth() / numImages;
                g.drawImage (filmStrip, knobArea.getX(), knobArea.getY(), knobArea.getWidth(), knobArea.getHeight(),
                             index * w, 0, w, filmStrip.getHeight());
            }
            else
            {
                auto h = filmStrip.getHeight() / numImages;
                g.drawImage (filmStrip, knobArea.getX(), knobArea.getY(), knobArea.getWidth(), knobArea.getHeight(),
                             0, index * h, filmStrip.getWidth(), h);
            }
        }
    }

    void resized() override
    {
        if (autoOrientation)
        {
            if (isHorizontal ())
                setSliderStyle (juce::Slider::LinearHorizontal, juce::sendNotification);
            else if (isVertical ())
                setSliderStyle (juce::Slider::LinearVertical, juce::sendNotification);
            else
                setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag, juce::sendNotification);
        }

        juce::Slider::resized();
    }

    void setSliderStyle (juce::Slider::SliderStyle newStyle, juce::NotificationType notification)
    {
        auto currentStyle = getSliderStyle();
        juce::Slider::setSliderStyle (newStyle);

        if (currentStyle != getSliderStyle ())
            if (notification != juce::dontSendNotification)
                layoutListeners.call (&StyleListener::sliderStyleChanged, *this, getSliderStyle ());
    }

    void setFilmStrip (juce::Image& image)
    {
        filmStrip = image;
    }

    void setNumImages (int num, bool horizontal)
    {
        numImages = num;
        horizontalFilmStrip = horizontal;
    }

    bool isHorizontal () const
    {
        if (autoOrientation)
        {
            const auto w = getWidth();
            const auto h = getHeight();

            return w > 2 * h;
        }

        switch (getSliderStyle())
        {
            case LinearHorizontal:
            case LinearBar:
            case TwoValueHorizontal:
            case ThreeValueHorizontal:
                return true;
            default:
                return false;
        }
    }

    bool isVertical () const
    {
        if (autoOrientation)
        {
            const auto w = getWidth();
            const auto h = getHeight();

            return h > 2 * w;
        }

        switch (getSliderStyle())
        {
            case LinearVertical:
            case LinearBarVertical:
            case TwoValueVertical:
            case ThreeValueVertical:
                return true;
            default:
                return false;
        }
    }

    bool isRotary () const
    {
        return ! isHorizontal() && ! isVertical();
    }

private:

    bool autoOrientation = true;

    juce::Image filmStrip;
    int         numImages = 0;
    bool        horizontalFilmStrip = false;

    juce::ListenerList<StyleListener> layoutListeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoOrientationSlider)
};

} // namespace foleys
