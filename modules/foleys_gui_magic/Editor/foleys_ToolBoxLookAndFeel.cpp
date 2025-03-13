#include "foleys_ToolBoxLookAndFeel.h"
#include "foleys_ToolBox.h"

namespace foleys
{


ToolBoxLookAndFeel::ToolBoxLookAndFeel()
{
    setColour (ToolBox::backgroundColourId, findColour (juce::ResizableWindow::backgroundColourId));
    setColour (ToolBox::outlineColourId, juce::Colours::silver);
    setColour (ToolBox::textColourId, juce::Colours::white);
    setColour (ToolBox::disabledTextColourId, juce::Colours::grey);
    setColour (ToolBox::removeButtonColourId, juce::Colours::darkred);
    setColour (ToolBox::selectedBackgroundColourId, juce::Colours::darkorange);
}

ToolBoxLookAndFeel::~ToolBoxLookAndFeel() {}

}
