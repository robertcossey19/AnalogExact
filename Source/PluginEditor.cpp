#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalogExactAudioProcessorEditor::AnalogExactAudioProcessorEditor (
    AnalogExactAudioProcessor& p,
    juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState (vts)
{
    setSize (920, 800);
    
    // Create WebView with custom handler
    webView = std::make_unique<CustomWebView> (*this);
    addAndMakeVisible (webView.get());
    
    // Generate and load HTML
    juce::String html = generateHTML();
    webView->goToURL ("data:text/html;charset=utf-8," + juce::URL::addEscapeChars (html, false));
    
    // Start timer for updates
    startTimerHz (30);
    
    // Initial parameter sync (after a short delay to let WebView load)
    juce::Timer::callAfterDelay (500, [this]() {
        updateWebViewParameters();
    });
}

AnalogExactAudioProcessorEditor::~AnalogExactAudioProcessorEditor()
{
}

//==============================================================================
void AnalogExactAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a202c));
}

void AnalogExactAudioProcessorEditor::resized()
{
    webView->setBounds (getLocalBounds());
}

void AnalogExactAudioProcessorEditor::timerCallback()
{
    // Update VU meters
    float levelL = audioProcessor.getInputLevelL();
    float levelR = audioProcessor.getInputLevelR();
    
    float dbL = 20.0f * std::log10 (levelL + 1e-9f);
    float dbR = 20.0f * std::log10 (levelR + 1e-9f);
    
    juce::String script = juce::String::formatted (
        "if (typeof updateVUMeters === 'function') updateVUMeters(%f, %f);",
        dbL, dbR
    );
    executeJS (script);
    
    // Check for parameter changes from plugin (automation, preset load, etc.)
    updateWebViewParameters();
}

void AnalogExactAudioProcessorEditor::executeJS (const juce::String& script)
{
    if (webView != nullptr)
    {
        webView->evaluateJavascript (script);
    }
}

void AnalogExactAudioProcessorEditor::setParameterInJS (const juce::String& paramName, float value)
{
    juce::String script = juce::String::formatted (
        "if (typeof setParameter === 'function') setParameter('%s', %f);",
        paramName.toRawUTF8(), value
    );
    executeJS (script);
}

void AnalogExactAudioProcessorEditor::setParameterInJS (const juce::String& paramName, int value)
{
    juce::String script = juce::String::formatted (
        "if (typeof setParameter === 'function') setParameter('%s', %d);",
        paramName.toRawUTF8(), value
    );
    executeJS (script);
}

void AnalogExactAudioProcessorEditor::setParameterInJS (const juce::String& paramName, bool value)
{
    juce::String script = juce::String::formatted (
        "if (typeof setParameter === 'function') setParameter('%s', %s);",
        paramName.toRawUTF8(), value ? "true" : "false"
    );
    executeJS (script);
}

