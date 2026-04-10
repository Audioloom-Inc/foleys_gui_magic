#include "foleys_SettableProperties.h"

namespace foleys
{
namespace
{
    juce::String splitIdentifierWords (const juce::String& text)
    {
        juce::String result;
        const auto source = text.replaceCharacters ("_-:./", "     ");

        for (int i = 0; i < source.length (); ++i)
        {
            const auto current = source[i];
            const auto previous = (i > 0) ? source[i - 1] : juce::juce_wchar (0);
            const auto next = (i + 1 < source.length ()) ? source[i + 1] : juce::juce_wchar (0);

            const auto isCurrentUpper = juce::CharacterFunctions::isUpperCase (current);
            const auto isPreviousLower = juce::CharacterFunctions::isLowerCase (previous);
            const auto isPreviousDigit = juce::CharacterFunctions::isDigit (previous);
            const auto isPreviousUpper = juce::CharacterFunctions::isUpperCase (previous);
            const auto isNextLower = juce::CharacterFunctions::isLowerCase (next);

            const auto startsNewWord = isCurrentUpper
                                       && (isPreviousLower
                                           || isPreviousDigit
                                           || (isPreviousUpper && isNextLower))
                                       && previous != ' ';

            if (startsNewWord)
                result += ' ';

            result += current;
        }

        while (result.contains ("  "))
            result = result.replace ("  ", " ");

        return result.trim ();
    }

    bool isUppercaseWord (const juce::String& word)
    {
        bool hasLetters = false;

        for (int i = 0; i < word.length (); ++i)
        {
            const auto c = word[i];

            if (! juce::CharacterFunctions::isLetter (c))
                continue;

            hasLetters = true;

            if (! juce::CharacterFunctions::isUpperCase (c))
                return false;
        }

        return hasLetters;
    }

    juce::String toTitleCaseWord (const juce::String& word)
    {
        if (word.isEmpty () || isUppercaseWord (word))
            return word;

        juce::String result;
        bool firstLetterDone = false;

        for (int i = 0; i < word.length (); ++i)
        {
            auto c = word[i];

            if (juce::CharacterFunctions::isLetter (c))
            {
                if (! firstLetterDone)
                {
                    c = juce::CharacterFunctions::toUpperCase (c);
                    firstLetterDone = true;
                }
                else
                {
                    c = juce::CharacterFunctions::toLowerCase (c);
                }
            }

            result += c;
        }

        return result;
    }
}

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

    return formatDisplayText (name.toString ());
}

juce::String SettableProperty::formatDisplayText (const juce::String& rawText)
{
    auto text = splitIdentifierWords (rawText);
    if (text.isEmpty ())
        return text;

    juce::StringArray words;
    words.addTokens (text, " ", "");
    words.removeEmptyStrings (true);

    for (auto& word : words)
        word = toTitleCaseWord (word);

    return words.joinIntoString (" ");
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

SettableProperty SettableProperty::withHint (const juce::String& newHint)
{
    return with (*this, &SettableProperty::hint, newHint);
}

SettableProperty SettableProperty::withFlags (int newFlags)
{
    return with (*this, &SettableProperty::flags, newFlags);
}

SettableProperty SettableProperty::withAdditionalFlags (int additionalFlags)
{
    return with (*this, &SettableProperty::flags, flags | additionalFlags);
}

SettableProperty SettableProperty::withCommand (juce::var newCommand)
{
    return with (*this, &SettableProperty::command, newCommand);
}

SettableProperty SettableProperty::withCustomValueFunction (std::function<void (const juce::var& newValue)> newFunction, bool setValueBeforeCallingFunction, bool setValueAfterCallingFunction)
{
    return with (*this, &SettableProperty::customValueFunction, CustomValueFunction { newFunction, setValueBeforeCallingFunction, setValueAfterCallingFunction });
}

SettableProperty SettableProperty::withCustomValueFunction (CustomValueFunction newFunction)
{
    return with (*this, &SettableProperty::customValueFunction, newFunction);
}

SettableProperty SettableProperty::hidden () const
{
    return with (*this, &SettableProperty::flags, flags & ~SettableProperty::AllViews);
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

bool SettableProperty::isAvailableInNormalView() const
{
    return (flags & NormalView) != 0;
}

bool SettableProperty::isAvailableInExpertView() const
{
    return (flags & ExpertView) != 0;
}

template <typename Member, typename Item>
SettableProperty SettableProperty::with (SettableProperty property, Member&& member, Item&& item)
{
    property.*member = std::forward<Item> (item);
    return property;
}

} // namespace foleys
