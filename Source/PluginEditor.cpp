#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalogExactAudioProcessorEditor::AnalogExactAudioProcessorEditor (AnalogExactAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (920, 820);
    setResizable (false, false);
    
    // Create WebView on message thread
    juce::MessageManager::callAsync ([this]()
    {
        if (this == nullptr) return;
        
        webView = std::make_unique<juce::WebBrowserComponent>();
        
        if (webView != nullptr)
        {
            addAndMakeVisible (webView.get());
            webView->setBounds (getLocalBounds());
            
            juce::String html = generateHTML();
            webView->goToURL ("data:text/html;charset=utf-8," + juce::URL::addEscapeChars (html, false));
            
            juce::Timer::callAfterDelay (1000, [this]()
            {
                if (this == nullptr) return;
                webViewReady = true;
                updateWebViewParameters();
            });
        }
    });
    
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
    if (webView == nullptr || !webViewReady.load()) return;
    
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        webView->evaluateJavascript (script);
    else
        juce::MessageManager::callAsync ([this, script]() {
            if (webView != nullptr && webViewReady.load())
                webView->evaluateJavascript (script);
        });
}

void AnalogExactAudioProcessorEditor::timerCallback()
{
    if (!webViewReady.load() || webView == nullptr) return;
    
    float levelL = audioProcessor.getInputLevelL();
    float levelR = audioProcessor.getInputLevelR();
    float dbL = 20.0f * std::log10 (levelL + 1e-9f);
    float dbR = 20.0f * std::log10 (levelR + 1e-9f);
    
    safeEvaluateJS (juce::String::formatted ("if(typeof updateVUMeters==='function')updateVUMeters(%f,%f);", dbL, dbR));
    updateWebViewParameters();
}

void AnalogExactAudioProcessorEditor::updateWebViewParameters()
{
    if (!webViewReady.load() || webView == nullptr) return;
    
    auto& vts = audioProcessor.getValueTreeState();
    const float epsilon = 0.01f;
    
    float inputGain = vts.getRawParameterValue (AnalogExactAudioProcessor::INPUT_GAIN_ID)->load();
    float outputGain = vts.getRawParameterValue (AnalogExactAudioProcessor::OUTPUT_GAIN_ID)->load();
    float bias = vts.getRawParameterValue (AnalogExactAudioProcessor::BIAS_ID)->load();
    int monitor = (int) vts.getRawParameterValue (AnalogExactAudioProcessor::MONITOR_ID)->load();
    int speed = (int) vts.getRawParameterValue (AnalogExactAudioProcessor::SPEED_ID)->load();
    int flux = (int) vts.getRawParameterValue (AnalogExactAudioProcessor::FLUX_ID)->load();
    int eq = (int) vts.getRawParameterValue (AnalogExactAudioProcessor::EQ_ID)->load();
    int tapeType = (int) vts.getRawParameterValue (AnalogExactAudioProcessor::TAPE_TYPE_ID)->load();
    bool transformer = vts.getRawParameterValue (AnalogExactAudioProcessor::TRANSFORMER_ID)->load() > 0.5f;
    int headblock = (int) vts.getRawParameterValue (AnalogExactAudioProcessor::HEADBLOCK_ID)->load();
    bool autoCal = vts.getRawParameterValue (AnalogExactAudioProcessor::AUTO_CAL_ID)->load() > 0.5f;
    
    if (std::abs (inputGain - lastInputGain) > epsilon) {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('inputGain',%f);", inputGain));
        lastInputGain = inputGain;
    }
    if (std::abs (outputGain - lastOutputGain) > epsilon) {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('outputGain',%f);", outputGain));
        lastOutputGain = outputGain;
    }
    if (std::abs (bias - lastBias) > epsilon) {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('bias',%f);", bias));
        lastBias = bias;
    }
    if (monitor != lastMonitor) {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('monitor',%d);", monitor));
        lastMonitor = monitor;
    }
    if (speed != lastSpeed) {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('speed',%d);", speed));
        lastSpeed = speed;
    }
    if (flux != lastFlux) {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('flux',%d);", flux));
        lastFlux = flux;
    }
    if (eq != lastEQ) {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('eq',%d);", eq));
        lastEQ = eq;
    }
    if (tapeType != lastTapeType) {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('tapeType',%d);", tapeType));
        lastTapeType = tapeType;
    }
    if (transformer != lastTransformer) {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('transformer',%s);", transformer ? "true" : "false"));
        lastTransformer = transformer;
    }
    if (headblock != lastHeadblock) {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('headblock',%d);", headblock));
        lastHeadblock = headblock;
    }
    if (autoCal != lastAutoCal) {
        safeEvaluateJS (juce::String::formatted ("if(typeof setParameter==='function')setParameter('autoCal',%s);", autoCal ? "true" : "false"));
        lastAutoCal = autoCal;
    }
}

