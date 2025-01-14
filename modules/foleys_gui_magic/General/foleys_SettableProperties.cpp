#include "foleys_SettableProperties.h"

namespace foleys
{
SettableProperty::SettableProperty (juce::ValueTree nodeToUse,
                                    juce::Identifier nameToUse,
                                    PropertyType typeToUse,
                                    juce::var defaultValueToUse,
                                    std::function<void(juce::ComboBox&)> menuCreationLambdaToUse)
: node (nodeToUse)
, name (nameToUse)
, type (typeToUse)
, defaultValue (defaultValueToUse)
, menuCreationLambda (menuCreationLambdaToUse)
{
}

juce::String SettableProperty::getDisplayName () const
{
    if (displayName.isNotEmpty())
        return displayName;

    return name.toString();
}

SettableProperty SettableProperty::withNode (juce::ValueTree newNode) const
{
    return with (*this, &SettableProperty::node, newNode);
}

SettableProperty SettableProperty::withName (juce::Identifier newName) const
{
    return with (*this, &SettableProperty::name, newName);
}

SettableProperty SettableProperty::withType (PropertyType newType) const
{
    return with (*this, &SettableProperty::type, newType);
}

SettableProperty SettableProperty::withDefaultValue (juce::var newDefault) const
{
    return with (*this, &SettableProperty::defaultValue, newDefault);
}

SettableProperty SettableProperty::withMenuCreationLambda (std::function<void(juce::ComboBox&)> newLambda) const
{
    return with (*this, &SettableProperty::menuCreationLambda, newLambda);
}

SettableProperty SettableProperty::withAllowedFileExtensions (juce::StringArray newExtensions) const
{
    return with (*this, &SettableProperty::allowedFileExtensions, newExtensions);
}

SettableProperty SettableProperty::withCategory (const juce::String& newCategory) const
{
    return with (*this, &SettableProperty::category, newCategory);
}

SettableProperty SettableProperty::withDescription (const juce::String& desc)
{
    return with (*this, &SettableProperty::description, desc);
}

SettableProperty SettableProperty::withDisplayName (const juce::String& newName)
{
    return with (*this, &SettableProperty::displayName, newName);
}

SettableProperty SettableProperty::withCustomFlags (int newFlags)
{
    return with (*this, &SettableProperty::customFlags, newFlags);
}

SettableProperty SettableProperty::withCustomInfo (juce::var newInfo)
{
    return with (*this, &SettableProperty::customInfo, newInfo);
}

juce::StringArray SettableProperty::getChoicesFromLambda () const
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

template <typename Member, typename Item>
SettableProperty SettableProperty::with (SettableProperty property, Member&& member, Item&& item)
{
    property.*member = std::forward<Item> (item);
    return property;
}

} // namespace foleys