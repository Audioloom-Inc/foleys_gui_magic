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

#include "foleys_GuiItem.h"

namespace foleys
{

GuiItem::GuiItem (MagicGUIBuilder& builder, juce::ValueTree node)
  : magicBuilder (builder),
    configNode (node),
    versionAdded ((int)node.getProperty (IDs::versionAdded, 0))
{
    setOpaque (false);
    setInterceptsMouseClicks (false, true);

    configNode.addListener (this);
    magicBuilder.getStylesheet().addListener (this);

    setDraggable (isSelected ());
}

GuiItem::~GuiItem()
{
    magicBuilder.getStylesheet().removeListener (this);
}

std::vector<SettableProperty> GuiItem::getSettablePropertiesInit()
{
    juce::ScopedValueSetter<bool> setter (initializing, true);
    auto props = getSettableProperties ();

    for (auto c : colourTranslationMap)
        props.push_back (c.toSettableProperty (configNode));
    
    return props;
}

void GuiItem::setColourTranslation (std::vector<std::pair<juce::String, int>> mapping)
{
    jassertfalse; // This function is deprecated, use addColourTranslation instead
}

void GuiItem::setColourTranslation (const juce::String& identifier, const int& colourId, const juce::Colour& defaultColour, const juce::String& displayName,  const juce::String& category)
{
    colourTranslationMap.getReference (identifier) = { identifier, colourId, displayName, defaultColour, category };
    colourNames.add (identifier);
}

juce::StringArray GuiItem::getColourNames() const
{
    juce::StringArray names;

    for (const auto& pair : colourTranslationMap)
        if (revealColourToUser (pair.identifier))
            names.addIfNotAlreadyThere (pair.identifier);

    return names;
}

juce::StringArray GuiItem::getColourDisplayNames() const
{
    juce::StringArray names;
    for (auto colour : colourTranslationMap)
        if (revealColourToUser (colour.identifier))
            names.add (getColourDisplayName (colour.identifier));

    return names;
}

juce::String GuiItem::getColourDisplayName (const juce::String& colourId) const
{
    return colourTranslationMap[colourId].getDisplayName ();
}

bool GuiItem::revealColourToUser (const juce::String& colourId) const
{
    return true;
}

// juce::var GuiItem::getProperty (const juce::Identifier& property, bool inherit)
// {
//     return magicBuilder.getStyleProperty (property, configNode, inherit);
// }

juce::var GuiItem::getProperty (const juce::Identifier& property, bool inherit) const
{
    return magicBuilder.getStyleProperty (property, configNode, inherit);
}

MagicGUIState& GuiItem::getMagicState()
{
    return magicBuilder.getMagicState();
}

GuiItem* GuiItem::findGuiItemWithId (const juce::String& name)
{
    if (configNode.getProperty (IDs::id, juce::String()).toString() == name)
        return this;

    return nullptr;
}

GuiItem* GuiItem::findGuiItemOfType (const juce::Identifier& type)
{
    if (configNode.getType () == type)
        return this;

    return nullptr;
}

void GuiItem::updateInternal()
{
    auto& stylesheet = magicBuilder.getStylesheet();

    if (auto* newLookAndFeel = stylesheet.getLookAndFeel (configNode))
        setLookAndFeel (newLookAndFeel);

    decorator.configure (magicBuilder, configNode);
    configureComponent();
    configureFlexBoxItem (configNode);
    configurePosition (configNode);

    updateColours();

    update();

    setEditMode (magicBuilder.isEditModeOn());

    repaint();
}

void GuiItem::updateColours()
{
    decorator.updateColours (magicBuilder, configNode);

    auto* component = getWrappedComponent();
    if (component == nullptr)
        return;

    for (auto pair : colourTranslationMap)
    {
        auto colour = magicBuilder.getStyleProperty (pair.identifier, configNode, inheritFromParents ()).toString();
        
        if (colour.isNotEmpty())
            component->setColour (pair.colourId, magicBuilder.getStylesheet().getColour (colour));
        else if (pair.defaultColour.getAlpha () > 0.f)
            component->setColour (pair.colourId, pair.defaultColour);
        else
            component->removeColour (pair.colourId);
    }
}

void GuiItem::configureComponent()
{
    auto* component = getWrappedComponent();
    if (component == nullptr)
        return;

    component->setComponentID (configNode.getProperty (IDs::id, juce::String()).toString());

    if (auto* tooltipClient = dynamic_cast<juce::SettableTooltipClient*>(component))
    {
        auto tooltip = magicBuilder.getStyleProperty (IDs::tooltip, configNode).toString();
        if (tooltip.isNotEmpty())
            tooltipClient->setTooltip (tooltip);
    }

    component->setAccessible (magicBuilder.getStyleProperty (IDs::accessibility, configNode));
    component->setTitle (magicBuilder.getStyleProperty (IDs::accessibilityTitle, configNode));
    component->setDescription (magicBuilder.getStyleProperty (IDs::accessibilityDescription, configNode).toString());
    component->setHelpText (magicBuilder.getStyleProperty (IDs::accessibilityHelpText, configNode).toString());
    component->setExplicitFocusOrder (magicBuilder.getStyleProperty (IDs::accessibilityFocusOrder, configNode));

    visibilityListener.onValueChanged = [&](juce::Value&){
        updateVisibility ();
    };

    auto  visibilityNode = magicBuilder.getStyleProperty (IDs::visibility, configNode, true);
    if (! visibilityNode.isVoid() && visibilityNode.isString () && visibilityNode.toString ().isNotEmpty())
        visibility.referTo (magicBuilder.getMagicState().getPropertyAsValue (visibilityNode.toString()));
    else
    {
        visibility.referTo ({});
        visibility = true;
    }
}

void GuiItem::configureFlexBoxItem (const juce::ValueTree& node)
{
    flexItem = juce::FlexItem (*this).withFlex (1.0f);

    const auto minWidth = magicBuilder.getStyleProperty (IDs::minWidth, node);
    if (! minWidth.isVoid())
        flexItem.minWidth = minWidth;

    const auto maxWidth = magicBuilder.getStyleProperty (IDs::maxWidth, node);
    if (! maxWidth.isVoid())
        flexItem.maxWidth = maxWidth;

    const auto minHeight = magicBuilder.getStyleProperty (IDs::minHeight, node);
    if (! minHeight.isVoid())
        flexItem.minHeight = minHeight;

    const auto maxHeight = magicBuilder.getStyleProperty (IDs::maxHeight, node);
    if (! maxHeight.isVoid())
        flexItem.maxHeight = maxHeight;

    const auto width = magicBuilder.getStyleProperty (IDs::width, node);
    if (! width.isVoid())
        flexItem.width = width;

    const auto height = magicBuilder.getStyleProperty (IDs::height, node);
    if (! height.isVoid())
        flexItem.height = height;

    auto grow = magicBuilder.getStyleProperty (IDs::flexGrow, node);
    if (! grow.isVoid())
        flexItem.flexGrow = grow;

    const auto flexShrink = magicBuilder.getStyleProperty (IDs::flexShrink, node);
    if (! flexShrink.isVoid())
        flexItem.flexShrink = flexShrink;

    const auto flexOrder = magicBuilder.getStyleProperty (IDs::flexOrder, node);
    if (! flexOrder.isVoid())
        flexItem.order = flexOrder;

    const auto alignSelf = magicBuilder.getStyleProperty (IDs::flexAlignSelf, node).toString();
    if (alignSelf == IDs::flexStart)
        flexItem.alignSelf = juce::FlexItem::AlignSelf::flexStart;
    else if (alignSelf == IDs::flexEnd)
        flexItem.alignSelf = juce::FlexItem::AlignSelf::flexEnd;
    else if (alignSelf == IDs::flexCenter)
        flexItem.alignSelf = juce::FlexItem::AlignSelf::center;
    else if (alignSelf == IDs::flexAuto)
        flexItem.alignSelf = juce::FlexItem::AlignSelf::autoAlign;
    else
        flexItem.alignSelf = juce::FlexItem::AlignSelf::stretch;
}

void GuiItem::configurePosition (const juce::ValueTree& node)
{
    configurePosition (magicBuilder.getStyleProperty (IDs::posX, node), posX, 0.0);
    configurePosition (magicBuilder.getStyleProperty (IDs::posY, node), posY, 0.0);

    auto defaultSize = getDefaultSize ();

    configurePosition (magicBuilder.getStyleProperty (IDs::posWidth, node), posWidth, defaultSize.getWidth ());
    configurePosition (magicBuilder.getStyleProperty (IDs::posHeight, node), posHeight, defaultSize.getHeight ());
}

void GuiItem::configurePosition (const juce::var& v, Position& p, double d)
{
    if (v.isVoid())
    {
        p.absolute = magicBuilder.getPropertyDefaultValue (IDs::display) == IDs::contents;
        p.value = d;
    }
    else
    {
        auto const s = v.toString();
        p.absolute = ! s.endsWith ("%");
        p.value = s.getDoubleValue();
    }
}

juce::Rectangle<int> GuiItem::resolvePosition (juce::Rectangle<int> parent)
{
    return juce::Rectangle<int>
    (
        parent.getX() + juce::roundToInt (posX.absolute ? posX.value : posX.value * parent.getWidth() * 0.01),
        parent.getY() + juce::roundToInt (posY.absolute ? posY.value : posY.value * parent.getHeight() * 0.01),
        juce::roundToInt (posWidth.absolute ? posWidth.value : posWidth.value * parent.getWidth() * 0.01),
        juce::roundToInt (posHeight.absolute ? posHeight.value : posHeight.value * parent.getHeight() * 0.01)
    );
}

void GuiItem::paint (juce::Graphics& g)
{
    decorator.drawDecorator (g, getLocalBounds());
}

juce::Rectangle<int> GuiItem::getClientBounds() const
{
    return decorator.getClientBounds (getLocalBounds()).client;
}

void GuiItem::resized()
{
    if (borderDragger)
        borderDragger->setBounds (getLocalBounds());

    if (auto* component = getWrappedComponent())
    {
        auto b = getClientBounds();
        component->setVisible (b.getWidth() > 2 && b.getHeight() > 2);
        component->setBounds (b);
    }
}

void GuiItem::updateLayout()
{
    resized();
}

bool GuiItem::shouldBeVisible()
{
    return visibility.getValue() && (visibleInFinalProduct || isEditModeOn ());
}


LayoutType GuiItem::getParentsLayoutType() const
{
    if (auto* container = dynamic_cast<Container*>(getParentComponent()))
        return container->getLayoutMode();

    return LayoutType::Contents;
}

juce::String GuiItem::getTabCaption (const juce::String& defaultName) const
{
    return decorator.getTabCaption (defaultName);
}

juce::Colour GuiItem::getTabColour() const
{
    return decorator.getTabColour();
}

juce::Array<GuiItem::ColourTranslation> GuiItem::getColourTranslation()
{
    juce::Array<ColourTranslation> translations;
    for (auto name : colourNames)
        translations.add (colourTranslationMap[name]);

    return translations;
}

void GuiItem::valueTreePropertyChanged (juce::ValueTree& treeThatChanged, const juce::Identifier& property)
{
    // replace ongoing calls to updateInternal with single calls – WIP
    if (property == foleys::IDs::styleClass)
        init ();
    else if (property == foleys::IDs::parameter)
        updateParameterConnection (getProperty (IDs::parameter));
    else if (property == foleys::IDs::visibleInFinalProduct)
        updateVisibility ();
    else if (property == foleys::IDs::disappearing)
        updateVisibility ();
    else if (property == foleys::IDs::tooltipTextColour)
        configureComponent ();
    else
        propertyChangedInternal (property);

    // ongoing calls here ...
    if (treeThatChanged == configNode)
    {
        if (auto* parent = findParentComponentOfClass<GuiItem>())
            parent->updateInternal();
        else
            updateInternal();

        return;
    }

    auto& stylesheet = magicBuilder.getStylesheet();
    if (stylesheet.isClassNode (treeThatChanged))
    {
        auto name = treeThatChanged.getType().toString();
        auto classes = configNode.getProperty (IDs::styleClass, juce::String()).toString();
        if (classes.contains (name))
            updateInternal();
    }
}

void GuiItem::valueTreeChildAdded (juce::ValueTree& treeThatChanged, juce::ValueTree& childAdded)
{
    if (treeThatChanged == configNode)
        addSubComponent (childAdded);
}

void GuiItem::setBoundsForced (juce::Rectangle<int> rectangle)
{
    auto configure = [&] (Position& p, int d){
        p.absolute = true;
        p.value = d;
    };
    
    configure (posX, rectangle.getX ());
    configure (posY, rectangle.getY ());
    configure (posWidth, rectangle.getWidth ());
    configure (posHeight, rectangle.getHeight ());
    
    setBounds (rectangle);
    savePosition ();
}

void GuiItem::valueTreeChildRemoved (juce::ValueTree& treeThatChanged, juce::ValueTree& childRemoved, int index)
{
    if (treeThatChanged == configNode)
        removeSubComponent (childRemoved, index);
}

void GuiItem::valueTreeChildOrderChanged (juce::ValueTree& treeThatChanged, int, int)
{
    if (treeThatChanged == configNode)
        createSubComponents();
}

void GuiItem::valueTreeParentChanged (juce::ValueTree& treeThatChanged)
{
    if (treeThatChanged == configNode)
    {
        if (auto* parent = dynamic_cast<GuiItem*>(getParentComponent()))
            parent->updateInternal();
        else
            updateInternal();
    }
}

void GuiItem::enablementChanged() 
{
    updateVisibility ();
}

void GuiItem::itemDragEnter (const juce::DragAndDropTarget::SourceDetails& details)
{
    if (details.description.toString().startsWith (IDs::dragCC))
    {
        auto paramID = getControlledParameterID (details.localPosition);
        if (paramID.isNotEmpty())
            if (auto* parameter = magicBuilder.getMagicState().getParameter (paramID))
                highlight = parameter->getName (64);

        repaint();
    }
}

void GuiItem::itemDragExit (const juce::DragAndDropTarget::SourceDetails& details)
{
    juce::ignoreUnused (details);
    highlight.clear();
    repaint();
}

GuiItem* GuiItem::findGuiItem (const juce::ValueTree& node)
{
    if (node == configNode)
        return this;

    return nullptr;
}

GuiItem* GuiItem::findGuiItemWithProperty (const juce::Identifier& property, const juce::var& value)
{
    if (configNode.getProperty (property) == value)
        return this;

    return nullptr;
}

juce::Array<GuiItem*> GuiItem::findGuiItemsOfType (const juce::Identifier& type)
{
    if (configNode.getType () == type)
        return { this };
}

void GuiItem::paintOverChildren (juce::Graphics& g)
{
    if (magicBuilder.isEditModeOn() && magicBuilder.getSelectedNode() == configNode)
    {
        g.setColour (juce::Colours::orange.withAlpha (0.5f));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
    }

    if (highlight.isNotEmpty())
    {
        g.setColour (juce::Colours::red);
        g.drawFittedText (highlight, getLocalBounds(), juce::Justification::centred, 3);
    }
}

void GuiItem::setEditMode (bool shouldEdit)
{
    setInterceptsMouseClicks (shouldEdit, true);

    if (auto* component = getWrappedComponent())
        component->setInterceptsMouseClicks (!shouldEdit, !shouldEdit);

    updateVisibility ();

    editModeChanged (shouldEdit);
}

bool GuiItem::isEditModeOn () const
{
    return magicBuilder.isEditModeOn();
}

void GuiItem::init()
{
    juce::ScopedValueSetter<bool> setter (initializing, true);

    // property names
    auto propertyNames = [&]()
    {
        juce::StringArray names;
        
        for (auto p : getSettableProperties ())
            names.add (p.name.toString ());

        for (int i = 0; i < configNode.getNumProperties (); ++i)
            if (auto name = configNode.getPropertyName (i).toString (); name.isNotEmpty())
                names.addIfNotAlreadyThere (name);

        return names;
    }();

    if (auto parameter = getProperty (IDs::parameter); parameter.isString ())
        updateParameterConnection (parameter.toString ());
    
    for (auto p : propertyNames)
    {
        if (p == foleys::IDs::parameter.toString ())
            continue;
        
        propertyChangedInternal (p);
    }

    animationHelper.setCustomAnimatorsEnabled (hasCustomAnimator ());
}

void GuiItem::setDraggable (bool selected)
{
    if (selected &&
        getParentsLayoutType() == LayoutType::Contents &&
        configNode != magicBuilder.getGuiRootNode())
    {
        // toFront (false);
        borderDragger = std::make_unique<BorderDragger>(this, nullptr);
        componentDragger = std::make_unique<juce::ComponentDragger>();

        borderDragger->onDragStart = [&]
        {
            magicBuilder.getUndoManager().beginNewTransaction ("Drag component position");
        };
        borderDragger->onDragging = [&]
        {
            customResizeOperation (borderDragger->getDeltaBounds ());
            // triggerAsyncUpdate ();
        };
        borderDragger->onDragEnd = [&]
        {
            triggerAsyncUpdate ();
        };

        borderDragger->setBounds (getLocalBounds());
        addAndMakeVisible (*borderDragger);
    }
    else
    {
        borderDragger.reset();
        componentDragger.reset();
    }
}

void GuiItem::savePosition ()
{
    // this way we can prevent so many unnecessary calls to update layout without breaking undo redo
    magicBuilder.beginSavePosition (this);
    
    auto findContainer = [&](){
        
        for (auto* p = findParentComponentOfClass<GuiItem> (); p != nullptr; p = p->findParentComponentOfClass<GuiItem> ())
            if (p->isContainer ())
                return p;
        
        return static_cast<GuiItem*> (nullptr);
    };

    auto* undo = &magicBuilder.getUndoManager();
    auto container = findContainer ();


    if (container == nullptr)
    {
        // its the root node

        auto pw = juce::String (getWidth());
        auto ph = juce::String (getHeight());
        
        configNode.setProperty (IDs::posX, 0, undo);
        configNode.setProperty (IDs::posY, 0, undo);
        configNode.setProperty (IDs::posWidth, pw, undo);
        configNode.setProperty (IDs::posHeight, ph, undo);
    }
    else
    {
        auto parent = container->getClientBounds();

        auto px = posX.absolute ? juce::String (getX() - parent.getX()) : juce::String (100.0 * (getX() - parent.getX()) / parent.getWidth()) + "%";
        auto py = posY.absolute ? juce::String (getY() - parent.getY()) : juce::String (100.0 * (getY() - parent.getY()) / parent.getHeight()) + "%";
        auto pw = posWidth.absolute ? juce::String (getWidth()) : juce::String (100.0 * getWidth() / parent.getWidth()) + "%";
        auto ph = posHeight.absolute ? juce::String (getHeight()) : juce::String (100.0 * getHeight() / parent.getHeight()) + "%";

        configNode.setProperty (IDs::posX, px, undo);
        configNode.setProperty (IDs::posY, py, undo);
        configNode.setProperty (IDs::posWidth, pw, undo);
        configNode.setProperty (IDs::posHeight, ph, undo);
    }

    // this way we can prevent so many unnecessary calls to update layout without breaking undo redo
    magicBuilder.endSavePosition (this);
}

void GuiItem::handleAsyncUpdate ()
{
    savePosition ();
}

juce::AnimatorSetBuilder GuiItem::createAnimatorSetBuilder (bool on)
{
    return juce::AnimatorSetBuilder (createAnimatorBuilder (on).build () );
}

void GuiItem::updateVisibility()
{
    visibleInFinalProduct = (bool)getProperty (IDs::visibleInFinalProduct);
    
    const auto disappearingSet = (bool)getProperty (IDs::disappearing);
    const auto hiddenInFinalProduct = ! visibleInFinalProduct;
    const auto hideNow = hiddenInFinalProduct && ! isEditModeOn ();
    const auto visible = (bool)visibility.getValue() && (! hideNow);
    const auto disappearingEnabled = isEnabled () && visible && disappearingSet;
    
    animationHelper.setEnabled (! isEditModeOn ());

    if (isEditModeOn ())
    {
        setAlpha (visibleInFinalProduct && isEnabled () ? 1.f : 0.4f);
    }
    else
    {
        setAlpha (! visibleInFinalProduct ? 0.f : isEnabled () ? 1.f : disappearingSet ? 0.f : (float)magicBuilder.getStyleProperty (IDs::alphaWhenDisabled, configNode, true));
    }

    setVisible (visible && getAlpha () > 0.f);

    animationHelper.setDisappearingEnabled (disappearingEnabled);
}

void GuiItem::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown ())
        return;
        
    magicBuilder.setSelectedNode (configNode);

    if (componentDragger)
    {
        magicBuilder.getUndoManager().beginNewTransaction ("Drag component position");
        componentDragger->startDraggingComponent (this, event);
    }
}

