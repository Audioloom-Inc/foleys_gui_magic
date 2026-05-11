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

    if (property.type == SettableProperty::Action)
        return new StyleActionPropertyComponent (builder, property, node);

    if (property.type == SettableProperty::Choice)
        return new StyleChoicePropertyComponent (builder, property, node, property.menuCreationLambda);

    if (property.type == SettableProperty::MultiChoice)
        return new StyleChoicePropertyComponent (builder, property, node, property.menuCreationLambda, true);

    if (property.type == SettableProperty::Gradient)
        return new StyleGradientPropertyComponent (builder, property, node);

    if (property.type == SettableProperty::Colour)
        return new StyleColourPropertyComponent (builder, property, node);
        
    if (property.type == SettableProperty::MultiList)
    {
        if (property.targetNodes.size () > 1)
            return new StyleTextPropertyComponent (builder, property, node);

        return new MultiListPropertyComponent (node.getPropertyAsValue (property.name, nullptr), property.getDisplayName(), property.getChoicesFromLambda());
    }

    jassertfalse;
    return nullptr;
}

//==============================================================================

StylePropertyComponent::StylePropertyComponent (MagicGUIBuilder& builderToUse, SettableProperty& propertyToUse, juce::ValueTree& nodeToUse)
:
StylePropertyComponent (builderToUse, propertyToUse.name, nodeToUse)
{
    customValueFunction = propertyToUse.customValueFunction;
    displayName = propertyToUse.getDisplayName();
    
    hint = propertyToUse.hint.isNotEmpty () ? propertyToUse.hint
                                            : propertyToUse.description;

    flags = propertyToUse.flags;
    
    if (hint.isNotEmpty ())
    {
        setTooltip (hint);
        infoLabel.setTooltip (hint);
    }
    
    addChildComponent (&infoLabel);
    infoLabel.setText ("Info", juce::dontSendNotification);
    infoLabel.setFont (juce::FontOptions (12.f).withStyle ("italic"));
    infoLabel.setJustificationType (juce::Justification::centred);
    inheritFromParents = (propertyToUse.flags & SettableProperty::InheritFromParents) != 0;

    if (! propertyToUse.targetNodes.isEmpty ())
        setTargetNodesInternal (propertyToUse.targetNodes);
}

StylePropertyComponent::StylePropertyComponent (MagicGUIBuilder& builderToUse, juce::Identifier propertyToUse, juce::ValueTree& nodeToUse)
  : juce::PropertyComponent (propertyToUse.toString()),
    builder (builderToUse),
    property (propertyToUse),
    node (nodeToUse)
{
    displayName = SettableProperty::formatDisplayText (property.toString ());
    
    addAndMakeVisible (remove);

    remove.setConnectedEdges (juce::TextButton::ConnectedOnLeft | juce::TextButton::ConnectedOnRight);
    remove.onClick = [&]
    {
        customValueFunction.process (juce::var (), [&](){
            removePropertyFromTargetNodes ();
        });

        refresh ();

        removeClicked();

        if ((flags & SettableProperty::Flags::RefreshInspectorOnChange) != 0)
            builder.updateInspector (true);
    };

    juce::Array<juce::ValueTree> initialTargets;
    initialTargets.add (nodeToUse);
    setTargetNodesInternal (initialTargets);
}

StylePropertyComponent::~StylePropertyComponent()
{
    for (auto target : targetNodes)
        target.removeListener (this);
}

juce::var StylePropertyComponent::lookupValue()
{
    const auto& stylesheet = builder.getStylesheet();

    mixedValue = false;
    hasAnyExplicitValue = false;
    allNodesExplicitValue = true;
    inheritedFrom = {};

    juce::var firstValue;
    auto hasFirstValue = false;

    auto validTargets = 0;

    for (auto targetNode : targetNodes)
    {
        if (! targetNode.isValid ())
            continue;

        ++validTargets;

        const auto hasExplicitValue = targetNode.hasProperty (property);
        hasAnyExplicitValue = hasAnyExplicitValue || hasExplicitValue;
        allNodesExplicitValue = allNodesExplicitValue && hasExplicitValue;

        juce::ValueTree inherited;
        auto value = stylesheet.getStyleProperty (property, targetNode, inheritFromParents, &inherited);

        if (value.isVoid ())
            value = builder.getPropertyDefaultValue (property, targetNode.getType ());

        if (! hasFirstValue)
        {
            firstValue = value;
            inheritedFrom = inherited;
            hasFirstValue = true;
            continue;
        }

        if (! areValuesEqual (firstValue, value))
            mixedValue = true;
    }

    if (validTargets == 0)
    {
        allNodesExplicitValue = false;
        remove.setEnabled (false);
        return builder.getPropertyDefaultValue (property);
    }

    remove.setEnabled (hasAnyExplicitValue);

    if (showPropertyTooltips)
    {
        if (mixedValue)
        {
            setTooltip ("Mixed values");
        }
        else if (targetNodes.size () <= 1)
        {
            if (node == inheritedFrom)
                setTooltip ({});
            else if (inheritedFrom.isValid() == false)
                setTooltip ("default");
            else if (stylesheet.isClassNode (inheritedFrom))
                setTooltip ("Class: " + inheritedFrom.getType().toString() + " (double-click)");
            else if (stylesheet.isTypeNode (inheritedFrom))
                setTooltip ("Type: " + inheritedFrom.getType().toString() + " (double-click)");
            else if (stylesheet.isIdNode (inheritedFrom))
                setTooltip ("Node: " + inheritedFrom.getType().toString() + " (double-click)");
            else
                setTooltip (inheritedFrom.getType().toString() + " (double-click)");
        }
        else
        {
            setTooltip ({});
        }
    }

    return hasFirstValue ? firstValue : builder.getPropertyDefaultValue (property);
}

