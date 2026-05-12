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


class StylePropertyComponent  : public juce::PropertyComponent,
                                private juce::ValueTree::Listener
{
public:
    StylePropertyComponent (MagicGUIBuilder& builder, juce::Identifier property, juce::ValueTree& node);
    StylePropertyComponent (MagicGUIBuilder& builder, SettableProperty& property, juce::ValueTree& node);
    ~StylePropertyComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    juce::ValueTree getInheritedFrom () const;
    const juce::Array<juce::ValueTree>& getTargetNodes () const;
    bool hasMixedValue () const;
    static const juce::String& getMixedValueText ();

    static juce::PropertyComponent* createComponent (MagicGUIBuilder& builder, SettableProperty& property, juce::ValueTree& node);

protected:

    juce::var lookupValue();

    MagicGUIBuilder&    builder;
    juce::Identifier    property;
    juce::String        displayName;
    juce::ValueTree     node;
    juce::Array<juce::ValueTree> targetNodes;
    juce::ValueTree     inheritedFrom;
    juce::String        hint;
    bool                inheritFromParents { false };
    int                 flags{ 0 }; // SettableProperty::Flags
    bool                mixedValue { false };
    bool                hasAnyExplicitValue { false };
    bool                allNodesExplicitValue { false };

    CustomValueFunction customValueFunction;

    std::unique_ptr<juce::Component> editor;
    juce::OwnedArray<juce::Component> extraEditors;

    juce::TextButton    remove { "X" };

    virtual void removeClicked () {}
    virtual bool showHint () const { return false; }
    virtual void update () = 0;
    
    void setEditor (std::unique_ptr<juce::Component> newEditor);
    void addExtraEditor (std::unique_ptr<juce::Component> newEditor);

    void refresh () override;

    void lookAndFeelChanged () override;
    bool isRefreshing () { return refreshing; }

    void setPropertyOnTargetNodes (const juce::var& value);
    void removePropertyFromTargetNodes ();
    void beginNewUndoTransaction (const juce::String& verb);
    
    /** @returns true if the inspector was updated, false otherwise */
    bool updateInspectorIfNeeded (bool async = true);

private:
    juce::Label infoLabel;
    bool showPropertyTooltips{ false };
    
    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged,
        const juce::Identifier& changedProperty) override;
    
    // true during call to refresh
    bool refreshing{ false };

    void setTargetNodesInternal (const juce::Array<juce::ValueTree>& nodes);
    bool areValuesEqual (const juce::var& lhs, const juce::var& rhs) const;
    bool needsSetPropertyOnTargetNodes (const juce::var& value) const;
    bool needsRemovePropertyFromTargetNodes () const;
     
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StylePropertyComponent)
};


} // namespace foleys
