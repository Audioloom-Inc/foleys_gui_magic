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

#include "foleys_StylePropertyComponent.h"

namespace foleys
{

//==============================================================================

juce::PropertyComponent* StylePropertyComponent::createComponent (MagicGUIBuilder& builder, SettableProperty& property, juce::ValueTree& node)
{
    if (property.type == SettableProperty::Text)
        return new StyleTextPropertyComponent (builder, property, node);

    if (property.type == SettableProperty::Number)
        return new StyleTextPropertyComponent (builder, property, node);

    if (property.type == SettableProperty::Toggle)
        return new StyleBoolPropertyComponent (builder, property, node);

    if (property.type == SettableProperty::Choice)
        return new StyleChoicePropertyComponent (builder, property, node, property.menuCreationLambda);

    if (property.type == SettableProperty::Gradient)
        return new StyleGradientPropertyComponent (builder, property, node);

    if (property.type == SettableProperty::Colour)
        return new StyleColourPropertyComponent (builder, property, node);
        
    if (property.type == SettableProperty::MultiList)
        return new MultiListPropertyComponent (node.getPropertyAsValue (property.name, nullptr), property.name.toString(), property.getChoicesFromLambda());

    jassertfalse;
    return nullptr;
}

//==============================================================================

StylePropertyComponent::StylePropertyComponent (MagicGUIBuilder& builderToUse, SettableProperty& propertyToUse, juce::ValueTree& nodeToUse)
:
StylePropertyComponent (builderToUse, propertyToUse.name, nodeToUse)
{
    displayName = propertyToUse.getDisplayName();
}

StylePropertyComponent::StylePropertyComponent (MagicGUIBuilder& builderToUse, juce::Identifier propertyToUse, juce::ValueTree& nodeToUse)
  : juce::PropertyComponent (propertyToUse.toString()),
    builder (builderToUse),
    property (propertyToUse),
    node (nodeToUse)
{
    displayName = property.toString();
    
    addAndMakeVisible (remove);

    remove.setConnectedEdges (juce::TextButton::ConnectedOnLeft | juce::TextButton::ConnectedOnRight);
    remove.onClick = [&]
    {
        node.removeProperty (property, &builder.getUndoManager());
        internalRefresh();

        removeClicked();
    };

    node.addListener (this);
}

StylePropertyComponent::~StylePropertyComponent()
{
    node.removeListener (this);
}

juce::var StylePropertyComponent::lookupValue()
{
    const auto value = builder.getStylesheet().getStyleProperty (property, node, true, &inheritedFrom);

    const auto& s = builder.getStylesheet();

    if (node == inheritedFrom)
        setTooltip ({});
    else if (inheritedFrom.isValid() == false)
        setTooltip ("default");
    else if (s.isClassNode (inheritedFrom))
        setTooltip ("Class: " + inheritedFrom.getType().toString() + " (double-click)");
    else if (s.isTypeNode (inheritedFrom))
        setTooltip ("Type: " + inheritedFrom.getType().toString() + " (double-click)");
    else if (s.isIdNode (inheritedFrom))
        setTooltip ("Node: " + inheritedFrom.getType().toString() + " (double-click)");
    else
        setTooltip (inheritedFrom.getType().toString() + " (double-click)");

    remove.setEnabled (node == inheritedFrom);

    if (value.isVoid())
        return builder.getPropertyDefaultValue (property);

    return value;
}

void StylePropertyComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().reduced (1).withWidth (getWidth() / 2);

    g.fillAll (findColour (ToolBox::backgroundColourId, true));
    g.setColour (findColour (ToolBox::outlineColourId, true));
    g.drawHorizontalLine (0, 0.0f, static_cast<float>(getRight()));
    g.drawHorizontalLine (getBottom() - 1, 0.0f, static_cast<float>(getRight()));
    g.setColour (node == inheritedFrom ? findColour (ToolBox::textColourId, true) : findColour (ToolBox::disabledTextColourId, true));
    g.drawFittedText (displayName, b, juce::Justification::left, 1);
}

void StylePropertyComponent::resized()
{
    auto b = getLocalBounds().reduced (1).withLeft (getWidth() / 2);
    remove.setBounds (b.removeFromRight (getHeight()));
    if (editor)
        editor->setBounds (b);
}

juce::ValueTree StylePropertyComponent::getInheritedFrom() const
{
    return inheritedFrom;
}

void StylePropertyComponent::lookAndFeelChanged()
{
    remove.setColour (juce::TextButton::buttonColourId, findColour (ToolBox::removeButtonColourId, true));
}

void StylePropertyComponent::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& changedProperty)
{
    if (builder.getUndoManager().isPerformingUndoRedo())
        return;

    if (tree == node && property == changedProperty)
        internalRefresh();
}

void StylePropertyComponent::internalRefresh() 
{ 
    if (isRefreshing ())
        return;
        
    juce::ScopedValueSetter<bool> flag (refreshing, true);
    refresh();
}

} // namespace foleys