void GuiItem::mouseDrag (const juce::MouseEvent& event)
{
    if (componentDragger)
    {
        if (event.mouseWasDraggedSinceMouseDown())
        {
            if (event.mods.isShiftDown ())
            {
                // prevent any calls to drag the component until next mouse up
                setDraggable (false);
    
                auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this);
                container->startDragging (getDragSourceDescription (event), this);
            }
            else
            {
                componentDragger->dragComponent (this, event, nullptr);
            }
        }
    }
}

void GuiItem::mouseUp (const juce::MouseEvent& event)
{
    triggerAsyncUpdate ();
}

bool GuiItem::isInterestedInDragSource (const juce::DragAndDropTarget::SourceDetails &)
{
    return true;
}

void GuiItem::itemDropped (const juce::DragAndDropTarget::SourceDetails &dragSourceDetails)
{
    highlight.clear();

    auto dragString = dragSourceDetails.description.toString();
    if (dragString.startsWith (IDs::dragCC))
    {
        auto number = dragString.substring (IDs::dragCC.length()).getIntValue();
        auto parameterID = getControlledParameterID (dragSourceDetails.localPosition);
        if (number > 0 && parameterID.isNotEmpty())
            if (auto* procState = dynamic_cast<MagicProcessorState*>(&magicBuilder.getMagicState()))
                procState->mapMidiController (number, parameterID);

        repaint();
        return;
    }

    const auto margin = (int) magicBuilder.getStylesheet ().getStyleProperty (IDs::margin, configNode, true);
    const auto padding = (int) magicBuilder.getStylesheet ().getStyleProperty (IDs::padding, configNode, true);
    
    const auto dropPosition = dragSourceDetails.localPosition - juce::Point<int> (margin, margin) - juce::Point<int> (padding, padding);

    if (dragSourceDetails.description == IDs::dragSelected)
    {
        auto dragged = magicBuilder.getSelectedNode();
        if (dragged.isValid() == false)
            return;

        magicBuilder.draggedItemOnto (dragged, configNode, dropPosition);
        return;
    }

    if (auto node = juce::ValueTree::fromXml (dragSourceDetails.description.toString()); node.isValid())
        return magicBuilder.draggedItemOnto (node, configNode, dropPosition);

    customItemDropAction (dragSourceDetails);
}

