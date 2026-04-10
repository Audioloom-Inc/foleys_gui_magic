/*
 ==============================================================================
    Copyright (c) 2019-2023 Foleys Finest Audio - Daniel Walz
    All rights reserved.

    **BSD 3-Cmenuuse License**

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

#include "foleys_CustomValueFunction.h"

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
        Draggable,
        Action,          /*< A button to trigger an action  */
        MultiChoice      /*< A list of choices that can be selected */
    };
     
    enum Flags
    {
        NoFlags = 0,
        NormalView = 1,
        ExpertView = 1 << 1,
        AllViews = NormalView | ExpertView,
        InheritFromParents = 1 << 2,
        RefreshInspectorOnChange = 1 << 3,
        AllFlags = ~NoFlags
    };

    // contrutcotr with all members below and default values for each
    SettableProperty (juce::ValueTree nodeToUse,
                      juce::Identifier nameToUse,
                      PropertyType typeToUse,
                      juce::var defaultValueToUse = {},
                      std::function<void(juce::ComboBox&)> menuCreationLambdaToUse = {});

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
    juce::String                                hint {};
    int                                         flags { AllViews };
    juce::var                                   command {};
    bool                                        settable{ true };
    
    /** when you need a custom function to set the property.
     * 
     *  Only implemented for some style property compoennts.
     */
    CustomValueFunction     customValueFunction {};

    /** if displayName is not empty, this will return displayName otherwise name */
    juce::String getDisplayName () const;
    static juce::String formatDisplayText (const juce::String& rawText);
    SettableProperty withNode (juce::ValueTree newNode) const;
    SettableProperty withName (juce::Identifier newName) const;
    SettableProperty withType (PropertyType newType) const;
    SettableProperty withDefaultValue (juce::var newDefault) const;
    SettableProperty withMenuCreationLambda (std::function<void(juce::ComboBox&)> newLambda) const;
    SettableProperty withAllowedFileExtensions (juce::StringArray newExtensions) const;
    SettableProperty withCategory (const juce::String& newCategory) const;
    SettableProperty withDescription (const juce::String& desc);
    SettableProperty withDisplayName (const juce::String& newName);
    SettableProperty withHint (const juce::String& newHint);
    SettableProperty withFlags (int newFlags);
    SettableProperty withAdditionalFlags (int additionalFlags);
    SettableProperty withCommand (juce::var newCommand);
    SettableProperty withCustomValueFunction (std::function<void(const juce::var& newValue)> newFunction, bool setValueBeforeCallingFunction = true, bool setValueAfterCallingFunction = false);
    SettableProperty withCustomValueFunction (CustomValueFunction newFunction);
    SettableProperty hidden () const;
    
    juce::StringArray getChoicesFromLambda () const;

    bool isAvailableInNormalView () const;
    bool isAvailableInExpertView () const;
    
private:
    template <typename Member, typename Item>       
    static SettableProperty with (SettableProperty property, Member&& member, Item&& item);
};

using SettableProperties = std::vector<SettableProperty>;

} // namespace foleys
