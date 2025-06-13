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

#include "foleys_StyleChoicePropertyComponent.h"

namespace foleys
{

StyleChoicePropertyComponent::StyleChoicePropertyComponent (MagicGUIBuilder& builderToUse,
                                                            SettableProperty propertyToUse,
                                                            juce::ValueTree& nodeToUse,
                                                            juce::StringArray choicesToUse,
                                                            bool multiChoice)
: StyleChoicePropertyComponent (builderToUse, propertyToUse, nodeToUse, builderToUse.createChoicesMenuLambda (choicesToUse))
{
    initialiseComboBox (false);
}

StyleChoicePropertyComponent::StyleChoicePropertyComponent (MagicGUIBuilder& builderToUse, 
                                                            SettableProperty propertyToUse, 
                                                            juce::ValueTree& nodeToUse, 
                                                            std::function<void(juce::ComboBox&)> lambdaToUse,
                                                            bool multiChoice)
                                                              
: StylePropertyComponent (builderToUse, propertyToUse, nodeToUse)
, menuCreationLambda (lambdaToUse)
, multiChoice (multiChoice)
{
    initialiseComboBox (false);
}

std::pair<bool, int> StyleChoicePropertyComponent::getIdToSelect (juce::ComboBox& combo, const juce::String& value)
{
    auto& properties = combo.getProperties ();
    
    // directly stored selected ids
    if (properties[IDs::useSelectedItemIdInComboBoxLambda])
        return { true, value.getIntValue () };

    // stored an identifier with an identifier
    for (auto& p : properties)
        if (p.value.toString () == value)
            return { true, p.name.toString ().getTrailingIntValue () };

    return { false, 0 };
}

void StyleChoicePropertyComponent::initialiseComboBox (bool editable)
{
    // ensure that the lambda is called before the popup is shown
    class Combo : public juce::ComboBox 
    {
    public:
        Combo (std::function<void(juce::ComboBox&)> lambda, bool multiChoice) : lambda (lambda), multiChoice (multiChoice) {}
        void showPopup () override
        { 
            auto& menu = *getRootMenu ();
            
            if (lambda) 
            {
                menu.clear ();
                lambda (*this); 
            }
            
            if (! multiChoice || menu.getNumItems () <= 0)
                return juce::ComboBox::showPopup (); 

            auto& lf = getLookAndFeel ();
            menu.setLookAndFeel (&lf);
            menu.showMenuAsync (juce::PopupMenu::Options ().withTargetComponent (this).withItemThatMustBeVisible (lastClicked), [&, weakThis = juce::WeakReference (this)](int clicked){
                if (! weakThis)
                    return;

                lastClicked = clicked;

                // ensure that menuActive is false
                if (clicked == 0)
                    return hidePopup ();

                // reopen popup
                juce::MessageManager::callAsync ([weakThis] () { if (weakThis) weakThis->showPopup (); });
            });
            
            menu.setLookAndFeel (nullptr);

        }
    
    private:
        std::function<void(juce::ComboBox&)> lambda;
        bool multiChoice;
        int lastClicked = 0;

        JUCE_DECLARE_WEAK_REFERENCEABLE (Combo)
    };

    bool useLambda = choices.isEmpty ();
    auto combo = std::make_unique<Combo>(useLambda ? menuCreationLambda : 0, multiChoice);
    combo->setEditableText (editable);

    if (! useLambda)
    {
        int index = 0;
        for (const auto& name : choices)
        {
            combo->addItem (name, ++index);
        }
    }
    else if (menuCreationLambda)
    {
        menuCreationLambda (*combo);
    }

    addAndMakeVisible (combo.get());

    auto safeThis = juce::Component::SafePointer<StyleChoicePropertyComponent> (this);
    combo->onChange = [&, safeThis]
    {
        if (auto* c = dynamic_cast<juce::ComboBox*>(editor.get()))
        {
            const auto useSelectedItemId = (bool)c->getProperties ()[IDs::useSelectedItemIdInComboBoxLambda];
            juce::var value = useSelectedItemId ? juce::var (c->getSelectedId()) : juce::var (c->getText());

            if (! useSelectedItemId)
                if (auto var = c->getProperties ()[juce::String ("ID_" + juce::String (c->getSelectedId ()))]; var.isString ())
                    if (auto string = var.toString (); string.isNotEmpty ())
                        value = var;

            if (! value.isString () || value.toString ().isNotEmpty ())
                node.setProperty (property, value, &builder.getUndoManager());
        }

        if (safeThis)
        {
            refresh();

            if ((flags & SettableProperty::Flags::RefreshInspectorOnChange) != 0)
                builder.updateInspector (true);
        }
    };

    setEditor (std::move (combo));

    proxy.addListener (this);
}

void StyleChoicePropertyComponent::update()
{
    const auto value = lookupValue();

    if (auto* combo = dynamic_cast<juce::ComboBox*>(editor.get()))
    {
        if (node == inheritedFrom)
        {
            proxy.referTo (node.getPropertyAsValue (property, &builder.getUndoManager()));
        }
        else
        {
            proxy.referTo (proxy);

            auto vString = value.toString ();

            if (auto selectedId = getIdToSelect (*combo, vString); selectedId.first == true)
                combo->setSelectedId (selectedId.second);
            else
                combo->setText (vString, juce::dontSendNotification);
        }
    }

    repaint();
}

bool StyleChoicePropertyComponent::showHint() const
{
    if (auto box = dynamic_cast<juce::ComboBox*> (editor.get()))
        return box->getNumItems () <= 0;

    return false;
}

void StyleChoicePropertyComponent::valueChanged (juce::Value&)
{
    if (updating)
        return;

    juce::ScopedValueSetter<bool> updateFlag (updating, true);
    auto v = proxy.getValue().toString();

    if (auto* combo = dynamic_cast<juce::ComboBox*>(editor.get()))
    {    
        if (auto selectedId = getIdToSelect (*combo, v); selectedId.first == true)
            combo->setSelectedId (selectedId.second, juce::dontSendNotification);
        else if (combo->getText () != v)
            combo->setText (v, juce::dontSendNotification);
    }

    if (property == IDs::lookAndFeel)
    {
        // this hack is needed, since the changing will have to trigger all colours a refresh
        // to fetch fallback colours
        if (auto* panel = findParentComponentOfClass<juce::PropertyPanel>())
            panel->refreshAll();
    }
    else
    {
        refresh();
    }
}

}