bool GuiItem::isSelected() const
{
    return magicBuilder.getSelectedNode() == configNode;
}

bool GuiItem::isInitializing() const
{
    return initializing;
}

bool GuiItem::isTemplate() const
{
    return ! configNode.getParent ().isValid ();
}

bool GuiItem::isTemplateOrInitializing() const
{
    return isTemplate () || isInitializing ();
}

bool GuiItem::isRoot() const
{
    return configNode == magicBuilder.getGuiRootNode();
}

juce::ValueTree GuiItem::getNode() const
{
    return configNode;
}

bool GuiItem::canBeDeleted() const
{
    return ! isRoot ();
}

//==============================================================================

//==============================================================================
//==============================================================================
GuiItem::AnimationHelper::AnimationHelper(foleys::GuiItem &item)
: item (item)
{
}

GuiItem::AnimationHelper::~AnimationHelper()
{
    juce::Desktop::getInstance ().removeGlobalMouseListener (this);
}

void GuiItem::AnimationHelper::show()
{
    fader = juce::ValueAnimatorBuilder ().withValueChangedCallback ([&, start = item.getAlpha (), end = 1.f](float value) {
        item.setAlpha (juce::jmap (value, 0.0f, 1.0f, start, end));
    }).withDurationMs (animTimeInMs).build ();
    
    animatorUpdater.addAnimator (fader);
    fader.start ();
}

