#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalogExactAudioProcessorEditor::AnalogExactAudioProcessorEditor (
    AnalogExactAudioProcessor& p,
    juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState (vts)
{
    setSize (920, 820);
    setResizable (false, false);
    
    // Create WebView - must be done on message thread
    juce::MessageManager::callAsync ([this]()
    {
        if (this == nullptr) return;
        
        webView = std::make_unique<juce::WebBrowserComponent> (
            juce::WebBrowserComponent::Options()
                .withBackend (juce::WebBrowserComponent::Options::Backend::defaultBackend)
                .withWinWebView2Options (
                    juce::WebBrowserComponent::Options::WinWebView2{}
                )
        );
        
        if (webView != nullptr)
        {
            addAndMakeVisible (webView.get());
            webView->setBounds (getLocalBounds());
            
            // Load HTML content
            juce::String html = generateHTML();
            webView->goToURL ("data:text/html;charset=utf-8," + juce::URL::addEscapeChars (html, false));
            
            // Mark as ready after a delay to let the page load
            juce::Timer::callAfterDelay (1000, [this]()
            {
                if (this == nullptr) return;
                webViewReady = true;
                updateWebViewParameters();
            });
        }
    });
    
    // Start timer for VU updates
    startTimerHz (24);
}

AnalogExactAudioProcessorEditor::~AnalogExactAudioProcessorEditor()
{
    stopTimer();
    webViewReady = false;
    webView = nullptr;
}

//==============================================================================
void AnalogExactAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a202c));
}

void AnalogExactAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

void AnalogExactAudioProcessorEditor::safeEvaluateJS (const juce::String& script)
{
    if (webView == nullptr || !webViewReady.load())
        return;
    
    // Must be called from message thread
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        webView->evaluateJavascript (script);
    }
    else
    {
        juce::MessageManager::callAsync ([this, script]()
        {
            if (webView != nullptr && webViewReady.load())
                webView->evaluateJavascript (script);
        });
    }
}

void AnalogExactAudioProcessorEditor::executeJS (const juce::String& script)
{
    safeEvaluateJS (script);
}

void AnalogExactAudioProcessorEditor::timerCallback()
{
    if (!webViewReady.load() || webView == nullptr)
        return;
    
    // Update VU meters
    float levelL = audioProcessor.getInputLevelL();
    float levelR = audioProcessor.getInputLevelR();
    
    float dbL = 20.0f * std::log10 (levelL + 1e-9f);
    float dbR = 20.0f * std::log10 (levelR + 1e-9f);
    
    juce::String script = juce::String::formatted (
        "if(typeof updateVUMeters==='function')updateVUMeters(%f,%f);",
        dbL, dbR
    );
    safeEvaluateJS (script);
    
    // Update parameters from DAW automation
    updateWebViewParameters();
}

void AnalogExactAudioProcessorEditor::updateWebViewParameters()
{
    if (!webViewReady.load() || webView == nullptr)
        return;
    
    const float epsilon = 0.01f;
    
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
    
    if (std::abs (inputGain - lastInputGain) > epsilon)
    {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('inputGain',%f);", inputGain));
        lastInputGain = inputGain;
    }
    
    if (std::abs (outputGain - lastOutputGain) > epsilon)
    {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('outputGain',%f);", outputGain));
        lastOutputGain = outputGain;
    }
    
    if (std::abs (bias - lastBias) > epsilon)
    {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('bias',%f);", bias));
        lastBias = bias;
    }
    
    if (monitor != lastMonitor)
    {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('monitor',%d);", monitor));
        lastMonitor = monitor;
    }
    
    if (speed != lastSpeed)
    {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('speed',%d);", speed));
        lastSpeed = speed;
    }
    
    if (flux != lastFlux)
    {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('flux',%d);", flux));
        lastFlux = flux;
    }
    
    if (eq != lastEQ)
    {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('eq',%d);", eq));
        lastEQ = eq;
    }
    
    if (tapeType != lastTapeType)
    {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('tapeType',%d);", tapeType));
        lastTapeType = tapeType;
    }
    
    if (transformer != lastTransformer)
    {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('transformer',%s);", transformer ? "true" : "false"));
        lastTransformer = transformer;
    }
    
    if (headblock != lastHeadblock)
    {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('headblock',%d);", headblock));
        lastHeadblock = headblock;
    }
    
    if (autoCal != lastAutoCal)
    {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('autoCal',%s);", autoCal ? "true" : "false"));
        lastAutoCal = autoCal;
    }
}