void AnalogExactAudioProcessorEditor::updateWebViewParameters()
{
    const float epsilon = 0.001f;
    
    // Read current parameter values
    float inputGain = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::INPUT_GAIN_ID)->load();
    float outputGain = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::OUTPUT_GAIN_ID)->load();
    float bias = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::BIAS_ID)->load();
    int monitor = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::MONITOR_ID)->load();
    int speed = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::SPEED_ID)->load();
    int flux = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::FLUX_ID)->load();
    int eq = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::EQ_ID)->load();
    int tapeType = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::TAPE_TYPE_ID)->load();
    bool transformer = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::TRANSFORMER_ID)->load() > 0.5f;
    int headblock = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::HEADBLOCK_ID)->load();
    bool autoCal = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::AUTO_CAL_ID)->load() > 0.5f;
    
    // Only send updates if values changed (using epsilon for floats)
    if (std::abs (inputGain - lastInputGain) > epsilon)
    {
        setParameterInJS ("inputGain", inputGain);
        lastInputGain = inputGain;
    }
    
    if (std::abs (outputGain - lastOutputGain) > epsilon)
    {
        setParameterInJS ("outputGain", outputGain);
        lastOutputGain = outputGain;
    }
    
    if (std::abs (bias - lastBias) > epsilon)
    {
        setParameterInJS ("bias", bias);
        lastBias = bias;
    }
    
    if (monitor != lastMonitor)
    {
        setParameterInJS ("monitor", monitor);
        lastMonitor = monitor;
    }
    
    if (speed != lastSpeed)
    {
        setParameterInJS ("speed", speed);
        lastSpeed = speed;
    }
    
    if (flux != lastFlux)
    {
        setParameterInJS ("flux", flux);
        lastFlux = flux;
    }
    
    if (eq != lastEQ)
    {
        setParameterInJS ("eq", eq);
        lastEQ = eq;
    }
    
    if (tapeType != lastTapeType)
    {
        setParameterInJS ("tapeType", tapeType);
        lastTapeType = tapeType;
    }
    
    if (transformer != lastTransformer)
    {
        setParameterInJS ("transformer", transformer);
        lastTransformer = transformer;
    }
    
    if (headblock != lastHeadblock)
    {
        setParameterInJS ("headblock", headblock);
        lastHeadblock = headblock;
    }
    
    if (autoCal != lastAutoCal)
    {
        setParameterInJS ("autoCal", autoCal);
        lastAutoCal = autoCal;
    }
}

void AnalogExactAudioProcessorEditor::handleWebViewMessage (const juce::String& url)
{
    // Parse URL like: juceplugin://parameterChanged?id=inputGain&value=5.0
    auto queryStart = url.indexOf ("?");
    if (queryStart < 0)
        return;
    
    auto queryString = url.substring (queryStart + 1);
    auto params = juce::StringArray::fromTokens (queryString, "&", "");
    
    juce::String paramId, paramValue;
    
    for (auto& param : params)
    {
        auto tokens = juce::StringArray::fromTokens (param, "=", "");
        if (tokens.size() == 2)
        {
            if (tokens[0] == "id")
                paramId = tokens[1];
            else if (tokens[0] == "value")
                paramValue = tokens[1];
        }
    }
    
    if (paramId.isEmpty())
        return;
    
    // Update the corresponding parameter
    if (paramId == "inputGain")
        valueTreeState.getParameter (AnalogExactAudioProcessor::INPUT_GAIN_ID)->setValueNotifyingHost (paramValue.getFloatValue() / 12.0f * 0.5f + 0.5f);
    else if (paramId == "outputGain")
        valueTreeState.getParameter (AnalogExactAudioProcessor::OUTPUT_GAIN_ID)->setValueNotifyingHost ((paramValue.getFloatValue() + 24.0f) / 30.0f);
    else if (paramId == "bias")
        valueTreeState.getParameter (AnalogExactAudioProcessor::BIAS_ID)->setValueNotifyingHost (paramValue.getFloatValue() / 10.0f + 0.5f);
    else if (paramId == "monitor")
        valueTreeState.getParameter (AnalogExactAudioProcessor::MONITOR_ID)->setValueNotifyingHost (paramValue.getFloatValue());
    else if (paramId == "speed")
        valueTreeState.getParameter (AnalogExactAudioProcessor::SPEED_ID)->setValueNotifyingHost (paramValue.getFloatValue() / 2.0f);
    else if (paramId == "flux")
        valueTreeState.getParameter (AnalogExactAudioProcessor::FLUX_ID)->setValueNotifyingHost (paramValue.getFloatValue() / 2.0f);
    else if (paramId == "eq")
        valueTreeState.getParameter (AnalogExactAudioProcessor::EQ_ID)->setValueNotifyingHost (paramValue.getFloatValue());
    else if (paramId == "tapeType")
        valueTreeState.getParameter (AnalogExactAudioProcessor::TAPE_TYPE_ID)->setValueNotifyingHost (paramValue.getFloatValue() / 5.0f);
    else if (paramId == "transformer")
        valueTreeState.getParameter (AnalogExactAudioProcessor::TRANSFORMER_ID)->setValueNotifyingHost (paramValue == "true" ? 1.0f : 0.0f);
    else if (paramId == "headblock")
        valueTreeState.getParameter (AnalogExactAudioProcessor::HEADBLOCK_ID)->setValueNotifyingHost (paramValue.getFloatValue());
    else if (paramId == "autoCal")
        valueTreeState.getParameter (AnalogExactAudioProcessor::AUTO_CAL_ID)->setValueNotifyingHost (paramValue == "true" ? 1.0f : 0.0f);
}