void GuiItem::AnimationHelper::hide()
{
    fader = juce::ValueAnimatorBuilder ().withValueChangedCallback ([&, start = item.getAlpha (), end = 0.f](float value) {
        item.setAlpha (juce::jmap (value, 0.0f, 1.0f, start, end));
    }).withDurationMs (animTimeInMs).build ();

    animatorUpdater.addAnimator (fader);
    lastHideAnimator = fader.makeWeak ();

    startTimer (hideDelayInMs);
}

void GuiItem::AnimationHelper::createCustomAnimators(bool on)
{
    customAnimator = item.createAnimatorSetBuilder (on).build ();
    animatorUpdater.addAnimator (customAnimator);
    customAnimator.start ();
}

void GuiItem::AnimationHelper::setEnabled (bool enabled) 
{
    this->enabled = enabled;

    attachOrDetachGlobalMouseListener();
}

bool GuiItem::AnimationHelper::isEnabled() const
{
    return enabled;
}

void GuiItem::AnimationHelper::setDisappearingEnabled(bool enabled)
{
    this->disappearingEnabled = enabled;

    attachOrDetachGlobalMouseListener ();

    if (disappearingEnabled)
    {        
        if (! preventFadeout ())
            item.setAlpha (0.f);
    }
    else
    {
        fader = juce::ValueAnimatorBuilder ().build ();
    }
}

