/**
 * @file foleys_CustomValueFunction.h
 * @author David Hill
 * @date 2025-03-13
 */

#pragma once

namespace foleys
{

/** a function type called when a user changes this property */
struct CustomValueFunction
{
    /* the function to be called*/
    std::function<void(const juce::var& newValue)> function;
    
    /** indicates that the value should be set before this function is called */
    bool setValueBeforeCallingFunction{ true };

    /** indicates that the value should be set after this function is called */
    bool setValueAfterCallingFunction{ false };

    CustomValueFunction& operator= (const CustomValueFunction& other)
    {
        function = other.function;
        setValueBeforeCallingFunction = other.setValueBeforeCallingFunction;
        setValueAfterCallingFunction = other.setValueAfterCallingFunction;

        return *this;
    }

    CustomValueFunction& operator= (std::function<void(const juce::var& newValue)> newFunction)
    {
        function = newFunction;
        return *this;
    }

    bool isValid () const 
    {
        return function != 0;
    }
    
    /** @internal */
    void process (const juce::var& newValue, std::function<void()> setValueFunction) const
    {
        if (! isValid ())
        {
            if (setValueFunction)
                setValueFunction();

            return;
        }

        if (setValueBeforeCallingFunction && setValueFunction)
            setValueFunction();

        function (newValue);

        if (setValueAfterCallingFunction && setValueFunction)
            setValueFunction();
    }
};


}