juce::String AnalogExactAudioProcessorEditor::generateHTML()
{
    juce::String h;
    
    h << "<!DOCTYPE html><html><head><meta charset='utf-8'/>";
    h << "<style>";
    h << "*{box-sizing:border-box;margin:0;padding:0}";
    h << "html,body{height:100%;background:#1a202c;font-family:system-ui,sans-serif;color:#e5e7eb;overflow:hidden}";
    h << ".panel{background:linear-gradient(145deg,#4a5568,#3a4454);border:1px solid #718096;border-radius:16px;padding:16px;height:100%;display:flex;flex-direction:column}";
    h << ".module{background:rgba(0,0,0,.2);border:1px solid rgba(0,0,0,.4);box-shadow:inset 0 2px 6px rgba(0,0,0,.4);border-radius:8px;padding:12px}";
    h << ".knob{width:70px;height:70px;border-radius:50%;background:linear-gradient(145deg,#5a6578,#2d3748);border:2px solid #1a202c;box-shadow:0 4px 8px rgba(0,0,0,.5);cursor:pointer;position:relative;margin:0 auto}";
    h << ".knob-ind{position:absolute;width:4px;height:10px;background:#e2e8f0;top:6px;left:50%;margin-left:-2px;border-radius:2px;transform-origin:center 29px}";
    h << ".vu{background:#111827;border:3px solid #4b5563;border-radius:8px;padding:10px}";
    h << ".vu-scale{height:50px;background:#ffebcd;border-radius:4px;position:relative;overflow:hidden}";
    h << ".vu-needle{position:absolute;width:2px;height:100%;background:#dc2626;left:50%;bottom:0;transform-origin:bottom center;box-shadow:0 0 4px #dc2626}";
    h << ".vu-dim{opacity:.35}";
    h << ".btn-group{display:flex;gap:4px}";
    h << ".btn{background:#374151;color:#e5e7eb;padding:6px 10px;border-radius:6px;border:1px solid #1f2937;font-weight:600;cursor:pointer;flex:1;font-size:11px}";
    h << ".btn.active{background:#f6ad55;color:#1a202c}";
    h << ".lamp{width:8px;height:8px;border-radius:50%;background:#374151;display:inline-block}";
    h << ".lamp.green{background:#34d399;box-shadow:0 0 4px #34d399}";
    h << ".lamp.red{background:#f87171;box-shadow:0 0 4px #f87171}";
    h << ".toggle{width:44px;height:22px;background:#4a5568;border-radius:11px;cursor:pointer;position:relative}";
    h << ".toggle.active{background:#f6ad55}";
    h << ".toggle-handle{position:absolute;top:2px;left:2px;width:18px;height:18px;background:#fff;border-radius:50%;transition:transform .2s}";
    h << ".toggle.active .toggle-handle{transform:translateX(22px)}";
    h << "#tech{max-height:0;opacity:0;overflow:hidden;transition:all .3s}";
    h << "#tech.open{max-height:300px;opacity:1}";
    h << ".grid2{display:grid;grid-template-columns:1fr 1fr;gap:10px}";
    h << ".grid3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}";
    h << ".tc{text-align:center}";
    h << ".mb{margin-bottom:10px}";
    h << ".text-xs{font-size:10px}";
    h << ".text-sm{font-size:12px}";
    h << ".text-yellow{color:#fcd34d}";
    h << ".bold{font-weight:700}";
    h << "<";
    h << "/style><";
    h << "/head><body>";
    h << "<div class='panel'>";
    
    // Header
    h << "<div class='mb' style='border-bottom:1px solid rgba(0,0,0,.3);padding-bottom:10px'>";
    h << "<div class='bold' style='font-size:20px;color:white'>ANALOGEXACT<";
    h << "/div>";
    h << "<div class='text-xs' style='color:#9ca3af'>INPUT=bypass • 768kHz core<";
    h << "/div><";
    h << "/div>";
    
    // VU Meters
    h << "<div class='grid2 mb'>";
    h << "<div class='vu' id='vuLwrap'><div class='text-xs mb' style='display:flex;justify-content:space-between'><span id='vuLL'>VU-L<";
    h << "/span><span style='color:#6b7280'>-20 0 +5<";
    h << "/span><";
    h << "/div><div class='vu-scale'><div id='vuL' class='vu-needle' style='transform:rotate(-70deg)'><";
    h << "/div><";
    h << "/div><";
    h << "/div>";
    h << "<div class='vu' id='vuRwrap'><div class='text-xs mb' style='display:flex;justify-content:space-between'><span id='vuLR'>VU-R<";
    h << "/span><span style='color:#6b7280'>-20 0 +5<";
    h << "/span><";
    h << "/div><div class='vu-scale'><div id='vuR' class='vu-needle' style='transform:rotate(-70deg)'><";
    h << "/div><";
    h << "/div><";
    h << "/div>";
    h << "<";
    h << "/div>";
    
    // Lamps
    h << "<div class='mb' style='display:flex;gap:12px;align-items:center'>";
    h << "<div style='display:flex;align-items:center;gap:4px'><div class='lamp green'><";
    h << "/div><span class='text-xs'>RDY<";
    h << "/span><";
    h << "/div>";
    h << "<div style='display:flex;align-items:center;gap:4px'><div class='lamp red'><";
    h << "/div><span class='text-xs'>STP<";
    h << "/span><";
    h << "/div>";
    h << "<div style='display:flex;align-items:center;gap:4px'><div id='lc1' class='lamp green'><";
    h << "/div><span class='text-xs'>CH1<";
    h << "/span><";
    h << "/div>";
    h << "<div style='display:flex;align-items:center;gap:4px'><div id='lc2' class='lamp green'><";
    h << "/div><span class='text-xs'>CH2<";
    h << "/span><";
    h << "/div>";
    h << "<";
    h << "/div>";
    
    // Knobs
    h << "<div class='grid3 mb'>";
    h << "<div class='module tc'><div class='text-sm bold mb'>INPUT<";
    h << "/div><div class='knob' id='kin'><div class='knob-ind'><";
    h << "/div><";
    h << "/div><div id='vin' class='text-xs text-yellow' style='margin-top:6px'>0.0dB<";
    h << "/div><";
    h << "/div>";
    h << "<div class='module tc'><div class='text-sm bold mb'>OUTPUT<";
    h << "/div><div class='knob' id='kout'><div class='knob-ind'><";
    h << "/div><";
    h << "/div><div id='vout' class='text-xs text-yellow' style='margin-top:6px'>0.0dB<";
    h << "/div><";
    h << "/div>";
    h << "<div class='module tc'><div class='text-sm bold mb'>BIAS<";
    h << "/div><div class='knob' id='kbias'><div class='knob-ind'><";
    h << "/div><";
    h << "/div><div id='vbias' class='text-xs text-yellow' style='margin-top:6px'>0.0dB<";
    h << "/div><";
    h << "/div>";
    h << "<";
    h << "/div>";
    
    // Monitor & Headblock
    h << "<div class='grid2 mb'>";
    h << "<div class='module'><div class='text-sm bold mb tc'>MONITOR<";
    h << "/div><div id='gmon' class='btn-group'><button data-v='0' class='btn active'>INPUT<";
    h << "/button><button data-v='1' class='btn'>REPRO<";
    h << "/button><";
    h << "/div><";
    h << "/div>";
    h << "<div class='module'><div class='text-sm bold mb tc'>HEADBLOCK<";
    h << "/div><div id='ghb' class='btn-group'><button data-v='0' class='btn active'>STEREO<";
    h << "/button><button data-v='1' class='btn'>MONO<";
    h << "/button><";
    h << "/div><";
    h << "/div>";
    h << "<";
    h << "/div>";
    
    // Tech toggle
    h << "<div class='tc mb'><button id='tbtn' style='background:none;border:none;color:#fcd34d;cursor:pointer;font-size:11px'>Setup Panel ▼<";
    h << "/button><";
    h << "/div>";
    
    // Tech panel
    h << "<div id='tech' class='module'>";
    h << "<div class='grid3 mb'>";
    h << "<div><div class='text-xs bold mb tc'>SPEED<";
    h << "/div><div id='gspd' class='btn-group'><button data-v='0' class='btn'>7.5<";
    h << "/button><button data-v='1' class='btn active'>15<";
    h << "/button><button data-v='2' class='btn'>30<";
    h << "/button><";
    h << "/div><";
    h << "/div>";
    h << "<div><div class='text-xs bold mb tc'>FLUX<";
    h << "/div><div id='gflx' class='btn-group'><button data-v='0' class='btn'>185<";
    h << "/button><button data-v='1' class='btn active'>250<";
    h << "/button><button data-v='2' class='btn'>370<";
    h << "/button><";
    h << "/div><";
    h << "/div>";
    h << "<div><div class='text-xs bold mb tc'>EQ<";
    h << "/div><div id='geq' class='btn-group'><button data-v='0' class='btn active'>NAB<";
    h << "/button><button data-v='1' class='btn'>IEC<";
    h << "/button><";
    h << "/div><";
    h << "/div>";
    h << "<";
    h << "/div>";
    h << "<div class='mb'><div class='text-xs bold mb tc'>TAPE<";
    h << "/div><div id='gtape' class='btn-group' style='flex-wrap:wrap'><button data-v='0' class='btn'>406<";
    h << "/button><button data-v='1' class='btn active'>456<";
    h << "/button><button data-v='2' class='btn'>499<";
    h << "/button><button data-v='3' class='btn'>GP9<";
    h << "/button><button data-v='4' class='btn'>SM900<";
    h << "/button><button data-v='5' class='btn'>SM911<";
    h << "/button><";
    h << "/div><";
    h << "/div>";
    h << "<div style='display:flex;justify-content:center;gap:20px'>";
    h << "<div style='display:flex;align-items:center;gap:6px'><div id='txfr' class='toggle active'><div class='toggle-handle'><";
    h << "/div><";
    h << "/div><span class='text-xs bold'>XFMR<";
    h << "/span><";
    h << "/div>";
    h << "<div style='display:flex;align-items:center;gap:6px'><div id='tcal' class='toggle active'><div class='toggle-handle'><";
    h << "/div><";
    h << "/div><span class='text-xs bold'>CAL<";
    h << "/span><";
    h << "/div>";
    h << "<";
    h << "/div>";
    h << "<";
    h << "/div>";
    
    h << "<";
    h << "/div>";
    
    // JavaScript
    h << "<script>";
    h << "var $=function(i){return document.getElementById(i);};";
    h << "var st={inDb:0,outDb:0,bias:0,mon:0,spd:1,flx:1,eq:0,tape:1,xfr:1,hb:0,cal:1};";
    
    // Knob
    h << "function knob(id,vid,k,mn,mx,fmt){";
    h << "var el=$(id),ind=el.querySelector('.knob-ind'),vl=$(vid),dr=0,sy=0,sv=0;";
    h << "function set(v){var c=Math.min(mx,Math.max(mn,v));st[k]=Math.round(c*10)/10;var p=(st[k]-mn)/(mx-mn);ind.style.transform='rotate('+(270*p-135)+'deg)';vl.textContent=fmt(st[k]);}";
    h << "set(st[k]);";
    h << "el.onpointerdown=function(e){dr=1;sy=e.clientY;sv=st[k];e.preventDefault();};";
    h << "el.onpointermove=function(e){if(!dr)return;set(sv+(sy-e.clientY)/150*(mx-mn));};";
    h << "el.onpointerup=el.onpointercancel=function(){dr=0;};";
    h << "return set;}";
    
    h << "var sIn=knob('kin','vin','inDb',-12,12,function(v){return v.toFixed(1)+'dB';});";
    h << "var sOut=knob('kout','vout','outDb',-24,6,function(v){return v.toFixed(1)+'dB';});";
    h << "var sBias=knob('kbias','vbias','bias',-5,5,function(v){return v.toFixed(1)+'dB';});";
    
    // Button groups
    h << "function grp(id,k,cb){var g=$(id);g.onclick=function(e){if(e.target.tagName!=='BUTTON')return;var v=parseInt(e.target.dataset.v);st[k]=v;g.querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.v)===v);});if(cb)cb();};}";
    h << "function ulamps(){var m=st.hb===1;$('lc2').classList.toggle('green',!m);$('vuRwrap').classList.toggle('vu-dim',m);$('vuLR').textContent=m?'VU-R(off)':'VU-R';}";
    h << "grp('gmon','mon');grp('ghb','hb',ulamps);grp('gspd','spd');grp('gflx','flx');grp('geq','eq');grp('gtape','tape');";
    
    // Toggles
    h << "$('txfr').onclick=function(){st.xfr=st.xfr?0:1;this.classList.toggle('active',st.xfr);};";
    h << "$('tcal').onclick=function(){st.cal=st.cal?0:1;this.classList.toggle('active',st.cal);};";
    
    // Tech panel
    h << "$('tbtn').onclick=function(){var t=$('tech'),o=t.classList.toggle('open');this.textContent=o?'Setup Panel \\u25B2':'Setup Panel \\u25BC';};";
    
    // VU
    h << "window.updateVUMeters=function(l,r){var n=function(d){return Math.max(-70,Math.min(20,(d+50)*2.25-70));};$('vuL').style.transform='rotate('+n(l)+'deg)';$('vuR').style.transform='rotate('+n(r)+'deg)';};";
    
    // Set param
    h << "window.setParameter=function(n,v){";
    h << "if(n==='inputGain')sIn(v);";
    h << "else if(n==='outputGain')sOut(v);";
    h << "else if(n==='bias')sBias(v);";
    h << "else if(n==='monitor'){st.mon=v;$('gmon').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.v)===v);});}";
    h << "else if(n==='speed'){st.spd=v;$('gspd').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.v)===v);});}";
    h << "else if(n==='flux'){st.flx=v;$('gflx').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.v)===v);});}";
    h << "else if(n==='eq'){st.eq=v;$('geq').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.v)===v);});}";
    h << "else if(n==='tapeType'){st.tape=v;$('gtape').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.v)===v);});}";
    h << "else if(n==='transformer'){st.xfr=v?1:0;$('txfr').classList.toggle('active',v);}";
    h << "else if(n==='headblock'){st.hb=v;$('ghb').querySelectorAll('button').forEach(function(b){b.classList.toggle('active',parseInt(b.dataset.v)===v);});ulamps();}";
    h << "else if(n==='autoCal'){st.cal=v?1:0;$('tcal').classList.toggle('active',v);}";
    h << "};";
    
    h << "ulamps();";
    h << "<";
    h << "/script><";
    h << "/body><";
    h << "/html>";
    
    return h;
}