bool GuiItem::AnimationHelper::isDisappearingEnabled() const
{
    return enabled;
}

void GuiItem::AnimationHelper::setCustomAnimatorsEnabled (bool enabled) 
{
    customAnimatorsEnabled = enabled;
    attachOrDetachGlobalMouseListener ();
}

bool GuiItem::AnimationHelper::isCustomAnimatorsEnabled() const
{
    return customAnimatorsEnabled;
}

bool GuiItem::hitTest (int x, int y)
{
    if (isEditModeOn ())
        return true;

    return getClientBounds ().contains (x, y);
}

bool GuiItem::fadeInForItem (GuiItem& other)
{
    if (auto param = other.getNode ()[IDs::parameter].toString (); param.isNotEmpty ())
        return param == getNode ()[IDs::parameter].toString ();

    return false;
}

void GuiItem::AnimationHelper::attachOrDetachGlobalMouseListener() 
{
    juce::Desktop::getInstance ().removeGlobalMouseListener (this);
    
    if (enabled)
        if (disappearingEnabled || customAnimatorsEnabled)
            juce::Desktop::getInstance ().addGlobalMouseListener (this);
}

bool GuiItem::AnimationHelper::preventFadeout() const
{
    if (checkComponent (juce::Desktop::getInstance ().getMainMouseSource ().getComponentUnderMouse (), true))
        return true;

    if (item.isEditModeOn ())
        return true;

    return item.preventFadeout ();
}

