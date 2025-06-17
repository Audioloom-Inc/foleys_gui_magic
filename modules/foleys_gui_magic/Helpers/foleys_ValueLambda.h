#pragma once

namespace foleys
{
    class ValueLambda : public juce::Value::Listener
    {
    public:
        ValueLambda (juce::Value& v) : value (v)
        {
            value.addListener (this);
        }

        ~ValueLambda() override
        {
            value.removeListener (this);
        }

        std::function<void(juce::Value&)> onValueChanged;

        void valueChanged (juce::Value& value) override { if (onValueChanged) onValueChanged (value); }
    
    private:
        juce::Value& value;
    };
}