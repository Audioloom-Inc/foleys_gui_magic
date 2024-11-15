#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace foleys
{

class ToolBoxContentComponent : public juce::Component
{
public:
    using juce::Component::Component;
    
    virtual ~ToolBoxContentComponent() = default;
    
    virtual void setSelectedNode (const juce::ValueTree& node) {}
    virtual void stateWasReloaded () {}
};

}