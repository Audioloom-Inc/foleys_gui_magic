/*
 ==============================================================================
    Copyright (c) 2019-2022 Foleys Finest Audio - Daniel Walz
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

#include "foleys_PropertiesEditor.h"

#include "foleys_StyleBoolPropertyComponent.h"
#include "foleys_StyleChoicePropertyComponent.h"
#include "foleys_StyleColourPropertyComponent.h"
#include "foleys_StyleGradientPropertyComponent.h"
#include "foleys_StyleTextPropertyComponent.h"
#include "foleys_MultiListPropertyComponent.h"

namespace foleys
{

PropertiesEditor::PropertiesEditor (MagicGUIBuilder& builderToEdit)
  : builder (builderToEdit),
    undo (builder.getUndoManager())
{
    addAndMakeVisible (nodeSelect);
    addAndMakeVisible (properties);
    addAndMakeVisible (newItemName);
    addAndMakeVisible (newItemButton);

    newItemButton.setConnectedEdges (juce::TextButton::ConnectedOnLeft);
    newItemButton.onClick = [&]
    {
        auto name = newItemName.getText();
        if (name.isEmpty())
            return;

        auto& stylesheet = builder.getStylesheet();
        stylesheet.addPaletteEntry (name, juce::Colours::silver, true);
        setSelectedNode (stylesheet.getCurrentPalette());
    };

    nodeSelect.onChange = [&]()
    {
        if (style.isValid() == false)
            return;

        auto index = nodeSelect.getSelectedId();
        if (index >= ComboIDs::PaletteEdit)
        {
            auto node = style.getChildWithName (IDs::palettes).getChild (index - ComboIDs::PaletteEdit);
            setSelectedNode (node);
        }
        else if (index >= ComboIDs::ClassEdit)
        {
            auto node = style.getChildWithName (IDs::classes).getChild (index - ComboIDs::ClassEdit);
            setSelectedNode (node);
        }
        else if (index >= ComboIDs::NodeEdit)
        {
            auto node = style.getChildWithName (IDs::nodes).getChild (index - ComboIDs::NodeEdit);
            setSelectedNode (node);
        }
        else if (index >= ComboIDs::TypeEdit)
        {
            auto node = style.getChildWithName (IDs::types).getChild (index - ComboIDs::TypeEdit);
            setSelectedNode (node);
        }

    };
}

void PropertiesEditor::setStyle (juce::ValueTree styleToEdit)
{
    style = styleToEdit;
    updatePopupMenu();

    style.addListener (this);
}

void PropertiesEditor::setSelectedNode (const juce::ValueTree& node)
{
    const auto openness = properties.getOpennessState();

    styleItem = node;
    updatePopupMenu();

    const auto& stylesheet = builder.getStylesheet();

    properties.clear();

    if (stylesheet.isColourPaletteNode (styleItem))
    {
        addPaletteColours();
        return;
    }

    if (styleItem.isValid() == false)
    {
        nodeSelect.setText (TRANS ("Nothing selected"));
        return;
    }

    // clear categories, add all properties and create sections
    categories.clear ();
    setupProperties ();
    finishPropertySetup ();

    updateNodeSelect ();

    properties.restoreOpennessState (*openness);
}

void PropertiesEditor::stateWasReloaded() 
{
    setStyle (getMagicBuilder ().getStylesheet().getCurrentStyle());
}

juce::ValueTree& PropertiesEditor::getNodeToEdit()
{
    return styleItem;
}

//==============================================================================

void PropertiesEditor::createNewClass()
{
    static juce::String editorID { "styleClass" };
#if JUCE_VERSION > 0x60008
    auto iconType = juce::MessageBoxIconType::QuestionIcon;
#else
    auto iconType = juce::AlertWindow::QuestionIcon;
#endif

    classNameInput = std::make_unique<juce::AlertWindow> (TRANS ("New style class"),
                                                          TRANS ("Enter a name:"),
                                                          iconType,
                                                          this);
    classNameInput->addTextEditor (editorID, "class");
    classNameInput->addButton (TRANS ("Cancel"), 0);
    classNameInput->addButton (TRANS ("Ok"), 1);
    classNameInput->centreAroundComponent (getTopLevelComponent(), 350, 200);
    classNameInput->enterModalState (true,
                                     juce::ModalCallbackFunction::create ([this] (int result)
    {
        if (result > 0)
        {
            if (auto* editor = classNameInput->getTextEditor (editorID))
            {
                auto name = editor->getText().replaceCharacters (".&$@ ", "---__");
                auto newNode = builder.getStylesheet().addNewStyleClass (name, &undo);
                auto index = newNode.getParent().indexOf (newNode);
                updatePopupMenu();
                nodeSelect.setSelectedId (ComboIDs::ClassEdit + index);
            }
        }

        classNameInput.reset();
    }));
}

void PropertiesEditor::deleteClass (const juce::String& name)
{
    auto& stylesheet = builder.getStylesheet();
    stylesheet.deleteStyleClass (name, &undo);
    builder.removeStyleClassReferences (builder.getGuiRootNode(), name);
    updatePopupMenu();
}

//==============================================================================

void PropertiesEditor::setupProperties ()
{
    auto& stylesheet = builder.getStylesheet ();
    
    if (stylesheet.isClassNode (styleItem))
        addProperties (createClassProperties ());
    else
        addProperties (createNodeProperties ());

    addProperties (createFlexItemProperties());

    if (stylesheet.isClassNode (styleItem))
    {
        for (auto factoryName : builder.getFactoryNames())
            addProperties (createTypeProperties (juce::ValueTree (factoryName)), factoryName);
    }
    else
    {
        addProperties (createTypeProperties (styleItem));
    }

    if (styleItem.getType() == IDs::view || stylesheet.isClassNode (styleItem))
        addProperties (createContainerProperties ());
}

void PropertiesEditor::addNodeProperties()
{
    const auto& stylesheet = builder.getStylesheet();

    if (stylesheet.isTypeNode (styleItem) ||
        stylesheet.isIdNode (styleItem))
        return;

    juce::Array<juce::PropertyComponent*> array;

    for (auto p : createNodeProperties ("Node"))
        array.add (builder.createStylePropertyComponent (p, styleItem));

    addSection ("Node", array);
}

void PropertiesEditor::addClassProperties() 
{
    juce::Array<juce::PropertyComponent*> array;

    for (auto p : createClassProperties ("Class"))
        array.add (builder.createStylePropertyComponent (p, styleItem));
        
    addSection ("Class", array);
}

void PropertiesEditor::addDecoratorProperties()
{
    juce::Array<juce::PropertyComponent*> array;
    
    for (auto p : createDecoratorProperties ())
        array.add (builder.createStylePropertyComponent (p, styleItem));

    addSection ("Decorator", array);
}

void PropertiesEditor::addTypeProperties (juce::Identifier type, juce::Array<juce::PropertyComponent*> additional)
{
    juce::Array<juce::PropertyComponent*> array;

    array.addArray (additional);

    for (auto p : createTypeProperties (juce::ValueTree (type)))
        if (auto comp = builder.createStylePropertyComponent (p, styleItem))
            array.add (comp);

    addSection (type.toString(), array);
}

std::vector<foleys::SettableProperty> PropertiesEditor::createTypeProperties (juce::ValueTree node)
{
    std::vector<SettableProperty> properties;

    foleys::GuiItem* realItem = builder.findGuiItem (node); // the actual item in the hierarchy
    std::unique_ptr<GuiItem> templateItem; // the template item to use when none is found

    foleys::GuiItem* item = realItem ? realItem : (templateItem = builder.createGuiItem (node)).get();
    
    if (item)
    {
        auto props = item->getSettableProperties();
        for (auto& other : props)
        {
            other.node = styleItem;   

            if (other.category.isEmpty())
                other.category = node.getType ().toString();

            properties.push_back (other);
        }

        for (auto colour : item->getColourNames ())
        {
            properties.push_back ({ styleItem, colour, SettableProperty::PropertyType::Colour, {}, {}, {}, "Colours" });
        }
    }

    return properties;
}

std::vector<SettableProperty> PropertiesEditor::createDecoratorProperties(const juce::String& category)
{
    std::vector<SettableProperty> array;
    
    auto toMenuLambda = [&](const juce::StringArray& names){
        return [names](juce::ComboBox& box){
            box.addItemList (names, 1);
        };
    };

    array.push_back ( { styleItem, IDs::visibility, SettableProperty::Choice, {}, builder.createPropertiesMenuLambda(), {}, category });
    array.push_back ( { styleItem, IDs::caption, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::captionSize, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::captionColour, SettableProperty::Colour, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::captionPlacement, SettableProperty::Choice, {}, toMenuLambda (getAllKeyNames (makeJustificationsChoices ())), {}, category } );
    array.push_back ( { styleItem, IDs::tooltip, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::accessibilityTitle, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::accessibility, SettableProperty::Toggle, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::accessibilityDescription, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::accessibilityHelpText, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::accessibilityFocusOrder, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::margin, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::padding, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::border, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::radius, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::borderColour, SettableProperty::Colour, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::backgroundColour, SettableProperty::Colour, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::tabCaption, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::tabColour, SettableProperty::Colour, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::lookAndFeel, SettableProperty::Choice, {}, toMenuLambda (builder.getStylesheet().getLookAndFeelNames()), {}, category } );
    array.push_back ( { styleItem, IDs::backgroundImage, SettableProperty::Choice, {}, toMenuLambda (Resources::getResourceFileNames()), {}, category } );
    array.push_back ( { styleItem, IDs::imagePlacement, SettableProperty::Choice, {}, toMenuLambda ({IDs::imageCentred, IDs::imageFill, IDs::imageStretch }), {}, category } );
    array.push_back ( { styleItem, IDs::backgroundAlpha, SettableProperty::Text, {}, {}, {}, category } );
    array.push_back ( { styleItem, IDs::backgroundGradient, SettableProperty::Gradient , {}, {}, {}, category } );

    return array;
}

std::vector<SettableProperty> PropertiesEditor::createFlexItemProperties(const juce::String& category)
{
    std::vector<SettableProperty> properties;

    properties.push_back ({ styleItem, IDs::posX, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::posY, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::posWidth, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::posHeight, SettableProperty::Number, {}, {}, {}, category });

    properties.push_back ({ styleItem, IDs::width, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::height, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::minWidth, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::minHeight, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::maxWidth, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::maxHeight, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::flexGrow, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::flexShrink, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::flexOrder, SettableProperty::Number, {}, {}, {}, category });
    properties.push_back ({ styleItem, IDs::flexAlignSelf, SettableProperty::Choice, {}, builder.createChoicesMenuLambda ({ IDs::flexStretch, IDs::flexStart, IDs::flexEnd, IDs::flexCenter, IDs::flexAuto }), {}, category });

    return properties;
}

std::vector<SettableProperty> PropertiesEditor::createContainerProperties (const juce::String& categoryName)
{
    std::vector<SettableProperty> properties;

    properties.push_back ({ styleItem, IDs::display, SettableProperty::Choice, {}, builder.createChoicesMenuLambda ({ IDs::contents, IDs::flexbox, IDs::tabbed }), {}, categoryName });
    properties.push_back ({ styleItem, IDs::repaintHz, SettableProperty::Number, {}, {}, {}, categoryName });
    properties.push_back ({ styleItem, IDs::scrollMode, SettableProperty::Choice, {}, builder.createChoicesMenuLambda ({ IDs::noScroll, IDs::scrollHorizontal, IDs::scrollVertical, IDs::scrollBoth }), {}, categoryName });
    properties.push_back ({ styleItem, IDs::tabHeight, SettableProperty::Number, {}, {}, {}, categoryName });
    properties.push_back ({ styleItem, IDs::selectedTab, SettableProperty::Choice, {}, builder.createPropertiesMenuLambda (), {}, categoryName });

    properties.push_back ({ styleItem, IDs::flexDirection, SettableProperty::Choice, {}, builder.createChoicesMenuLambda ({ IDs::flexDirRow, IDs::flexDirRowReverse, IDs::flexDirColumn, IDs::flexDirColumnReverse }), {}, categoryName });
    properties.push_back ({ styleItem, IDs::flexWrap, SettableProperty::Choice, {}, builder.createChoicesMenuLambda ({ IDs::flexNoWrap, IDs::flexWrapNormal, IDs::flexWrapReverse }), {}, categoryName });
    properties.push_back ({ styleItem, IDs::flexAlignContent, SettableProperty::Choice, {}, builder.createChoicesMenuLambda ({ IDs::flexStretch, IDs::flexStart, IDs::flexEnd, IDs::flexCenter, IDs::flexSpaceAround, IDs::flexSpaceBetween }), {}, categoryName });
    properties.push_back ({ styleItem, IDs::flexAlignItems, SettableProperty::Choice, {}, builder.createChoicesMenuLambda ({ IDs::flexStretch, IDs::flexStart, IDs::flexEnd, IDs::flexCenter }), {}, categoryName });
    properties.push_back ({ styleItem, IDs::flexJustifyContent, SettableProperty::Choice, {}, builder.createChoicesMenuLambda ({ IDs::flexStart, IDs::flexEnd, IDs::flexCenter, IDs::flexSpaceAround, IDs::flexSpaceBetween }), {}, categoryName });

    properties.push_back ({ styleItem, IDs::focusContainerType, SettableProperty::Choice, {}, builder.createChoicesMenuLambda ({ IDs::focusNone, IDs::focusContainer, IDs::focusKeyContainer }), {}, categoryName });

    return properties;
}

std::vector<SettableProperty> PropertiesEditor::createClassProperties (const juce::String& categoryName)
{
    std::vector<SettableProperty> properties;

    auto media = styleItem.getOrCreateChildWithName (IDs::media, &undo);

    properties.push_back ({ styleItem, IDs::recursive, SettableProperty::Toggle, {}, {}, {}, categoryName });
    properties.push_back ({ styleItem, IDs::active, SettableProperty::Choice, {}, builder.createPropertiesMenuLambda(), {}, categoryName });
    properties.push_back ({ media, IDs::minWidth, SettableProperty::Text , {}, {}, {}, categoryName}); 
    properties.push_back ({ media, IDs::maxWidth, SettableProperty::Text , {}, {}, {}, categoryName}); 
    properties.push_back ({ media, IDs::minHeight, SettableProperty::Text, {}, {}, {}, categoryName });
    properties.push_back ({ media, IDs::maxHeight, SettableProperty::Text, {}, {}, {}, categoryName });

    return properties;
}

std::vector<SettableProperty> PropertiesEditor::createNodeProperties (const juce::String& categoryName)
{
        
    std::vector<SettableProperty> properties;

    properties.push_back ({ styleItem, IDs::id, SettableProperty::Text, {}, {}, {}, categoryName });

    if (styleItem == builder.getGuiRootNode())
    {
        properties.push_back ({ styleItem, IDs::resizable, SettableProperty::Toggle, {}, {}, {}, categoryName });
        properties.push_back ({ styleItem, IDs::resizeCorner, SettableProperty::Toggle, {}, {}, {}, categoryName });
        properties.push_back ({ styleItem, IDs::width, SettableProperty::Number, {}, {}, {}, categoryName });
        properties.push_back ({ styleItem, IDs::height, SettableProperty::Number, {}, {}, {}, categoryName });
        properties.push_back ({ styleItem, IDs::minWidth, SettableProperty::Number, {}, {}, {}, categoryName });
        properties.push_back ({ styleItem, IDs::maxWidth, SettableProperty::Number, {}, {}, {}, categoryName });
        properties.push_back ({ styleItem, IDs::minHeight, SettableProperty::Number, {}, {}, {}, categoryName });
        properties.push_back ({ styleItem, IDs::maxHeight, SettableProperty::Number, {}, {}, {}, categoryName });
        properties.push_back ({ styleItem, IDs::aspect, SettableProperty::Number, {}, {}, {}, categoryName });
        properties.push_back ({ styleItem, IDs::tooltipText, SettableProperty::Colour, {}, {}, {}, categoryName });
        properties.push_back ({ styleItem, IDs::tooltipBackground, SettableProperty::Colour, {}, {}, {}, categoryName });
        properties.push_back ({ styleItem, IDs::tooltipOutline, SettableProperty::Colour, {}, {}, {}, categoryName });
    }
    
    auto classNames = builder.getStylesheet().getAllClassesNames();
    // array.add (new MultiListPropertyComponent (styleItem.getPropertyAsValue (IDs::styleClass, &undo, true), IDs::styleClass.toString(), classNames));
    properties.push_back ({ styleItem, IDs::styleClass, SettableProperty::MultiList, {}, [classNames](juce::ComboBox& box) { box.addItemList (classNames, 1); }, {}, categoryName });

    return properties;
}

void PropertiesEditor::addFlexItemProperties()
{
    juce::Array<juce::PropertyComponent*> array;
    
    for (auto p : createFlexItemProperties ())
        array.add (builder.createStylePropertyComponent (p, styleItem));    

    addSection ("Item", array);
}

void PropertiesEditor::addContainerProperties()
{
    juce::Array<juce::PropertyComponent*> array;

    for (auto p : createContainerProperties ("Container"))
        array.add (builder.createStylePropertyComponent (p, styleItem));
        
    addSection ("Container", array);
}

void PropertiesEditor::addSection (const juce::String& name, juce::Array<juce::PropertyComponent*> propertyComponents) 
{
    properties.addSection (name, propertyComponents, getDefaultOpennessState ());
}

void PropertiesEditor::addPaletteColours()
{
    juce::Array<juce::PropertyComponent*> array;

    for (int i=0; i < styleItem.getNumProperties(); ++i)
        array.add (new StyleColourPropertyComponent (builder, styleItem.getPropertyName (i), styleItem));
}

//==============================================================================

void PropertiesEditor::updatePopupMenu()
{
    auto* popup = nodeSelect.getRootMenu();
    popup->clear();

    auto typesNode = style.getChildWithName (IDs::types);
    if (typesNode.isValid())
    {
        juce::PopupMenu menu;
        int index = ComboIDs::TypeEdit;
        for (const auto& child : typesNode)
            menu.addItem (juce::PopupMenu::Item ("Type: " + child.getType().toString()).setID (index++));

        popup->addSubMenu ("Types", menu);
    }

    auto nodesNode = style.getChildWithName (IDs::nodes);
    if (nodesNode.isValid())
    {
        int index = ComboIDs::NodeEdit;
        juce::PopupMenu menu;
        for (const auto& child : nodesNode)
            menu.addItem (juce::PopupMenu::Item ("Node: " + child.getType().toString()).setID (index++));

        popup->addSubMenu ("Nodes", menu);
    }

    auto classesNode = style.getChildWithName (IDs::classes);
    if (classesNode.isValid())
    {
        int index = ComboIDs::ClassEdit;
        juce::PopupMenu menu;
        for (const auto& child : classesNode)
            menu.addItem (juce::PopupMenu::Item ("Class: " + child.getType().toString()).setID (index++));

        menu.addSeparator();
        menu.addItem (juce::PopupMenu::Item ("New Class...")
                      .setAction ([p = juce::Component::SafePointer<PropertiesEditor>(this)]() mutable
        {
            if (p != nullptr)
                p->createNewClass();
        }));

        if (builder.getStylesheet().isClassNode (styleItem))
        {
            auto name = styleItem.getType().toString();
            menu.addItem (juce::PopupMenu::Item ("Delete Class \"" + name + "\"")
                          .setAction ([p = juce::Component::SafePointer<PropertiesEditor>(this), name]() mutable
            {
                if (p != nullptr)
                    p->deleteClass (name);
            }));
        }

        popup->addSubMenu ("Classes", menu);
    }

    auto palettesNode = style.getChildWithName (IDs::palettes);
    if (palettesNode.isValid())
    {
        int index = ComboIDs::PaletteEdit;
        juce::PopupMenu menu;
        for (const auto& child : palettesNode)
            menu.addItem (juce::PopupMenu::Item ("Palette: " + child.getType().toString()).setID (index++));

        popup->addSubMenu ("Colour Palettes", menu);
    }
}

void PropertiesEditor::paint (juce::Graphics& g)
{
    g.setColour (findColour (ToolBox::outlineColourId, true));
    g.drawRect (getLocalBounds(), 1);
}

void PropertiesEditor::resized()
{
    const auto buttonHeight = 24;
    auto bounds = getLocalBounds().reduced (1);

    nodeSelect.setBounds (bounds.removeFromTop (buttonHeight));

    auto bottom = bounds.removeFromBottom (buttonHeight);
    newItemButton.setBounds (bottom.removeFromRight (buttonHeight));
    newItemName.setBounds (bottom);

    properties.setBounds (bounds.reduced (0, 2));
}

MagicGUIBuilder& PropertiesEditor::getMagicBuilder()
{
    return builder;
}

void PropertiesEditor::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&)
{
    updatePopupMenu();
}

void PropertiesEditor::valueTreeChildRemoved (juce::ValueTree&,
                                              juce::ValueTree& childWhichHasBeenRemoved,
                                              int)
{
    if (childWhichHasBeenRemoved == styleItem)
        setSelectedNode ({});
}

void PropertiesEditor::updateNodeSelect() 
{
    auto& stylesheet = builder.getStylesheet();

    if (stylesheet.isClassNode (styleItem))
        nodeSelect.setText (TRANS ("Class: ") + styleItem.getType().toString(), juce::dontSendNotification);
    else if (stylesheet.isTypeNode (styleItem))
        nodeSelect.setText (TRANS ("Type: ") + styleItem.getType().toString(), juce::dontSendNotification);
    else if (stylesheet.isIdNode (styleItem))
        nodeSelect.setText (TRANS ("Node: ") + styleItem.getType().toString(), juce::dontSendNotification);
    else if (stylesheet.isColourPaletteNode (styleItem))
        nodeSelect.setText (TRANS ("Palette: ") + styleItem.getType().toString(), juce::dontSendNotification);
    else
        nodeSelect.setText (TRANS ("Editing node"), juce::dontSendNotification);
}

void PropertiesEditor::addProperties (std::vector<SettableProperty> props, const juce::String& parentCategory) 
{
    for (auto p : props)
        addProperty (p, parentCategory);
}

void PropertiesEditor::addProperty (const SettableProperty& property, const juce::String& parentCategory) 
{
    const auto category = property.category.isEmpty() ? "---" : property.category;
    categories.getReference (category).push_back (property);
}

void PropertiesEditor::finishPropertySetup()
{
    juce::HashMap<juce::String, std::vector<SettableProperty>>::Iterator iter (categories);
    
    juce::StringArray sorted;
    
    while (iter.next())
        sorted.add (iter.getKey());

    sorted.sort (true);

    for (auto category : sorted)
    {
        const auto& items = categories.getReference (category);
        
        juce::Array<juce::PropertyComponent*> array;

        for (auto p : items)
            if (auto comp = builder.createStylePropertyComponent (p, p.node))
                array.add (comp);

        addSection (category, array);
    }
}

bool PropertiesEditor::isClassNode() const
{
    return builder.getStylesheet().isClassNode (styleItem);
}

bool PropertiesEditor::isTypeNode() const
{
    return builder.getStylesheet().isTypeNode (styleItem);
}

bool PropertiesEditor::isIdNode() const
{
    return builder.getStylesheet().isIdNode (styleItem);
}

bool PropertiesEditor::isContainer() const
{
    return styleItem.getType() == IDs::view;
}

} // namespace foleys