void GuiItem::AnimationHelper::mouseEnter(const juce::MouseEvent &event )
{
    if (disappearingEnabled)
        if (auto c = event.originalComponent)
            if (checkComponent (c, true))
                if (! item.isParentOf (c) || item.getAlpha () > 0.f)
                    show ();

    if (customAnimatorsEnabled)
        if (checkComponent (event.eventComponent, false))
            createCustomAnimators (true);
}

void GuiItem::AnimationHelper::mouseExit(const juce::MouseEvent & event)
{
    if (disappearingEnabled)
        if (checkComponent (event.originalComponent, true))
            hide ();

    if (customAnimatorsEnabled)
        if (checkComponent (event.eventComponent, false))
            createCustomAnimators (false);
}

void GuiItem::AnimationHelper::timerCallback() 
{
    if (! preventFadeout ())
    {
        if (auto s = lastHideAnimator.lock ()) 
            s->start ();
            
        stopTimer ();
    }
}

bool GuiItem::AnimationHelper::checkComponent(Component *c, bool returnTrueForThis) const
{
    if (c != nullptr)
        if (auto parentItem = c->findParentComponentOfClass<GuiItem> ())
            if (returnTrueForThis || ! (c == &item || item.isParentOf (c)))
                return item.fadeInForItem (*parentItem);

    return false;
}



}  // namespace foleys
