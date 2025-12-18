#include "PluginEditor.h"

#include <optional>
#include <vector>
#include <cstring>

static std::vector<std::byte> bytesFromCString (const char* s)
{
    const auto n = std::strlen (s);
    std::vector<std::byte> out (n);
    std::memcpy (out.data(), s, n);
    return out;
}

AnalogExactAudioProcessorEditor::AnalogExactAudioProcessorEditor (AnalogExactAudioProcessor& p,
                                                                  juce::AudioProcessorValueTreeState& vts)
    : juce::AudioProcessorEditor (&p),
      audioProcessor (p),
      valueTreeState (vts)
{
    setSize (1100, 800);

    loadingLabel.setText ("Loading web UI…", juce::dontSendNotification);
    loadingLabel.setJustificationType (juce::Justification::centred);
    loadingLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (loadingLabel);

    // IMPORTANT: prevents Cubase crashing during editor construction (your crash stack showed setBounds during ctor)
    juce::MessageManager::callAsync ([safeThis = juce::Component::SafePointer<AnalogExactAudioProcessorEditor> (this)]
    {
        if (safeThis != nullptr)
            safeThis->createWebUI();
    });
}

AnalogExactAudioProcessorEditor::~AnalogExactAudioProcessorEditor() = default;

void AnalogExactAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void AnalogExactAudioProcessorEditor::resized()
{
    loadingLabel.setBounds (getLocalBounds());
    if (web != nullptr)
        web->setBounds (getLocalBounds());
}

void AnalogExactAudioProcessorEditor::createWebUI()
{
    if (web != nullptr)
        return;

    // PASTE YOUR FULL CodePen-export HTML HERE.
    // This raw-string avoids ALL quoting/escaping problems.
    static constexpr const char* kHtml = R"ANEX_UI(
PASTE_YOUR_FULL_HTML_HERE
)ANEX_UI";

    auto options = juce::WebBrowserComponent::Options{}
        .withKeepPageLoadedWhenBrowserIsHidden()   // avoids the “about:blank” behavior in some hosts  [oai_citation:2‡JUCE Documentation](https://docs.juce.com/master/classjuce_1_1WebBrowserComponent_1_1Options.html?utm_source=chatgpt.com)
        .withNativeIntegrationEnabled (true)
        .withResourceProvider ([] (const juce::String& path)
            -> std::optional<juce::WebBrowserComponent::Resource>
        {
            const auto p = path.trim();

            if (p == "/" || p == "/index.html")
            {
                juce::WebBrowserComponent::Resource r;
                r.mimeType = "text/html";
                r.data = bytesFromCString (kHtml);
                return r;
            }

            return std::nullopt;
        });

    // This is the correct JUCE check (not areAnyWebBrowsersAvailable)
    if (! juce::WebBrowserComponent::areOptionsSupported (options))  //  [oai_citation:3‡JUCE Documentation](https://docs.juce.com/master/classjuce_1_1WebBrowserComponent.html?utm_source=chatgpt.com)
    {
        loadingLabel.setText ("Web UI not supported in this host.", juce::dontSendNotification);
        return;
    }

    web = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (*web);

    loadingLabel.setVisible (false);
    resized();

    web->goToURL (juce::WebBrowserComponent::getResourceProviderRoot()); //  [oai_citation:4‡JUCE Documentation](https://docs.juce.com/master/classjuce_1_1WebBrowserComponent.html?utm_source=chatgpt.com)
}