juce::String AnalogExactAudioProcessorEditor::generateHTML()
{
    juce::String html;
    
    html << "<!DOCTYPE html>";
    html << "<html lang='en'>";
    html << "<head>";
    html << "<meta charset='utf-8'/>";
    html << "<meta name='viewport' content='width=device-width,initial-scale=1'/>";
    html << "<title>ANALOGEXACT<" << "/title>";
    html << "<style>";
    html << "html,body{height:100%;background:#1a202c;margin:0;padding:0;overflow:hidden;font-family:'Inter',system-ui,sans-serif;color:#e5e7eb}";
    html << ".metal-panel{background:linear-gradient(145deg,#4a5568,#3a4454);border:1px solid #718096;border-top-color:#a0aec0;border-radius:16px;padding:16px;height:calc(100% - 32px);box-sizing:border-box;display:flex;flex-direction:column}";
    html << ".module-bg{background:rgba(0,0,0,.2);border:1px solid rgba(0,0,0,.4);box-shadow:inset 0 2px 6px rgba(0,0,0,.4);border-radius:8px;padding:16px}";
    html << ".knob{position:relative;width:80px;height:80px;border-radius:50%;background:linear-gradient(145deg,#5a6578,#2d3748);border:2px solid #1a202c;box-shadow:0 5px 10px rgba(0,0,0,.5),inset 0 2px 3px rgba(255,255,255,.08);display:flex;align-items:center;justify-content:center;cursor:pointer;user-select:none;touch-action:none}";
    html << ".knob-ind{position:absolute;width:4px;height:12px;background:#e2e8f0;top:6px;border-radius:2px;transform-origin:center 34px;box-shadow:0 0 3px rgba(255,255,255,.5)}";
    html << ".vu-housing{background:#111827;border:4px solid #4b5563;border-radius:10px;padding:12px;box-shadow:inset 0 0 20px rgba(0,0,0,.8)}";
    html << ".vu-scale{position:relative;width:100%;height:60px;background:#ffebcd;border-radius:4px;overflow:hidden}";
    html << ".vu-needle{position:absolute;width:2px;height:100%;background:#dc2626;bottom:0;left:50%;transform-origin:bottom center;transition:transform .04s linear;box-shadow:0 0 5px #dc2626}";
    html << ".vu-dim{opacity:.35;filter:grayscale(.3)}";
    html << ".switch-group{display:flex;gap:4px}";
    html << ".switch-group button{background:#374151;color:#e5e7eb;padding:8px 12px;border-radius:6px;border:1px solid #1f2937;font-weight:600;cursor:pointer;flex:1;font-size:12px}";
    html << ".switch-group button.active{background:#f6ad55;color:#1a202c;box-shadow:inset 0 2px 4px rgba(0,0,0,.4)}";
    html << ".lamp{width:10px;height:10px;border-radius:50%;background:#374151;box-shadow:inset 0 0 3px rgba(0,0,0,.8);display:inline-block}";
    html << ".lamp.on.green{background:#34d399;box-shadow:0 0 6px #34d399}";
    html << ".lamp.on.red{background:#f87171;box-shadow:0 0 6px #f87171}";
    html << "#tech-panel{max-height:0;opacity:0;overflow:hidden;transition:max-height .5s ease,opacity .4s ease}";
    html << "#tech-panel.open{max-height:400px;opacity:1}";
    html << ".toggle-switch{display:inline-block;width:50px;height:26px;background:#4a5568;border-radius:13px;cursor:pointer;position:relative;border:1px solid #1a202c;transition:background .2s}";
    html << ".toggle-switch.active{background:#f6ad55}";
    html << ".toggle-switch-handle{position:absolute;top:2px;left:2px;width:20px;height:20px;background:#fff;border-radius:50%;box-shadow:0 1px 3px rgba(0,0,0,.4);transition:transform .2s}";
    html << ".toggle-switch.active .toggle-switch-handle{transform:translateX(24px)}";
    html << ".grid-2{display:grid;grid-template-columns:1fr 1fr;gap:12px}";
    html << ".grid-3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px}";
    html << ".text-center{text-align:center}";
    html << ".text-xs{font-size:11px}";
    html << ".text-sm{font-size:13px}";
    html << ".font-bold{font-weight:700}";
    html << ".font-mono{font-family:monospace}";
    html << ".text-yellow{color:#fcd34d}";
    html << ".text-gray{color:#9ca3af}";
    html << ".flex{display:flex}";
    html << ".items-center{align-items:center}";
    html << ".justify-center{justify-content:center}";
    html << ".gap-2{gap:8px}";
    html << ".gap-3{gap:12px}";
    html << ".mb-1{margin-bottom:4px}";
    html << ".mb-2{margin-bottom:8px}";
    html << ".mt-2{margin-top:8px}";
    html << ".mt-3{margin-top:12px}";
    html << ".pb-3{padding-bottom:12px}";
    html << ".border-b{border-bottom:1px solid rgba(0,0,0,.3)}";
    html << "<" << "/style>";
    html << "<" << "/head>";
    html << "<body>";
    html << "<div class='metal-panel'>";
    
    // Header
    html << "<div class='pb-3 border-b mb-2'>";
    html << "<h1 style='font-size:24px;font-weight:700;color:white;margin:0'>ANALOGEXACT<" << "/h1>";
    html << "<p class='text-xs text-gray' style='margin:4px 0 0'>INPUT monitor = true hard bypass • Non-linear core @ 768 kHz<" << "/p>";
    html << "<" << "/div>";
    
    // VU Meters
    html << "<div class='grid-2 mb-2'>";
    html << "<div id='vuLwrap' class='vu-housing'>";
    html << "<div class='flex items-center' style='justify-content:space-between'><span class='text-xs' id='vuLabelL'>VU - L<" << "/span><span class='text-xs text-gray'>-20 -10 0 +3 +5<" << "/span><" << "/div>";
    html << "<div class='vu-scale mt-2'><div id='vuL' class='vu-needle' style='transform:rotate(-70deg)'><" << "/div><" << "/div>";
    html << "<" << "/div>";
    html << "<div id='vuRwrap' class='vu-housing'>";
    html << "<div class='flex items-center' style='justify-content:space-between'><span class='text-xs' id='vuLabelR'>VU - R<" << "/span><span class='text-xs text-gray'>-20 -10 0 +3 +5<" << "/span><" << "/div>";
    html << "<div class='vu-scale mt-2'><div id='vuR' class='vu-needle' style='transform:rotate(-70deg)'><" << "/div><" << "/div>";
    html << "<" << "/div>";
    html << "<" << "/div>";
    
    // Status Lamps
    html << "<div class='flex items-center gap-3 mb-2' style='padding:0 8px'>";
    html << "<div class='flex items-center gap-2'><div id='lamp-ready' class='lamp on green'><" << "/div><span class='text-xs'>READY<" << "/span><" << "/div>";
    html << "<div class='flex items-center gap-2'><div id='lamp-play' class='lamp'><" << "/div><span class='text-xs'>PLAY<" << "/span><" << "/div>";
    html << "<div class='flex items-center gap-2'><div id='lamp-stop' class='lamp on red'><" << "/div><span class='text-xs'>STOP<" << "/span><" << "/div>";
    html << "<div style='border-left:1px solid rgba(0,0,0,.3);height:20px;margin:0 8px'><" << "/div>";
    html << "<div class='flex items-center gap-2'><div id='lamp-ch1' class='lamp on green'><" << "/div><span class='text-xs'>CH-1<" << "/span><" << "/div>";
    html << "<div class='flex items-center gap-2'><div id='lamp-ch2' class='lamp on green'><" << "/div><span class='text-xs'>CH-2<" << "/span><" << "/div>";
    html << "<" << "/div>";
    
    // Knobs
    html << "<div class='grid-3 mb-2'>";
    html << "<div class='module-bg text-center'><h2 class='text-sm font-bold mb-2'>INPUT LEVEL<" << "/h2><div class='flex justify-center'><div class='knob' id='knob-in'><div class='knob-ind'><" << "/div><" << "/div><" << "/div><div id='val-in' class='font-mono text-xs text-yellow mt-2'>0.0 dB<" << "/div><" << "/div>";
    html << "<div class='module-bg text-center'><h2 class='text-sm font-bold mb-2'>OUTPUT LEVEL<" << "/h2><div class='flex justify-center'><div class='knob' id='knob-out'><div class='knob-ind'><" << "/div><" << "/div><" << "/div><div id='val-out' class='font-mono text-xs text-yellow mt-2'>0.0 dB<" << "/div><" << "/div>";
    html << "<div class='module-bg text-center'><h2 class='text-sm font-bold mb-2'>BIAS ADJUST<" << "/h2><div class='flex justify-center'><div class='knob' id='knob-bias'><div class='knob-ind'><" << "/div><" << "/div><" << "/div><div id='val-bias' class='font-mono text-xs text-yellow mt-2'>0.0 dB<" << "/div><" << "/div>";
    html << "<" << "/div>";
    
    // Monitor & Headblock
    html << "<div class='grid-2 mb-2'>";
    html << "<div class='module-bg'><h2 class='text-sm font-bold mb-2 text-center'>MONITOR<" << "/h2><div id='monitor' class='switch-group'><button data-monitor='0' class='active'>INPUT<" << "/button><button data-monitor='1'>REPRO<" << "/button><" << "/div><" << "/div>";
    html << "<div class='module-bg'><h2 class='text-sm font-bold mb-2 text-center'>HEADBLOCK<" << "/h2><div id='headblock' class='switch-group'><button data-headblock='0' class='active'>STEREO<" << "/button><button data-headblock='1'>MONO<" << "/button><" << "/div><" << "/div>";
    html << "<" << "/div>";
    
    // Tech Panel Toggle
    html << "<div class='text-center mb-2'><button id='toggle-tech' style='background:none;border:none;color:#fcd34d;cursor:pointer;font-weight:600;font-size:12px'>Technician's Setup Panel ▼<" << "/button><" << "/div>";
    
    // Tech Panel
    html << "<div id='tech-panel' class='module-bg'>";
    html << "<div class='grid-3 mb-2'>";
    html << "<div><div class='text-xs font-bold mb-1 text-center'>SPEED (IPS)<" << "/div><div id='speed' class='switch-group'><button data-speed='0'>7.5<" << "/button><button data-speed='1' class='active'>15<" << "/button><button data-speed='2'>30<" << "/button><" << "/div><" << "/div>";
    html << "<div><div class='text-xs font-bold mb-1 text-center'>FLUX (nWb/m)<" << "/div><div id='flux' class='switch-group'><button data-flux='0'>185<" << "/button><button data-flux='1' class='active'>250<" << "/button><button data-flux='2'>370<" << "/button><" << "/div><" << "/div>";
    html << "<div><div class='text-xs font-bold mb-1 text-center'>EQ<" << "/div><div id='eq' class='switch-group'><button data-eq='0' class='active'>NAB<" << "/button><button data-eq='1'>IEC<" << "/button><" << "/div><" << "/div>";
    html << "<" << "/div>";
    html << "<div class='mb-2'><div class='text-xs font-bold mb-1 text-center'>TAPE TYPE<" << "/div><div id='tape' class='switch-group' style='display:grid;grid-template-columns:repeat(3,1fr);gap:4px'><button data-tapetype='0'>406<" << "/button><button data-tapetype='1' class='active'>456<" << "/button><button data-tapetype='2'>499<" << "/button><button data-tapetype='3'>GP9<" << "/button><button data-tapetype='4'>SM900<" << "/button><button data-tapetype='5'>SM911<" << "/button><" << "/div><" << "/div>";
    html << "<div class='flex justify-center gap-3'>";
    html << "<div class='flex items-center gap-2'><div id='transformer' class='toggle-switch active'><div class='toggle-switch-handle'><" << "/div><" << "/div><span class='text-xs font-bold'>Transformer I/O<" << "/span><" << "/div>";
    html << "<div class='flex items-center gap-2'><div id='autocal' class='toggle-switch active'><div class='toggle-switch-handle'><" << "/div><" << "/div><span class='text-xs font-bold'>Auto Cal<" << "/span><" << "/div>";
    html << "<" << "/div>";
    html << "<" << "/div>";
    
    html << "<" << "/div>";
    
    // JavaScript
    html << "<script>";
    html << "(function(){'use strict';";
    html << "var $=function(id){return document.getElementById(id);};";
    html << "var state={inDb:0,outDb:0,bias:0,monitor:0,speed:1,flux:1,eq:0,tapeType:1,transformer:true,headblock:0,autoCal:true};";
    
    // Knob handler
    html << "function makeKnob(knobId,valId,key,min,max,fmt){";
    html << "var el=$(knobId),ind=el.querySelector('.knob-ind'),val=$(valId);";
    html << "var dragging=false,startY=0,startVal=0;";
    html << "function setVal(v){var c=Math.min(max,Math.max(min,v));state[key]=Math.round(c*10)/10;var pct=(state[key]-min)/(max-min);ind.style.transform='rotate('+(270*pct-135)+'deg)';val.textContent=fmt(state[key]);}";
    html << "setVal(state[key]||0);";
    html << "el.onpointerdown=function(e){dragging=true;startY=e.clientY;startVal=state[key];e.preventDefault();};";
    html << "el.onpointermove=function(e){if(!dragging)return;var dy=startY-e.clientY;setVal(startVal+(dy/150)*(max-min));};";
    html << "el.onpointerup=el.onpointercancel=function(){dragging=false;};";
    html << "return setVal;}";
    
    html << "var setInput=makeKnob('knob-in','val-in','inDb',-12,12,function(v){return v.toFixed(1)+' dB';});";
    html << "var setOutput=makeKnob('knob-out','val-out','outDb',-24,6,function(v){return v.toFixed(1)+' dB';});";
    html << "var setBias=makeKnob('knob-bias','val-bias','bias',-5,5,function(v){return v.toFixed(1)+' dB';});";
    
    // Button groups
    html << "function setupGroup(gid,key){var g=$(gid);g.onclick=function(e){if(e.target.tagName!=='BUTTON')return;var v=parseInt(e.target.dataset[key]);state[key]=v;g.querySelectorAll('button').forEach(function(b){b.classList.remove('active');});e.target.classList.add('active');updateLamps();};}";
    html << "setupGroup('monitor','monitor');setupGroup('headblock','headblock');setupGroup('speed','speed');setupGroup('flux','flux');setupGroup('eq','eq');setupGroup('tape','tapetype');";
    
    // Toggles
    html << "function setupToggle(id,key){var el=$(id);el.onclick=function(){state[key]=!state[key];el.classList.toggle('active',state[key]);};}";
    html << "setupToggle('transformer','transformer');setupToggle('autocal','autoCal');";
    
    // Tech panel
    html << "$('toggle-tech').onclick=function(){var p=$('tech-panel');var o=p.classList.contains('open');p.classList.toggle('open',!o);this.textContent=o?\"Technician's Setup Panel \\u25BC\":\"Technician's Setup Panel \\u25B2\";};";
    
    // VU meters
    html << "window.updateVUMeters=function(dbL,dbR){var needle=function(db){return Math.max(-70,Math.min(20,(db+50)*2.25-70));};$('vuL').style.transform='rotate('+needle(dbL)+'deg)';$('vuR').style.transform='rotate('+needle(dbR)+'deg)';};";
    
    // Set parameter from C++
    html << "window.setParameter=function(n,v){";
    html << "if(n==='inputGain')setInput(v);";
    html << "else if(n==='outputGain')setOutput(v);";
    html << "else if(n==='bias')setBias(v);";
    html << "else if(n==='monitor'){state.monitor=v;$('monitor').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.monitor)===v);});}";
    html << "else if(n==='speed'){state.speed=v;$('speed').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.speed)===v);});}";
    html << "else if(n==='flux'){state.flux=v;$('flux').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.flux)===v);});}";
    html << "else if(n==='eq'){state.eq=v;$('eq').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.eq)===v);});}";
    html << "else if(n==='tapeType'){state.tapeType=v;$('tape').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.tapetype)===v);});}";
    html << "else if(n==='transformer'){state.transformer=v;$('transformer').classList.toggle('active',v);}";
    html << "else if(n==='headblock'){state.headblock=v;$('headblock').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.headblock)===v);});updateLamps();}";
    html << "else if(n==='autoCal'){state.autoCal=v;$('autocal').classList.toggle('active',v);}";
    html << "};";
    
    // Update lamps
    html << "function updateLamps(){var m=(state.headblock===1);$('lamp-ch2').classList.toggle('on',!m);$('lamp-ch2').classList.toggle('green',!m);$('vuRwrap').classList.toggle('vu-dim',m);$('vuLabelR').textContent=m?'VU - R (idle)':'VU - R';}";
    html << "updateLamps();";
    
    html << "})();";
    html << "<" << "/script>";
    html << "<" << "/body>";
    html << "<" << "/html>";
    
    return html;
}