void StylePropertyComponent::paint (juce::Graphics& g)
{
    constexpr float labelWidthRatio = 0.56f;
    auto b = getLocalBounds().reduced (1).withWidth (juce::roundToInt ((float) getWidth() * labelWidthRatio));

    g.fillAll (findColour (ToolBox::backgroundColourId, true));

    auto activeLabelColour = findColour (ToolBox::textColourId, true);
    auto inactiveLabelColour = findColour (ToolBox::disabledTextColourId, true);
    auto labelColour = allNodesExplicitValue ? activeLabelColour : inactiveLabelColour;

    if (targetNodes.size () <= 1)
        labelColour = (node == inheritedFrom) ? activeLabelColour : inactiveLabelColour;

    if (auto* toggle = dynamic_cast<juce::ToggleButton*> (editor.get()))
        labelColour = toggle->getToggleState() ? activeLabelColour : inactiveLabelColour;

    g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
    g.setColour (labelColour);
    g.drawFittedText (displayName, b, juce::Justification::left, 1);
}

void StylePropertyComponent::resized()
{
    constexpr float labelWidthRatio = 0.56f;

    auto right = getLocalBounds ().reduced (1, 1);
    auto left = right.removeFromLeft (juce::roundToInt ((float) getWidth() * labelWidthRatio));
    const auto controlPaddingY = 1;
    const auto iconSize = juce::jlimit (14, 18, right.getHeight() - 2);

    remove.setBounds (right.removeFromRight (iconSize).reduced (1));
    
    if (editor)
        editor->setBounds (right.reduced (0, controlPaddingY));

    for (auto e : extraEditors)
    {
        auto extraEditorSize = iconSize;

        if ((bool) e->getProperties().getWithDefault ("canvasLargeExtraEditor", false))
            extraEditorSize = juce::roundToInt ((float) iconSize * 1.5f);

        e->setBounds (left.removeFromRight (extraEditorSize).reduced (1));
    }

    infoLabel.setBounds (left.reduced (0, controlPaddingY));
}

juce::ValueTree StylePropertyComponent::getInheritedFrom() const
{
    return inheritedFrom;
}

const juce::Array<juce::ValueTree>& StylePropertyComponent::getTargetNodes () const
{
    return targetNodes;
}

bool StylePropertyComponent::hasMixedValue () const
{
    return mixedValue;
}

const juce::String& StylePropertyComponent::getMixedValueText ()
{
    static const juce::String mixedText { "Mixed" };
    return mixedText;
}

void StylePropertyComponent::setEditor (std::unique_ptr<juce::Component> newEditor) 
{
    editor = std::move (newEditor);
}

void StylePropertyComponent::addExtraEditor (std::unique_ptr<juce::Component> newEditor) 
{
    extraEditors.add (newEditor.release());
}

void StylePropertyComponent::lookAndFeelChanged()
{
    remove.setColour (juce::TextButton::buttonColourId, findColour (ToolBox::removeButtonColourId, true));
}

bool StylePropertyComponent::updateInspectorIfNeeded (bool async) 
{
    if (flags & SettableProperty::RefreshInspectorOnChange)
    {
        if (async)
            builder.updateInspector (true);
        else
            builder.updateInspector (false);
        
        return true;
    }
    return false;
}

void StylePropertyComponent::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& changedProperty)
{
    if (builder.getUndoManager().isPerformingUndoRedo())
        return;

    if (property == changedProperty && targetNodes.contains (tree))
        refresh ();
}

void StylePropertyComponent::refresh () 
{ 
    if (isRefreshing ())
        return;
        
    juce::ScopedValueSetter<bool> flag (refreshing, true);
    update ();

    if (editor)
        editor->setVisible (! showHint ());

    infoLabel.setVisible (showHint ());    
}

void StylePropertyComponent::setPropertyOnTargetNodes (const juce::var& value)
{
    if (targetNodes.isEmpty ())
    {
        if (node.isValid ())
            node.setProperty (property, value, &builder.getUndoManager());

        return;
    }

    for (auto target : targetNodes)
        if (target.isValid ())
            target.setProperty (property, value, &builder.getUndoManager());
}

void StylePropertyComponent::removePropertyFromTargetNodes ()
{
    if (targetNodes.isEmpty ())
    {
        if (node.isValid ())
            node.removeProperty (property, &builder.getUndoManager());

        return;
    }

    for (auto target : targetNodes)
        if (target.isValid ())
            target.removeProperty (property, &builder.getUndoManager());
}

void StylePropertyComponent::setTargetNodesInternal (const juce::Array<juce::ValueTree>& nodes)
{
    for (auto target : targetNodes)
        target.removeListener (this);

    targetNodes.clearQuick ();

    for (auto target : nodes)
        if (target.isValid ())
            targetNodes.addIfNotAlreadyThere (target);

    if (targetNodes.isEmpty ())
    {
        if (node.isValid ())
            targetNodes.add (node);
    }

    if (! targetNodes.isEmpty ())
        node = targetNodes.getReference (0);

    for (auto target : targetNodes)
        target.addListener (this);
}

bool StylePropertyComponent::areValuesEqual (const juce::var& lhs, const juce::var& rhs) const
{
    if (lhs == rhs)
        return true;

    return lhs.toString() == rhs.toString();
}

} // namespace foleys
