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
    
    // Only send updates if values changed
    if (inputGain != lastInputGain)
    {
        setParameterInJS ("inputGain", inputGain);
        lastInputGain = inputGain;
    }
    
    if (outputGain != lastOutputGain)
    {
        setParameterInJS ("outputGain", outputGain);
        lastOutputGain = outputGain;
    }
    
    if (bias != lastBias)
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
    juce::URL parsedURL (url);
    
    auto params = parsedURL.getParameterNames();
    juce::String paramId, paramValue;
    
    for (auto& param : params)
    {
        if (param == "id")
            paramId = parsedURL.getParameterValue (param);
        else if (param == "value")
            paramValue = parsedURL.getParameterValue (param);
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
    return R"(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>ANALOGEXACT</title>
<script src="https://cdn.tailwindcss.com"></script>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=Roboto+Mono&display=swap" rel="stylesheet">
<style>
  html,body{height:100%;background:#1a202c;margin:0;padding:0;overflow:hidden}
  body{font-family:'Inter',sans-serif;color:#e5e7eb}
  .metal-panel{background:linear-gradient(145deg,#4a5568,#3a4454);border:1px solid #718096;border-top-color:#a0aec0}
  .module-bg{background:rgba(0,0,0,.2);border:1px solid rgba(0,0,0,.4);box-shadow:inset 0 2px 6px rgba(0,0,0,.4)}
  .knob{position:relative;width:80px;height:80px;border-radius:50%;background:linear-gradient(145deg,#5a6578,#2d3748);border:2px solid #1a202c;box-shadow:0 5px 10px rgba(0,0,0,.5),inset 0 2px 3px rgba(255,255,255,.08);display:flex;align-items:center;justify-content:center;cursor:pointer;user-select:none;touch-action:none}
  .knob-ind{position:absolute;width:4px;height:12px;background:#e2e8f0;top:6px;border-radius:2px;transform-origin:center 34px;box-shadow:0 0 3px rgba(255,255,255,.5)}
  .vu-housing{background:#111827;border:4px solid #4b5563;border-radius:10px;padding:12px;box-shadow:inset 0 0 20px rgba(0,0,0,.8)}
  .vu-scale{position:relative;width:100%;height:60px;background:#ffebcd;border-radius:4px;overflow:hidden}
  .vu-needle{position:absolute;width:2px;height:100%;background:#dc2626;bottom:0;left:50%;transform-origin:bottom center;transition:transform .04s linear;box-shadow:0 0 5px #dc2626}
  .vu-dim{opacity:.35;filter:grayscale(.3)}
  .switch-group button{background:#374151;color:#e5e7eb;padding:8px 12px;border-radius:6px;border:1px solid #1f2937;font-weight:600;transition:all .15s}
  .switch-group button.active{background:#f6ad55;color:#1a202c;box-shadow:inset 0 2px 4px rgba(0,0,0,.4)}
  .lamp{width:10px;height:10px;border-radius:50%;background:#374151;box-shadow:inset 0 0 3px rgba(0,0,0,.8)}
  .lamp.on.green{background:#34d399;box-shadow:0 0 6px #34d399}
  .lamp.on.red{background:#f87171;box-shadow:0 0 6px #f87171}
  #tech-panel{max-height:0;opacity:0;overflow:hidden;transition:max-height .5s ease,opacity .4s ease}
  #tech-panel.open{max-height:600px;opacity:1}
  .toggle-switch{display:inline-block;width:50px;height:26px;background:#4a5568;border-radius:13px;cursor:pointer;position:relative;border:1px solid #1a202c;transition:background .2s}
  .toggle-switch.active{background:#f6ad55}
  .toggle-switch-handle{position:absolute;top:2px;left:2px;width:20px;height:20px;background:#fff;border-radius:50%;box-shadow:0 1px 3px rgba(0,0,0,.4);transition:transform .2s}
  .toggle-switch.active .toggle-switch-handle{transform:translateX(24px)}
</style>
</head>
<body>
  <div class="metal-panel rounded-2xl p-4 w-full h-full flex flex-col" style="max-width:900px;margin:0 auto">
    <div class="flex justify-between items-center pb-3 border-b border-black/30">
      <div>
        <h1 class="text-2xl font-bold text-white tracking-wider">ANALOGEXACT</h1>
        <p class="text-xs text-gray-400">INPUT monitor = true hard bypass • Non-linear core @ 768 kHz (120 taps)</p>
      </div>
    </div>

    <div class="grid grid-cols-1 gap-3 mt-3">
      <div class="grid grid-cols-2 gap-3">
        <div id="vuLwrap" class="vu-housing">
          <div class="flex items-center justify-between text-xs mb-1">
            <span id="vuLabelL" class="text-gray-300">VU - L</span>
            <span class="text-gray-500">-20  -10   0   +3  +5</span>
          </div>
          <div class="vu-scale"><div id="vuL" class="vu-needle" style="transform:rotate(-70deg)"></div></div>
        </div>
        <div id="vuRwrap" class="vu-housing">
          <div class="flex items-center justify-between text-xs mb-1">
            <span id="vuLabelR" class="text-gray-300">VU - R</span>
            <span class="text-gray-500">-20  -10   0   +3  +5</span>
          </div>
          <div class="vu-scale"><div id="vuR" class="vu-needle" style="transform:rotate(-70deg)"></div></div>
        </div>
      </div>

      <div class="flex items-center gap-3 px-2">
        <div class="flex items-center gap-2">
          <div id="lamp-ready" class="lamp on green"></div><span class="text-xs">READY</span>
          <div id="lamp-play" class="lamp"></div><span class="text-xs">PLAY</span>
          <div id="lamp-stop" class="lamp on red"></div><span class="text-xs">STOP</span>
        </div>
        <div class="flex items-center gap-2 pl-4 border-l border-black/30">
          <div id="lamp-ch1" class="lamp on green"></div><span class="text-xs">CH-1</span>
          <div id="lamp-ch2" class="lamp on green"></div><span class="text-xs">CH-2</span>
        </div>
      </div>

      <div class="grid grid-cols-3 gap-4">
        <div class="module-bg p-4 rounded-lg flex flex-col items-center gap-2">
          <h2 class="text-sm font-bold">INPUT LEVEL</h2>
          <div class="knob" id="knob-in"><div class="knob-ind"></div></div>
          <span id="val-in" class="font-mono text-xs text-yellow-300">0.0 dB</span>
        </div>
        <div class="module-bg p-4 rounded-lg flex flex-col items-center gap-2">
          <h2 class="text-sm font-bold">OUTPUT LEVEL</h2>
          <div class="knob" id="knob-out"><div class="knob-ind"></div></div>
          <span id="val-out" class="font-mono text-xs text-yellow-300">0.0 dB</span>
        </div>
        <div class="module-bg p-4 rounded-lg flex flex-col items-center gap-2">
          <h2 class="text-sm font-bold">BIAS ADJUST</h2>
          <div class="knob" id="knob-bias"><div class="knob-ind"></div></div>
          <span id="val-bias" class="font-mono text-xs text-yellow-300">0.0 dB</span>
        </div>
      </div>

      <div class="grid grid-cols-2 gap-4">
        <div class="module-bg p-4 rounded-lg">
          <h2 class="text-sm font-bold mb-2 text-center">MONITOR</h2>
          <div id="monitor" class="switch-group flex gap-1">
            <button data-monitor="0" class="flex-1 active">INPUT</button>
            <button data-monitor="1" class="flex-1">REPRO</button>
          </div>
        </div>
        <div class="module-bg p-4 rounded-lg">
          <h2 class="text-sm font-bold mb-2 text-center">HEADBLOCK</h2>
          <div id="headblock" class="switch-group flex gap-1">
            <button data-headblock="0" class="flex-1 active">STEREO</button>
            <button data-headblock="1" class="flex-1">MONO</button>
          </div>
        </div>
      </div>
    </div>

    <div class="text-center pt-2">
      <button id="toggle-tech" class="text-yellow-400 hover:text-yellow-300 font-semibold text-xs">Technician's Setup Panel ▼</button>
    </div>

    <div id="tech-panel" class="module-bg rounded-lg p-3 mt-2">
      <div class="grid grid-cols-3 gap-3 text-xs">
        <div>
          <label class="font-semibold block mb-1 text-center">SPEED (IPS)</label>
          <div id="speed" class="switch-group flex gap-1">
            <button data-speed="0">7.5</button>
            <button data-speed="1" class="active">15</button>
            <button data-speed="2">30</button>
          </div>
        </div>
        <div>
          <label class="font-semibold block mb-1 text-center">FLUX (nWb/m)</label>
          <div id="flux" class="switch-group flex gap-1">
            <button data-flux="0">185</button>
            <button data-flux="1" class="active">250</button>
            <button data-flux="2">370</button>
          </div>
        </div>
        <div>
          <label class="font-semibold block mb-1 text-center">EQ</label>
          <div id="eq" class="switch-group flex gap-1">
            <button data-eq="0" class="active">NAB</button>
            <button data-eq="1">IEC</button>
          </div>
        </div>
      </div>
      <div class="mt-3">
        <label class="font-semibold block mb-1 text-center text-xs">TAPE TYPE</label>
        <div id="tape" class="switch-group grid grid-cols-3 gap-1">
          <button data-tapetype="0">406</button>
          <button data-tapetype="1" class="active">456</button>
          <button data-tapetype="2">499</button>
          <button data-tapetype="3">GP9</button>
          <button data-tapetype="4">SM900</button>
          <button data-tapetype="5">SM911</button>
        </div>
      </div>
      <div class="flex gap-3 mt-3 items-center justify-center">
        <div class="flex items-center gap-2">
          <div id="transformer" class="toggle-switch active"><div class="toggle-switch-handle"></div></div>
          <span class="text-xs font-semibold">Transformer I/O</span>
        </div>
        <div class="flex items-center gap-2">
          <div id="autocal" class="toggle-switch active"><div class="toggle-switch-handle"></div></div>
          <span class="text-xs font-semibold">Auto Cal</span>
        </div>
      </div>
    </div>
  </div>

<script>
(()=>{'use strict';
const $=id=>document.getElementById(id);

// State
const state = {
  inDb:0, outDb:0, bias:0, monitor:0, speed:1, flux:1, eq:0, tapeType:1, 
  transformer:true, headblock:0, autoCal:true
};

// Knobs
function makeKnob(knobId,valId,key,min,max,fmt){
  const el=$(knobId), ind=el.querySelector('.knob-ind'), val=$(valId);
  let dragging=false,startY=0,startVal=0;
  
  function setVal(v){
    const c=Math.min(max,Math.max(min,v));
    state[key]=Math.round(c*10)/10;
    const pct=(state[key]-min)/(max-min);
    ind.style.transform=`rotate(${270*pct-135}deg)`;
    val.textContent=fmt(state[key]);
    // Send to C++
    window.sendParameterToPlugin && window.sendParameterToPlugin(key, state[key]);
  }
  
  setVal(state[key]||0);
  el.addEventListener('pointerdown',e=>{ dragging=true; startY=e.clientY; startVal=state[key]; e.preventDefault(); });
  el.addEventListener('pointermove',e=>{ if(!dragging)return; const dy=startY-e.clientY; setVal(startVal+(dy/150)*(max-min)); });
  const end=()=>{ dragging=false; };
  el.addEventListener('pointerup',end); 
  el.addEventListener('pointercancel',end);
  el.addEventListener('wheel',e=>{ e.preventDefault(); const step=(max-min)/100; setVal(state[key]+(e.deltaY<0?step:-step)); },{passive:false});
  
  return setVal;
}

const setInput = makeKnob('knob-in','val-in','inDb',-12,12,v=>`${v.toFixed(1)} dB`);
const setOutput = makeKnob('knob-out','val-out','outDb',-24,6,v=>`${v.toFixed(1)} dB`);
const setBias = makeKnob('knob-bias','val-bias','bias',-5,5,v=>`${v.toFixed(1)} dB`);

// Buttons
function setupButtonGroup(groupId,key){
  const group=$(groupId);
  group.addEventListener('click',(e)=>{
    if(e.target.tagName!=='BUTTON')return;
    const val=parseInt(e.target.dataset[key]);
    state[key]=val;
    group.querySelectorAll('button').forEach(b=>b.classList.remove('active'));
    e.target.classList.add('active');
    window.sendParameterToPlugin && window.sendParameterToPlugin(key, val);
    updateChannelLamps();
  });
}

setupButtonGroup('monitor','monitor');
setupButtonGroup('headblock','headblock');
setupButtonGroup('speed','speed');
setupButtonGroup('flux','flux');
setupButtonGroup('eq','eq');
setupButtonGroup('tape','tapetype');

// Toggles
function setupToggle(id,key){
  const el=$(id);
  el.addEventListener('click',()=>{
    state[key]=!state[key];
    el.classList.toggle('active',state[key]);
    window.sendParameterToPlugin && window.sendParameterToPlugin(key, state[key]);
  });
}

setupToggle('transformer','transformer');
setupToggle('autocal','autoCal');

// Tech panel
$('toggle-tech').addEventListener('click',()=>{
  const panel=$('tech-panel');
  const open=panel.classList.contains('open');
  if(open){
    panel.classList.remove('open');
    $('toggle-tech').textContent="Technician's Setup Panel ▼";
  }else{
    panel.classList.add('open');
    $('toggle-tech').textContent="Technician's Setup Panel ▲";
  }
});

// VU meter update from C++
window.updateVUMeters = function(dbL, dbR) {
  const needle=db=>Math.max(-70,Math.min(20,(db+50)*2.25-70));
  $('vuL').style.transform=`rotate(${needle(dbL)}deg)`;
  $('vuR').style.transform=`rotate(${needle(dbR)}deg)`;
};

// Parameter updates from C++ (automation, preset load)
window.setParameter = function(name, value) {
  if(name==='inputGain'){ setInput(value); }
  else if(name==='outputGain'){ setOutput(value); }
  else if(name==='bias'){ setBias(value); }
  else if(name==='monitor'){
    state.monitor=value;
    $('monitor').querySelectorAll('button').forEach(b=>{
      b.classList.toggle('active',parseInt(b.dataset.monitor)===value);
    });
  }
  else if(name==='speed'){
    state.speed=value;
    $('speed').querySelectorAll('button').forEach(b=>{
      b.classList.toggle('active',parseInt(b.dataset.speed)===value);
    });
  }
  else if(name==='flux'){
    state.flux=value;
    $('flux').querySelectorAll('button').forEach(b=>{
      b.classList.toggle('active',parseInt(b.dataset.flux)===value);
    });
  }
  else if(name==='eq'){
    state.eq=value;
    $('eq').querySelectorAll('button').forEach(b=>{
      b.classList.toggle('active',parseInt(b.dataset.eq)===value);
    });
  }
  else if(name==='tapeType'){
    state.tapeType=value;
    $('tape').querySelectorAll('button').forEach(b=>{
      b.classList.toggle('active',parseInt(b.dataset.tapetype)===value);
    });
  }
  else if(name==='transformer'){
    state.transformer=value;
    $('transformer').classList.toggle('active',value);
  }
  else if(name==='headblock'){
    state.headblock=value;
    $('headblock').querySelectorAll('button').forEach(b=>{
      b.classList.toggle('active',parseInt(b.dataset.headblock)===value);
    });
    updateChannelLamps();
  }
  else if(name==='autoCal'){
    state.autoCal=value;
    $('autocal').classList.toggle('active',value);
  }
};

function updateChannelLamps(){
  const isMono=(state.headblock===1);
  $('lamp-ch2').classList.toggle('on',!isMono);
  $('lamp-ch2').classList.toggle('green',!isMono);
  $('vuRwrap').classList.toggle('vu-dim',isMono);
  $('vuLabelR').textContent=isMono?'VU - R (idle)':'VU - R';
}

updateChannelLamps();

// Send UI changes to C++ plugin
window.sendParameterToPlugin = function(key, value) {
  // This will be called when UI changes, need to notify JUCE
  const paramMap = {
    'inDb': 'inputGain',
    'outDb': 'outputGain',
    'bias': 'bias',
    'monitor': 'monitor',
    'speed': 'speed',
    'flux': 'flux',
    'eq': 'eq',
    'tapetype': 'tapeType',
    'transformer': 'transformer',
    'headblock': 'headblock',
    'autoCal': 'autoCal'
  };
  
  const paramId = paramMap[key];
  if (paramId && window.juce) {
    window.juce.parameterChanged(paramId, value);
  }
};

})();
</script>
</body>
</html>
)";
}
