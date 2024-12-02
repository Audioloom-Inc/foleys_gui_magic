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
        MultiList
    };

    // contrutcotr with all members below and default values for each
    SettableProperty (juce::ValueTree nodeToUse,
                      juce::Identifier nameToUse,
                      PropertyType typeToUse,
                      juce::var defaultValueToUse = {},
                      std::function<void(juce::ComboBox&)> menuCreationLambdaToUse = {},
                      juce::StringArray allowedFileExtensionsToUse = {},
                      juce::String categoryToUse = {},
                      juce::String descriptionToUse = {},
                      juce::String displayNameToUse = {})
      : node (nodeToUse),
        name (nameToUse),
        type (typeToUse),
        defaultValue (defaultValueToUse),
        menuCreationLambda (menuCreationLambdaToUse),
        allowedFileExtensions (allowedFileExtensionsToUse),
        category (categoryToUse),
        description (descriptionToUse),
        displayName (displayNameToUse)
    {
    }

    // default copy ctor
    SettableProperty (const SettableProperty&) = default;

    juce::ValueTree                             node {};
    const juce::Identifier                      name {};
    const PropertyType                          type {};
    const juce::var                             defaultValue {};
    const std::function<void(juce::ComboBox&)>  menuCreationLambda {};
    const juce::StringArray                     allowedFileExtensions {};
    juce::String                                category {};
    juce::String                                description {};
    juce::String                                displayName {};

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
    
    SettableProperty withDisplayName (const juce::String& newName)
    {
        return with (*this, &SettableProperty::displayName, newName);
    }

    SettableProperty withDescription (const juce::String& desc)
    {
        return with (*this, &SettableProperty::description, desc);
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
