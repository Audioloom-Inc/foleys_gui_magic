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
#include "foleys_ToolBoxContent.h"

namespace foleys
{

class MagicGUIBuilder;
class ToolBoxContentComponent;

/**
 The Toolbox defines a floating window, that allows live editing of the currently loaded GUI.
 */
class ToolBox
  : public juce::Component
  , public juce::DragAndDropContainer
  , public juce::KeyListener
  , private juce::MultiTimer
  , private foleys::MagicGUIBuilder::Listener
  , private juce::AsyncUpdater
{
public:

    /** first: parent to use, second: run in window */
    using Properties = std::pair<juce::WeakReference<juce::Component>, bool>;

    /**
     Create a ToolBox floating window to edit the currently shown GUI.
     The window will float attached to the edited window.

     @param properties a pair with the window to attach to and a boolean to run toolbox in window
     @param builder is the builder instance that manages the GUI
     */
    ToolBox (const Properties& properties, MagicGUIBuilder& builder);
    ~ToolBox() override;

    enum PositionOption
    {
        left,
        right,
        detached
    };

    enum Layout
    {
        StretchableLayout = 0,
        TabbedLayout
    };

    enum ColourIds {
        backgroundColourId         = 0x90000001,
        outlineColourId            = 0x90000002,
        textColourId               = 0x90000003,
        disabledTextColourId       = 0x90000004,
        removeButtonColourId       = 0x90000005,
        selectedBackgroundColourId = 0x90000006
    };

    virtual void loadDialog();
    virtual void saveDialog();

    virtual void loadGUI (const juce::File& file);
    virtual bool saveGUI (const juce::File& file);
    
    void addContentComponent (ToolBoxContentComponent* content, const juce::String& name);
    ToolBoxContentComponent* getContentComponent (const juce::String& name) const;
    ToolBoxContentComponent* getContentComponent (int index) const;
    void removeContentComponent (ToolBoxContentComponent* content);
    void removeContentComponent (int index);
    void removeAllContentComponents ();
    int getNumContentComponents() const;

    /** updates the layout to use either tabs or a stretchable layout */
    void setLayout (const Layout& layout);
    Layout getLayout () const;

    void setSelectedNode (const juce::ValueTree& node);
    void setNodeToEdit (juce::ValueTree node);

    void setToolboxPosition (PositionOption position);

    bool keyPressed (const juce::KeyPress& key) override;
    bool keyPressed (const juce::KeyPress& key, juce::Component* originalComponent) override;
    void mouseDoubleClick (const juce::MouseEvent& event) override;
    
    void openTab (const juce::String& name);
    void setLastLocation (juce::File file);

    static juce::PropertiesFile::Options getApplicationPropertyStorage();

protected:
    enum Timers : int
    {
        WindowDrag = 1,
        AutoSave
    };

    static juce::String   positionOptionToString (PositionOption option);
    static PositionOption positionOptionFromString (const juce::String& text);

    static std::unique_ptr<juce::FileFilter> getFileFilter();

    juce::Component::SafePointer<juce::Component> parent;

    MagicGUIBuilder&            builder;
    juce::UndoManager&          undo;
    juce::ApplicationProperties appProperties;

    juce::TextButton fileMenu { TRANS ("File...") };
    juce::TextButton viewMenu { TRANS ("View...") };

    juce::TextButton undoButton { TRANS ("Undo") };

    juce::TextButton editSwitch { TRANS ("Edit") };

    PositionOption positionOption { left };

    Layout layout { StretchableLayout };

    // owns content (and resizers in stretchable layout    )
    juce::OwnedArray<Component>                         content;
    juce::OwnedArray<juce::StretchableLayoutResizerBar> resizers;
    juce::Array<Component*>                             layoutComponents;
    juce::StretchableLayoutManager                      resizeManager;

    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };

    std::unique_ptr<juce::FileBrowserComponent> fileBrowser;
    juce::File                                  lastLocation;
    juce::File                                  autoSaveFile;

    void                           updateToolboxPosition();
    juce::ResizableCornerComponent resizeCorner { this, nullptr };
    juce::ComponentDragger         componentDragger;

    bool layoutIsUpdating { false };
    void updateLayout ();

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback (int timer) override;

    void selectedItem (const juce::ValueTree& node) override;
    void guiItemDropped ([[maybe_unused]] const juce::ValueTree& node, [[maybe_unused]] juce::ValueTree& droppedOnto) override { }
    void stateWasReloaded () override;
    void editModeToggled (bool editModeOn) override;

    void handleAsyncUpdate() override;

    template<typename... MethodArgs, typename... Args>
    void call (void (ToolBoxContentComponent::*callbackFunction) (MethodArgs...), Args&&... args) const
    {
        for (auto comp : content)
            if (auto c = dynamic_cast<ToolBoxContentComponent*> (comp))
                (c->*callbackFunction) (args...);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToolBox)
};

}  // namespace foleys
