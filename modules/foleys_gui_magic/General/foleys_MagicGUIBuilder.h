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

#include "../Layout/foleys_GuiItem.h"
#include "../Layout/foleys_Stylesheet.h"
#include "../State/foleys_MagicGUIState.h"
#include "../State/foleys_RadioButtonManager.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace foleys
{
#if FOLEYS_SHOW_GUI_EDITOR_PALLETTE
    class ToolBox;
#endif

    class StylePropertyComponent;

/**
 The MagicGUIBuilder is responsible to recreate the GUI from a single ValueTree.
 You can add your own factories to the builder to allow additional components.
 */
class MagicGUIBuilder
  : public juce::ChangeListener
  , public juce::ValueTree::Listener
{
public:
    MagicGUIBuilder (MagicGUIState& magicStateToUse);
    ~MagicGUIBuilder() override;

    /**
     Create a node from the description
     */
    std::unique_ptr<GuiItem> createGuiItem (const juce::ValueTree& node);

    /**
     This triggers the rebuild of the GUI with setting the parent component
     */
    void createGUI (juce::Component& parent);

    /**
     Grant access to the stylesheet to look up visual and layout properties
     */
    Stylesheet& getStylesheet();

    /**
     Grants access to the main XML, that holds all information.
     */
    juce::ValueTree& getConfigTree();

    /**
     Grants access to the GUI root node.
     */
    juce::ValueTree getGuiRootNode();

    /**
     Lookup a Component through the tree. It will return the first with that id regardless if there is another one.
     We discourage using that function, because that Component can be deleted and recreated at any time without notice.
     */
    GuiItem* findGuiItemWithId (const juce::String& name);

    /**
     Seeks recursively for a GuiItem
     */
    GuiItem* findGuiItem (const juce::ValueTree& node);

    /**
     This selects the stylesheet node and sets it to the Stylesheet.
     If no stylesheet is found, a default one is created.
     */
    void updateStylesheet();

    /**
     Recreates all components from the `<View/>` tree.
     If no div tree is found, createDefaultGUITree is called to give subclasses
     a chance to create a suitable default.
     */
    void updateComponents();

    /**
     Recalculates the layout of all components
     */
    void updateLayout (juce::Rectangle<int> bounds);

    /**
     Resolve all colours fresh, in case the palette has changed
     */
    void updateColours();

    /**
     Register a factory for Components to be available in the GUI editor. If you need a reference to the application, you can capture that in the factory lambda.
     */
    void registerFactory (juce::Identifier type, std::unique_ptr<GuiItem> (*factory) (MagicGUIBuilder& builder, const juce::ValueTree&));

    /**
     With that method you can register your custom LookAndFeel class and apply it to different components.
     */
    void registerLookAndFeel (juce::String name, std::unique_ptr<juce::LookAndFeel> lookAndFeel);

    /**
     Registers automatically the JUCE LookAndFeel classes (V1..V4 at the time of writing)
     */
    void registerJUCELookAndFeels();

    /**
     Calling this method will register the bundled JUCE factories to give already access to
     many JUCE Components.
     */
    void registerJUCEFactories();

    /**
     This method traverses the dom and checks each style, if that property was defined.

     @param name the name of the property.
     @param node is the node in the DOM. This is used for inheritance by traversing upwards.
     */
    juce::var getStyleProperty (const juce::Identifier& name, const juce::ValueTree& node) const;

    /**
     This will go through all nodes and delete the reference to a class
     */
    void removeStyleClassReferences (juce::ValueTree tree, const juce::String& name);

    /**
     This method returns the names of colours for a certain Component type
     */
    juce::StringArray getColourNames (juce::Identifier type);

    /**
     This resets the GUI to show a single empty container
     */
    void clearGUI();

    /**
     This is used to display a dialog box. It is called by the GUI editor, but in future it might be reached
     using the configured GUI.
     */
    void showOverlayDialog (std::unique_ptr<juce::Component> dialog);

    /**
     Closes a possibly open overlay dialog
     */
    void closeOverlayDialog();

    /**
     returns the names of all registered factories
     */
    juce::StringArray getFactoryNames() const;

    /**
     creates a new root item. You can override this mtethod to create your own root item.
     */
    virtual std::unique_ptr<GuiItem> createRootItem (const juce::ValueTree& node);

    /** */
    virtual std::unique_ptr<GuiItem> createContainer (const juce::ValueTree& node);

    /**  */
    virtual StylePropertyComponent * createStylePropertyComponent (SettableProperty property, juce::ValueTree node);

    std::function<void (juce::ComboBox&)> createChoicesMenuLambda (juce::StringArray choices) const;
    std::function<void (juce::ComboBox&)> createParameterMenuLambda() const;
    std::function<void (juce::ComboBox&)> createPropertiesMenuLambda() const;
    std::function<void (juce::ComboBox&)> createTriggerMenuLambda() const;

    template<typename ObjectType>
    std::function<void (juce::ComboBox&)> createObjectsMenuLambda() const
    {
        return [this] (juce::ComboBox& combo)
        {
            int index = 0;
            for (const auto& name: magicState.getObjectIDsByType<ObjectType>())
                combo.addItem (name, ++index);
        };
    }

    /*!
     * Grant access to the RadioButtonManager
     * @return the radioButtonManager
     */
    RadioButtonManager& getRadioButtonManager();

    void changeListenerCallback (juce::ChangeBroadcaster* sender) override;

    void valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged) override;

    /**
     Lookup the default value of the property
     */
    virtual juce::var getPropertyDefaultValue (juce::Identifier property) const;

    MagicGUIState& getMagicState();

    juce::UndoManager& getUndoManager();

#if FOLEYS_SHOW_GUI_EDITOR_PALLETTE
    void attachToolboxToWindow (juce::Component& window);

    ToolBox& getMagicToolBox();
#endif

    class Listener
    {
    public:
        /** This is called in edit mode, when the user selects an item */
        virtual void selectedItem (const juce::ValueTree& node) = 0;

        /** This is called when the user drops an item in edit node */
        virtual void guiItemDropped (const juce::ValueTree& node, juce::ValueTree& droppedOnto) = 0;
    };

    void addListener (Listener* listener) { listeners.add (listener); }
    void removeListener (Listener* listener) { listeners.remove (listener); }

    /**
     This method sets the GUI in edit mode, that allows to drag the components around.
     */
    void setEditMode (bool shouldEdit);
    bool isEditModeOn() const;

    void                   setSelectedNode (const juce::ValueTree& node);
    const juce::ValueTree& getSelectedNode() const;

    void draggedItemOnto (juce::ValueTree dropped, juce::ValueTree target, int index = -1);

private:
    juce::UndoManager undo;
    Stylesheet        stylesheet { *this };

    //==============================================================================

    juce::Component::SafePointer<juce::Component> parent;

    MagicGUIState& magicState;

    RadioButtonManager radioButtonManager;

    std::unique_ptr<GuiItem> root;

    std::unique_ptr<juce::Component> overlayDialog;

    std::map<juce::Identifier, std::unique_ptr<GuiItem> (*) (MagicGUIBuilder& builder, const juce::ValueTree&)> factories;

    juce::ListenerList<Listener> listeners;
    bool                         editMode = false;
    juce::ValueTree              selectedNode;

#if FOLEYS_SHOW_GUI_EDITOR_PALLETTE
    std::unique_ptr<ToolBox> magicToolBox;
#endif

    JUCE_DECLARE_WEAK_REFERENCEABLE (MagicGUIBuilder)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagicGUIBuilder)
};

#define FOLEYS_DECLARE_GUI_FACTORY(itemName)                                                                                                                   \
    static inline std::unique_ptr<GuiItem> factory (foleys::MagicGUIBuilder& builder, const juce::ValueTree& node)                                             \
    {                                                                                                                                                          \
        return std::make_unique<itemName> (builder, node);                                                                                                     \
    }


}  // namespace foleys
