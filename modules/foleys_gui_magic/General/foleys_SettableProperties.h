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

namespace foleys
{


/**
 A SettableProperty is a value that can be selected by the designer and will be
 set for the Component each time the ValueTree is loaded.
 */
class SettableProperty
{
public:
    enum PropertyType
    {
        Text,           /*< Plain text, e.g. for buttons */
        Number,         /*< A number, e.g. line width */
        Colour,         /*< Show the colour selector and palette names */
        Toggle,         /*< Show a toggle for bool properties */
        Choice,         /*< Shows choices provided */
        Gradient,       /*< Show a bespoke gradient editor */

        File,           /** custom styles */
        Asset,
        MultiList,
        Font,
        Draggable
    };
    
    // contrutcotr with all members below and default values for each
    SettableProperty (juce::ValueTree nodeToUse,
                      juce::Identifier nameToUse,
                      PropertyType typeToUse,
                      juce::var defaultValueToUse = {},
                      std::function<void(juce::ComboBox&)> menuCreationLambdaToUse = {})
      : node (nodeToUse),
        name (nameToUse),
        type (typeToUse),
        defaultValue (defaultValueToUse),
        menuCreationLambda (menuCreationLambdaToUse)
    {
    }

    // default copy ctor
    SettableProperty (const SettableProperty&) = default;

    juce::ValueTree                             node {};
    juce::Identifier                            name {};
    PropertyType                                type {};
    juce::var                                   defaultValue {};
    std::function<void(juce::ComboBox&)>        menuCreationLambda {};
    juce::StringArray                           allowedFileExtensions {};
    juce::String                                category {};
    juce::String                                description {};
    juce::String                                displayName {};
    int                                         customFlags {};
    juce::var                                   customInfo {};

    juce::String getDisplayName () const
    {
        if (displayName.isNotEmpty())
            return displayName;

        return name.toString();
    }

    SettableProperty withNode (juce::ValueTree newNode) const
    {
        return with (*this, &SettableProperty::node, newNode);
    }
    
    SettableProperty withName (juce::Identifier newName) const
    {
        return with (*this, &SettableProperty::name, newName);
    }

    SettableProperty withType (PropertyType newType) const
    {
        return with (*this, &SettableProperty::type, newType);
    }

    SettableProperty withDefaultValue (juce::var newDefault) const
    {
        return with (*this, &SettableProperty::defaultValue, newDefault);
    }

    SettableProperty withMenuCreationLambda (std::function<void(juce::ComboBox&)> newLambda) const
    {
        return with (*this, &SettableProperty::menuCreationLambda, newLambda);
    }

    SettableProperty withAllowedFileExtensions (juce::StringArray newExtensions) const
    {
        return with (*this, &SettableProperty::allowedFileExtensions, newExtensions);
    }

    SettableProperty withCategory (const juce::String& newCategory) const
    {
        return with (*this, &SettableProperty::category, newCategory);
    }

    SettableProperty withDescription (const juce::String& desc)
    {
        return with (*this, &SettableProperty::description, desc);
    }

    SettableProperty withDisplayName (const juce::String& newName)
    {
        return with (*this, &SettableProperty::displayName, newName);
    }

    SettableProperty withCustomFlags (int newFlags)
    {
        return with (*this, &SettableProperty::customFlags, newFlags);
    }

    SettableProperty withCustomInfo (juce::var newInfo)
    {
        return with (*this, &SettableProperty::customInfo, newInfo);
    }

    juce::StringArray getChoicesFromLambda () const
    {
        if (menuCreationLambda)
        {
            juce::ComboBox box;

            juce::StringArray choices;
            menuCreationLambda (box);
            for (int i=0; i<box.getNumItems(); ++i)
                choices.add (box.getItemText(i));

            return choices;
        }

        return {};
    }

private:
    template <typename Member, typename Item>
    static SettableProperty with (SettableProperty property, Member&& member, Item&& item)
    {
        property.*member = std::forward<Item> (item);
        return property;
    }
};

} // namespace foleys