juce::String AnalogExactAudioProcessorEditor::generateHTML()
{
    // Split HTML into parts to avoid raw string literal issues with </
    juce::String html;
    
    html += "<!DOCTYPE html>\n";
    html += "<html lang=\"en\">\n";
    html += "<head>\n";
    html += "<meta charset=\"utf-8\"/>\n";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/>\n";
    html += "<title>ANALOGEXACT<";
    html += "/title>\n";
    html += "<script src=\"https://cdn.tailwindcss.com\"><";
    html += "/script>\n";
    html += "<link href=\"https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=Roboto+Mono&display=swap\" rel=\"stylesheet\">\n";
    html += "<style>\n";
    html += "html,body{height:100%;background:#1a202c;margin:0;padding:0;overflow:hidden}\n";
    html += "body{font-family:'Inter',sans-serif;color:#e5e7eb}\n";
    html += ".metal-panel{background:linear-gradient(145deg,#4a5568,#3a4454);border:1px solid #718096;border-top-color:#a0aec0}\n";
    html += ".module-bg{background:rgba(0,0,0,.2);border:1px solid rgba(0,0,0,.4);box-shadow:inset 0 2px 6px rgba(0,0,0,.4)}\n";
    html += ".knob{position:relative;width:80px;height:80px;border-radius:50%;background:linear-gradient(145deg,#5a6578,#2d3748);border:2px solid #1a202c;box-shadow:0 5px 10px rgba(0,0,0,.5),inset 0 2px 3px rgba(255,255,255,.08);display:flex;align-items:center;justify-content:center;cursor:pointer;user-select:none;touch-action:none}\n";
    html += ".knob-ind{position:absolute;width:4px;height:12px;background:#e2e8f0;top:6px;border-radius:2px;transform-origin:center 34px;box-shadow:0 0 3px rgba(255,255,255,.5)}\n";
    html += ".vu-housing{background:#111827;border:4px solid #4b5563;border-radius:10px;padding:12px;box-shadow:inset 0 0 20px rgba(0,0,0,.8)}\n";
    html += ".vu-scale{position:relative;width:100%;height:60px;background:#ffebcd;border-radius:4px;overflow:hidden}\n";
    html += ".vu-needle{position:absolute;width:2px;height:100%;background:#dc2626;bottom:0;left:50%;transform-origin:bottom center;transition:transform .04s linear;box-shadow:0 0 5px #dc2626}\n";
    html += ".vu-dim{opacity:.35;filter:grayscale(.3)}\n";
    html += ".switch-group button{background:#374151;color:#e5e7eb;padding:8px 12px;border-radius:6px;border:1px solid #1f2937;font-weight:600;transition:all .15s}\n";
    html += ".switch-group button.active{background:#f6ad55;color:#1a202c;box-shadow:inset 0 2px 4px rgba(0,0,0,.4)}\n";
    html += ".lamp{width:10px;height:10px;border-radius:50%;background:#374151;box-shadow:inset 0 0 3px rgba(0,0,0,.8)}\n";
    html += ".lamp.on.green{background:#34d399;box-shadow:0 0 6px #34d399}\n";
    html += ".lamp.on.red{background:#f87171;box-shadow:0 0 6px #f87171}\n";
    html += "#tech-panel{max-height:0;opacity:0;overflow:hidden;transition:max-height .5s ease,opacity .4s ease}\n";
    html += "#tech-panel.open{max-height:600px;opacity:1}\n";
    html += ".toggle-switch{display:inline-block;width:50px;height:26px;background:#4a5568;border-radius:13px;cursor:pointer;position:relative;border:1px solid #1a202c;transition:background .2s}\n";
    html += ".toggle-switch.active{background:#f6ad55}\n";
    html += ".toggle-switch-handle{position:absolute;top:2px;left:2px;width:20px;height:20px;background:#fff;border-radius:50%;box-shadow:0 1px 3px rgba(0,0,0,.4);transition:transform .2s}\n";
    html += ".toggle-switch.active .toggle-switch-handle{transform:translateX(24px)}\n";
    html += "<";
    html += "/style>\n";
    html += "<";
    html += "/head>\n";
    html += "<body>\n";
    html += "  <div class=\"metal-panel rounded-2xl p-4 w-full h-full flex flex-col\" style=\"max-width:900px;margin:0 auto\">\n";
    html += "    <div class=\"flex justify-between items-center pb-3 border-b border-black/30\">\n";
    html += "      <div>\n";
    html += "        <h1 class=\"text-2xl font-bold text-white tracking-wider\">ANALOGEXACT<";
    html += "/h1>\n";
    html += "        <p class=\"text-xs text-gray-400\">INPUT monitor = true hard bypass • Non-linear core @ 768 kHz (120 taps)<";
    html += "/p>\n";
    html += "      <";
    html += "/div>\n";
    html += "    <";
    html += "/div>\n\n";
    html += "    <div class=\"grid grid-cols-1 gap-3 mt-3\">\n";
    html += "      <div class=\"grid grid-cols-2 gap-3\">\n";
    html += "        <div id=\"vuLwrap\" class=\"vu-housing\">\n";
    html += "          <div class=\"flex items-center justify-between text-xs mb-1\">\n";
    html += "            <span id=\"vuLabelL\" class=\"text-gray-300\">VU - L<";
    html += "/span>\n";
    html += "            <span class=\"text-gray-500\">-20  -10   0   +3  +5<";
    html += "/span>\n";
    html += "          <";
    html += "/div>\n";
    html += "          <div class=\"vu-scale\"><div id=\"vuL\" class=\"vu-needle\" style=\"transform:rotate(-70deg)\"><";
    html += "/div><";
    html += "/div>\n";
    html += "        <";
    html += "/div>\n";
    html += "        <div id=\"vuRwrap\" class=\"vu-housing\">\n";
    html += "          <div class=\"flex items-center justify-between text-xs mb-1\">\n";
    html += "            <span id=\"vuLabelR\" class=\"text-gray-300\">VU - R<";
    html += "/span>\n";
    html += "            <span class=\"text-gray-500\">-20  -10   0   +3  +5<";
    html += "/span>\n";
    html += "          <";
    html += "/div>\n";
    html += "          <div class=\"vu-scale\"><div id=\"vuR\" class=\"vu-needle\" style=\"transform:rotate(-70deg)\"><";
    html += "/div><";
    html += "/div>\n";
    html += "        <";
    html += "/div>\n";
    html += "      <";
    html += "/div>\n\n";
    html += "      <div class=\"flex items-center gap-3 px-2\">\n";
    html += "        <div class=\"flex items-center gap-2\">\n";
    html += "          <div id=\"lamp-ready\" class=\"lamp on green\"><";
    html += "/div><span class=\"text-xs\">READY<";
    html += "/span>\n";
    html += "          <div id=\"lamp-play\" class=\"lamp\"><";
    html += "/div><span class=\"text-xs\">PLAY<";
    html += "/span>\n";
    html += "          <div id=\"lamp-stop\" class=\"lamp on red\"><";
    html += "/div><span class=\"text-xs\">STOP<";
    html += "/span>\n";
    html += "        <";
    html += "/div>\n";
    html += "        <div class=\"flex items-center gap-2 pl-4 border-l border-black/30\">\n";
    html += "          <div id=\"lamp-ch1\" class=\"lamp on green\"><";
    html += "/div><span class=\"text-xs\">CH-1<";
    html += "/span>\n";
    html += "          <div id=\"lamp-ch2\" class=\"lamp on green\"><";
    html += "/div><span class=\"text-xs\">CH-2<";
    html += "/span>\n";
    html += "        <";
    html += "/div>\n";
    html += "      <";
    html += "/div>\n\n";
    html += "      <div class=\"grid grid-cols-3 gap-4\">\n";
    html += "        <div class=\"module-bg p-4 rounded-lg flex flex-col items-center gap-2\">\n";
    html += "          <h2 class=\"text-sm font-bold\">INPUT LEVEL<";
    html += "/h2>\n";
    html += "          <div class=\"knob\" id=\"knob-in\"><div class=\"knob-ind\"><";
    html += "/div><";
    html += "/div>\n";
    html += "          <span id=\"val-in\" class=\"font-mono text-xs text-yellow-300\">0.0 dB<";
    html += "/span>\n";
    html += "        <";
    html += "/div>\n";
    html += "        <div class=\"module-bg p-4 rounded-lg flex flex-col items-center gap-2\">\n";
    html += "          <h2 class=\"text-sm font-bold\">OUTPUT LEVEL<";
    html += "/h2>\n";
    html += "          <div class=\"knob\" id=\"knob-out\"><div class=\"knob-ind\"><";
    html += "/div><";
    html += "/div>\n";
    html += "          <span id=\"val-out\" class=\"font-mono text-xs text-yellow-300\">0.0 dB<";
    html += "/span>\n";
    html += "        <";
    html += "/div>\n";
    html += "        <div class=\"module-bg p-4 rounded-lg flex flex-col items-center gap-2\">\n";
    html += "          <h2 class=\"text-sm font-bold\">BIAS ADJUST<";
    html += "/h2>\n";
    html += "          <div class=\"knob\" id=\"knob-bias\"><div class=\"knob-ind\"><";
    html += "/div><";
    html += "/div>\n";
    html += "          <span id=\"val-bias\" class=\"font-mono text-xs text-yellow-300\">0.0 dB<";
    html += "/span>\n";
    html += "        <";
    html += "/div>\n";
    html += "      <";
    html += "/div>\n\n";
    html += "      <div class=\"grid grid-cols-2 gap-4\">\n";
    html += "        <div class=\"module-bg p-4 rounded-lg\">\n";
    html += "          <h2 class=\"text-sm font-bold mb-2 text-center\">MONITOR<";
    html += "/h2>\n";
    html += "          <div id=\"monitor\" class=\"switch-group flex gap-1\">\n";
    html += "            <button data-monitor=\"0\" class=\"flex-1 active\">INPUT<";
    html += "/button>\n";
    html += "            <button data-monitor=\"1\" class=\"flex-1\">REPRO<";
    html += "/button>\n";
    html += "          <";
    html += "/div>\n";
    html += "        <";
    html += "/div>\n";
    html += "        <div class=\"module-bg p-4 rounded-lg\">\n";
    html += "          <h2 class=\"text-sm font-bold mb-2 text-center\">HEADBLOCK<";
    html += "/h2>\n";
    html += "          <div id=\"headblock\" class=\"switch-group flex gap-1\">\n";
    html += "            <button data-headblock=\"0\" class=\"flex-1 active\">STEREO<";
    html += "/button>\n";
    html += "            <button data-headblock=\"1\" class=\"flex-1\">MONO<";
    html += "/button>\n";
    html += "          <";
    html += "/div>\n";
    html += "        <";
    html += "/div>\n";
    html += "      <";
    html += "/div>\n";
    html += "    <";
    html += "/div>\n\n";
    html += "    <div class=\"text-center pt-2\">\n";
    html += "      <button id=\"toggle-tech\" class=\"text-yellow-400 hover:text-yellow-300 font-semibold text-xs\">Technician's Setup Panel ▼<";
    html += "/button>\n";
    html += "    <";
    html += "/div>\n\n";
    html += "    <div id=\"tech-panel\" class=\"module-bg rounded-lg p-3 mt-2\">\n";
    html += "      <div class=\"grid grid-cols-3 gap-3 text-xs\">\n";
    html += "        <div>\n";
    html += "          <label class=\"font-semibold block mb-1 text-center\">SPEED (IPS)<";
    html += "/label>\n";
    html += "          <div id=\"speed\" class=\"switch-group flex gap-1\">\n";
    html += "            <button data-speed=\"0\">7.5<";
    html += "/button>\n";
    html += "            <button data-speed=\"1\" class=\"active\">15<";
    html += "/button>\n";
    html += "            <button data-speed=\"2\">30<";
    html += "/button>\n";
    html += "          <";
    html += "/div>\n";
    html += "        <";
    html += "/div>\n";
    html += "        <div>\n";
    html += "          <label class=\"font-semibold block mb-1 text-center\">FLUX (nWb/m)<";
    html += "/label>\n";
    html += "          <div id=\"flux\" class=\"switch-group flex gap-1\">\n";
    html += "            <button data-flux=\"0\">185<";
    html += "/button>\n";
    html += "            <button data-flux=\"1\" class=\"active\">250<";
    html += "/button>\n";
    html += "            <button data-flux=\"2\">370<";
    html += "/button>\n";
    html += "          <";
    html += "/div>\n";
    html += "        <";
    html += "/div>\n";
    html += "        <div>\n";
    html += "          <label class=\"font-semibold block mb-1 text-center\">EQ<";
    html += "/label>\n";
    html += "          <div id=\"eq\" class=\"switch-group flex gap-1\">\n";
    html += "            <button data-eq=\"0\" class=\"active\">NAB<";
    html += "/button>\n";
    html += "            <button data-eq=\"1\">IEC<";
    html += "/button>\n";
    html += "          <";
    html += "/div>\n";
    html += "        <";
    html += "/div>\n";
    html += "      <";
    html += "/div>\n";
    html += "      <div class=\"mt-3\">\n";
    html += "        <label class=\"font-semibold block mb-1 text-center text-xs\">TAPE TYPE<";
    html += "/label>\n";
    html += "        <div id=\"tape\" class=\"switch-group grid grid-cols-3 gap-1\">\n";
    html += "          <button data-tapetype=\"0\">406<";
    html += "/button>\n";
    html += "          <button data-tapetype=\"1\" class=\"active\">456<";
    html += "/button>\n";
    html += "          <button data-tapetype=\"2\">499<";
    html += "/button>\n";
    html += "          <button data-tapetype=\"3\">GP9<";
    html += "/button>\n";
    html += "          <button data-tapetype=\"4\">SM900<";
    html += "/button>\n";
    html += "          <button data-tapetype=\"5\">SM911<";
    html += "/button>\n";
    html += "        <";
    html += "/div>\n";
    html += "      <";
    html += "/div>\n";
    html += "      <div class=\"flex gap-3 mt-3 items-center justify-center\">\n";
    html += "        <div class=\"flex items-center gap-2\">\n";
    html += "          <div id=\"transformer\" class=\"toggle-switch active\"><div class=\"toggle-switch-handle\"><";
    html += "/div><";
    html += "/div>\n";
    html += "          <span class=\"text-xs font-semibold\">Transformer I/O<";
    html += "/span>\n";
    html += "        <";
    html += "/div>\n";
    html += "        <div class=\"flex items-center gap-2\">\n";
    html += "          <div id=\"autocal\" class=\"toggle-switch active\"><div class=\"toggle-switch-handle\"><";
    html += "/div><";
    html += "/div>\n";
    html += "          <span class=\"text-xs font-semibold\">Auto Cal<";
    html += "/span>\n";
    html += "        <";
    html += "/div>\n";
    html += "      <";
    html += "/div>\n";
    html += "    <";
    html += "/div>\n";
    html += "  <";
    html += "/div>\n\n";
    
    // JavaScript
    html += "<script>\n";
    html += "(function(){'use strict';\n";
    html += "const $=function(id){return document.getElementById(id);};\n";
    html += "const state={inDb:0,outDb:0,bias:0,monitor:0,speed:1,flux:1,eq:0,tapeType:1,transformer:true,headblock:0,autoCal:true};\n";
    html += "function makeKnob(knobId,valId,key,min,max,fmt){\n";
    html += "  const el=$(knobId),ind=el.querySelector('.knob-ind'),val=$(valId);\n";
    html += "  let dragging=false,startY=0,startVal=0;\n";
    html += "  function setVal(v){\n";
    html += "    const c=Math.min(max,Math.max(min,v));\n";
    html += "    state[key]=Math.round(c*10)/10;\n";
    html += "    const pct=(state[key]-min)/(max-min);\n";
    html += "    ind.style.transform='rotate('+(270*pct-135)+'deg)';\n";
    html += "    val.textContent=fmt(state[key]);\n";
    html += "    window.sendParameterToPlugin&&window.sendParameterToPlugin(key,state[key]);\n";
    html += "  }\n";
    html += "  setVal(state[key]||0);\n";
    html += "  el.addEventListener('pointerdown',function(e){dragging=true;startY=e.clientY;startVal=state[key];e.preventDefault();});\n";
    html += "  el.addEventListener('pointermove',function(e){if(!dragging)return;const dy=startY-e.clientY;setVal(startVal+(dy/150)*(max-min));});\n";
    html += "  const end=function(){dragging=false;};\n";
    html += "  el.addEventListener('pointerup',end);\n";
    html += "  el.addEventListener('pointercancel',end);\n";
    html += "  el.addEventListener('wheel',function(e){e.preventDefault();const step=(max-min)/100;setVal(state[key]+(e.deltaY<0?step:-step));},{passive:false});\n";
    html += "  return setVal;\n";
    html += "}\n";
    html += "const setInput=makeKnob('knob-in','val-in','inDb',-12,12,function(v){return v.toFixed(1)+' dB';});\n";
    html += "const setOutput=makeKnob('knob-out','val-out','outDb',-24,6,function(v){return v.toFixed(1)+' dB';});\n";
    html += "const setBias=makeKnob('knob-bias','val-bias','bias',-5,5,function(v){return v.toFixed(1)+' dB';});\n";
    html += "function setupButtonGroup(groupId,key){\n";
    html += "  const group=$(groupId);\n";
    html += "  group.addEventListener('click',function(e){\n";
    html += "    if(e.target.tagName!=='BUTTON')return;\n";
    html += "    const val=parseInt(e.target.dataset[key]);\n";
    html += "    state[key]=val;\n";
    html += "    group.querySelectorAll('button').forEach(function(b){b.classList.remove('active');});\n";
    html += "    e.target.classList.add('active');\n";
    html += "    window.sendParameterToPlugin&&window.sendParameterToPlugin(key,val);\n";
    html += "    updateChannelLamps();\n";
    html += "  });\n";
    html += "}\n";
    html += "setupButtonGroup('monitor','monitor');\n";
    html += "setupButtonGroup('headblock','headblock');\n";
    html += "setupButtonGroup('speed','speed');\n";
    html += "setupButtonGroup('flux','flux');\n";
    html += "setupButtonGroup('eq','eq');\n";
    html += "setupButtonGroup('tape','tapetype');\n";
    html += "function setupToggle(id,key){\n";
    html += "  const el=$(id);\n";
    html += "  el.addEventListener('click',function(){\n";
    html += "    state[key]=!state[key];\n";
    html += "    el.classList.toggle('active',state[key]);\n";
    html += "    window.sendParameterToPlugin&&window.sendParameterToPlugin(key,state[key]);\n";
    html += "  });\n";
    html += "}\n";
    html += "setupToggle('transformer','transformer');\n";
    html += "setupToggle('autocal','autoCal');\n";
    html += "$('toggle-tech').addEventListener('click',function(){\n";
    html += "  const panel=$('tech-panel');\n";
    html += "  const open=panel.classList.contains('open');\n";
    html += "  if(open){panel.classList.remove('open');$('toggle-tech').textContent=\"Technician's Setup Panel \\u25BC\";}\n";
    html += "  else{panel.classList.add('open');$('toggle-tech').textContent=\"Technician's Setup Panel \\u25B2\";}\n";
    html += "});\n";
    html += "window.updateVUMeters=function(dbL,dbR){\n";
    html += "  const needle=function(db){return Math.max(-70,Math.min(20,(db+50)*2.25-70));};\n";
    html += "  $('vuL').style.transform='rotate('+needle(dbL)+'deg)';\n";
    html += "  $('vuR').style.transform='rotate('+needle(dbR)+'deg)';\n";
    html += "};\n";
    html += "window.setParameter=function(name,value){\n";
    html += "  if(name==='inputGain'){setInput(value);}\n";
    html += "  else if(name==='outputGain'){setOutput(value);}\n";
    html += "  else if(name==='bias'){setBias(value);}\n";
    html += "  else if(name==='monitor'){state.monitor=value;$('monitor').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.monitor)===value);});}\n";
    html += "  else if(name==='speed'){state.speed=value;$('speed').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.speed)===value);});}\n";
    html += "  else if(name==='flux'){state.flux=value;$('flux').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.flux)===value);});}\n";
    html += "  else if(name==='eq'){state.eq=value;$('eq').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.eq)===value);});}\n";
    html += "  else if(name==='tapeType'){state.tapeType=value;$('tape').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.tapetype)===value);});}\n";
    html += "  else if(name==='transformer'){state.transformer=value;$('transformer').classList.toggle('active',value);}\n";
    html += "  else if(name==='headblock'){state.headblock=value;$('headblock').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.headblock)===value);});updateChannelLamps();}\n";
    html += "  else if(name==='autoCal'){state.autoCal=value;$('autocal').classList.toggle('active',value);}\n";
    html += "};\n";
    html += "function updateChannelLamps(){\n";
    html += "  const isMono=(state.headblock===1);\n";
    html += "  $('lamp-ch2').classList.toggle('on',!isMono);\n";
    html += "  $('lamp-ch2').classList.toggle('green',!isMono);\n";
    html += "  $('vuRwrap').classList.toggle('vu-dim',isMono);\n";
    html += "  $('vuLabelR').textContent=isMono?'VU - R (idle)':'VU - R';\n";
    html += "}\n";
    html += "updateChannelLamps();\n";
    html += "window.sendParameterToPlugin=function(key,value){\n";
    html += "  const paramMap={inDb:'inputGain',outDb:'outputGain',bias:'bias',monitor:'monitor',speed:'speed',flux:'flux',eq:'eq',tapetype:'tapeType',transformer:'transformer',headblock:'headblock',autoCal:'autoCal'};\n";
    html += "  const paramId=paramMap[key];\n";
    html += "  if(paramId){window.location.href='juceplugin://parameterChanged?id='+paramId+'&value='+value;}\n";
    html += "};\n";
    html += "})();\n";
    html += "<";
    html += "/script>\n";
    html += "<";
    html += "/body>\n";
    html += "<";
    html += "/html>\n";
    
    return html;
}
