#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalogExactAudioProcessorEditor::AnalogExactAudioProcessorEditor (AnalogExactAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (900, 750);
    setResizable (false, false);
    
    // Generate HTML content
    htmlContent = generateHTML();
    
    // Create WebView with ResourceProvider
    juce::WebBrowserComponent::Options options;
    
    auto resourceProvider = [this] (const juce::String& url) 
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        // Serve index.html for root
        if (url == "/" || url.isEmpty() || url.endsWith ("index.html"))
        {
            return juce::WebBrowserComponent::Resource {
                juce::MemoryBlock (htmlContent.toRawUTF8(), htmlContent.getNumBytesAsUTF8()),
                "text/html"
            };
        }
        return std::nullopt;
    };
    
    options = options.withResourceProvider (resourceProvider)
                     .withKeepPageLoadedWhenBrowserIsHidden();
    
    webView = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (webView.get());
    
    // Navigate to the resource provider root
    webView->goToURL (webView->getResourceProviderRoot());
    
    // Mark ready after delay
    juce::Timer::callAfterDelay (800, [this]() {
        webViewReady = true;
        updateWebViewParameters();
    });
    
    startTimerHz (30);
}

AnalogExactAudioProcessorEditor::~AnalogExactAudioProcessorEditor()
{
    stopTimer();
    webViewReady = false;
}

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
    
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        webView->evaluateJavascript (script);
    }
    else
    {
        juce::WeakReference<AnalogExactAudioProcessorEditor> weakThis (this);
        juce::MessageManager::callAsync ([weakThis, script]() {
            if (auto* editor = weakThis.get())
                if (editor->webView != nullptr && editor->webViewReady.load())
                    editor->webView->evaluateJavascript (script);
        });
    }
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
    
    safeEvaluateJS (juce::String::formatted (
        "if(typeof updateVU==='function')updateVU(%.1f,%.1f);", dbL, dbR));
    
    updateWebViewParameters();
}

void AnalogExactAudioProcessorEditor::updateWebViewParameters()
{
    if (!webViewReady.load() || webView == nullptr)
        return;
    
    auto& vts = audioProcessor.getValueTreeState();
    
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
    
    const float eps = 0.05f;
    
    if (std::abs(inputGain - lastInputGain) > eps) {
        safeEvaluateJS(juce::String::formatted("if(typeof setP==='function')setP('in',%.2f);", inputGain));
        lastInputGain = inputGain;
    }
    if (std::abs(outputGain - lastOutputGain) > eps) {
        safeEvaluateJS(juce::String::formatted("if(typeof setP==='function')setP('out',%.2f);", outputGain));
        lastOutputGain = outputGain;
    }
    if (std::abs(bias - lastBias) > eps) {
        safeEvaluateJS(juce::String::formatted("if(typeof setP==='function')setP('bias',%.2f);", bias));
        lastBias = bias;
    }
    if (monitor != lastMonitor) {
        safeEvaluateJS(juce::String::formatted("if(typeof setP==='function')setP('mon',%d);", monitor));
        lastMonitor = monitor;
    }
    if (speed != lastSpeed) {
        safeEvaluateJS(juce::String::formatted("if(typeof setP==='function')setP('spd',%d);", speed));
        lastSpeed = speed;
    }
    if (flux != lastFlux) {
        safeEvaluateJS(juce::String::formatted("if(typeof setP==='function')setP('flx',%d);", flux));
        lastFlux = flux;
    }
    if (eq != lastEQ) {
        safeEvaluateJS(juce::String::formatted("if(typeof setP==='function')setP('eq',%d);", eq));
        lastEQ = eq;
    }
    if (tapeType != lastTapeType) {
        safeEvaluateJS(juce::String::formatted("if(typeof setP==='function')setP('tape',%d);", tapeType));
        lastTapeType = tapeType;
    }
    if (transformer != lastTransformer) {
        safeEvaluateJS(juce::String::formatted("if(typeof setP==='function')setP('xfmr',%s);", transformer?"true":"false"));
        lastTransformer = transformer;
    }
    if (headblock != lastHeadblock) {
        safeEvaluateJS(juce::String::formatted("if(typeof setP==='function')setP('hb',%d);", headblock));
        lastHeadblock = headblock;
    }
    if (autoCal != lastAutoCal) {
        safeEvaluateJS(juce::String::formatted("if(typeof setP==='function')setP('cal',%s);", autoCal?"true":"false"));
        lastAutoCal = autoCal;
    }
}

juce::String AnalogExactAudioProcessorEditor::generateHTML()
{
    return R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ANALOGEXACT</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
html,body{height:100%;background:#1a202c;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;color:#e5e7eb;overflow:hidden;user-select:none}
.panel{background:linear-gradient(145deg,#4a5568,#3a4454);border:1px solid #718096;border-top-color:#a0aec0;border-radius:16px;padding:16px;margin:8px;height:calc(100% - 16px);display:flex;flex-direction:column}
.hdr{border-bottom:1px solid rgba(0,0,0,.3);padding-bottom:12px;margin-bottom:12px}
.hdr h1{font-size:24px;font-weight:700;color:#fff;letter-spacing:2px}
.hdr p{font-size:11px;color:#9ca3af;margin-top:4px}
.row{display:flex;gap:12px;margin-bottom:12px}
.vu-wrap{flex:1;background:#111827;border:4px solid #4b5563;border-radius:10px;padding:12px}
.vu-hdr{display:flex;justify-content:space-between;font-size:10px;margin-bottom:6px}
.vu-hdr span:first-child{color:#d1d5db}
.vu-hdr span:last-child{color:#6b7280}
.vu-scale{height:55px;background:#ffebcd;border-radius:4px;position:relative;overflow:hidden}
.vu-needle{position:absolute;width:2px;height:100%;background:#dc2626;left:50%;bottom:0;transform-origin:bottom center;transition:transform 50ms linear;box-shadow:0 0 6px #dc2626}
.vu-dim{opacity:.4;filter:grayscale(.4)}
.lamps{display:flex;gap:16px;padding:0 4px;margin-bottom:12px}
.lamp-item{display:flex;align-items:center;gap:6px}
.lamp{width:10px;height:10px;border-radius:50%;background:#374151;box-shadow:inset 0 0 3px rgba(0,0,0,.6)}
.lamp.grn{background:#34d399;box-shadow:0 0 8px #34d399}
.lamp.red{background:#f87171;box-shadow:0 0 8px #f87171}
.lamp-item span{font-size:10px;color:#9ca3af}
.mods{display:flex;gap:12px;margin-bottom:12px}
.mod{flex:1;background:rgba(0,0,0,.25);border:1px solid rgba(0,0,0,.4);border-radius:8px;padding:14px;text-align:center}
.mod h2{font-size:13px;font-weight:700;margin-bottom:10px;color:#e5e7eb}
.knob{width:72px;height:72px;border-radius:50%;background:linear-gradient(145deg,#5a6578,#2d3748);border:2px solid #1a202c;box-shadow:0 4px 12px rgba(0,0,0,.5),inset 0 2px 4px rgba(255,255,255,.05);margin:0 auto;position:relative;cursor:pointer;touch-action:none}
.knob-ind{position:absolute;width:4px;height:14px;background:#e2e8f0;border-radius:2px;top:5px;left:50%;margin-left:-2px;transform-origin:center 31px;box-shadow:0 0 4px rgba(255,255,255,.4)}
.knob-val{font-size:12px;color:#fcd34d;margin-top:8px;font-family:monospace}
.btns{display:flex;gap:4px}
.btn{flex:1;background:#374151;color:#e5e7eb;border:1px solid #1f2937;border-radius:6px;padding:8px 6px;font-size:11px;font-weight:600;cursor:pointer;transition:all .15s}
.btn.act{background:#f6ad55;color:#1a202c;box-shadow:inset 0 2px 4px rgba(0,0,0,.3)}
.tech-btn{background:none;border:none;color:#fcd34d;font-size:11px;font-weight:600;cursor:pointer;padding:8px;width:100%;text-align:center}
.tech{max-height:0;opacity:0;overflow:hidden;transition:max-height .4s ease,opacity .3s ease}
.tech.open{max-height:400px;opacity:1}
.tech-inner{background:rgba(0,0,0,.25);border:1px solid rgba(0,0,0,.4);border-radius:8px;padding:14px}
.tech-row{display:flex;gap:12px;margin-bottom:12px}
.tech-col{flex:1}
.tech-col h3{font-size:10px;font-weight:700;text-align:center;margin-bottom:6px;color:#d1d5db}
.tape-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:4px}
.toggles{display:flex;justify-content:center;gap:24px;margin-top:8px}
.tog-item{display:flex;align-items:center;gap:8px}
.tog{width:46px;height:24px;background:#4a5568;border-radius:12px;cursor:pointer;position:relative;border:1px solid #1a202c;transition:background .2s}
.tog.act{background:#f6ad55}
.tog-handle{position:absolute;top:2px;left:2px;width:18px;height:18px;background:#fff;border-radius:50%;box-shadow:0 1px 3px rgba(0,0,0,.4);transition:transform .2s}
.tog.act .tog-handle{transform:translateX(22px)}
.tog-item span{font-size:10px;font-weight:600;color:#d1d5db}
</style>
</head>
<body>
<div class="panel">
<div class="hdr">
<h1>ANALOGEXACT</h1>
<p>INPUT monitor = true hard bypass (unity, completely dry) • Non-linear core @ 768 kHz (120 taps)</p>
</div>
<div class="row">
<div class="vu-wrap" id="vuLwrap">
<div class="vu-hdr"><span id="vuLbl">VU - L</span><span>-20 -10 0 +3 +5</span></div>
<div class="vu-scale"><div class="vu-needle" id="vuL"></div></div>
</div>
<div class="vu-wrap" id="vuRwrap">
<div class="vu-hdr"><span id="vuRbl">VU - R</span><span>-20 -10 0 +3 +5</span></div>
<div class="vu-scale"><div class="vu-needle" id="vuR"></div></div>
</div>
</div>
<div class="lamps">
<div class="lamp-item"><div class="lamp grn"></div><span>READY</span></div>
<div class="lamp-item"><div class="lamp"></div><span>PLAY</span></div>
<div class="lamp-item"><div class="lamp red"></div><span>STOP</span></div>
<div style="border-left:1px solid rgba(255,255,255,.1);margin:0 8px"></div>
<div class="lamp-item"><div class="lamp grn" id="lCh1"></div><span>CH-1</span></div>
<div class="lamp-item"><div class="lamp grn" id="lCh2"></div><span>CH-2</span></div>
</div>
<div class="mods">
<div class="mod">
<h2>INPUT LEVEL</h2>
<div class="knob" id="kIn"><div class="knob-ind" id="indIn"></div></div>
<div class="knob-val" id="vIn">0.0 dB</div>
</div>
<div class="mod">
<h2>OUTPUT LEVEL</h2>
<div class="knob" id="kOut"><div class="knob-ind" id="indOut"></div></div>
<div class="knob-val" id="vOut">0.0 dB</div>
</div>
<div class="mod">
<h2>BIAS ADJUST</h2>
<div class="knob" id="kBias"><div class="knob-ind" id="indBias"></div></div>
<div class="knob-val" id="vBias">0.0 dB</div>
</div>
</div>
<div class="row">
<div class="mod" style="flex:1">
<h2>MONITOR</h2>
<div class="btns" id="gMon">
<button class="btn act" data-v="0">INPUT</button>
<button class="btn" data-v="1">REPRO</button>
</div>
</div>
<div class="mod" style="flex:1">
<h2>HEADBLOCK</h2>
<div class="btns" id="gHb">
<button class="btn act" data-v="0">STEREO 2-TRK</button>
<button class="btn" data-v="1">FULL-TRACK MONO</button>
</div>
</div>
</div>
<button class="tech-btn" id="techBtn">Technician's Setup Panel ▼</button>
<div class="tech" id="tech">
<div class="tech-inner">
<div class="tech-row">
<div class="tech-col">
<h3>SPEED (IPS)</h3>
<div class="btns" id="gSpd">
<button class="btn" data-v="0">7.5</button>
<button class="btn act" data-v="1">15</button>
<button class="btn" data-v="2">30</button>
</div>
</div>
<div class="tech-col">
<h3>FLUXIVITY (nWb/m)</h3>
<div class="btns" id="gFlx">
<button class="btn" data-v="0">185</button>
<button class="btn act" data-v="1">250</button>
<button class="btn" data-v="2">370</button>
</div>
</div>
<div class="tech-col">
<h3>EQ STANDARD</h3>
<div class="btns" id="gEq">
<button class="btn act" data-v="0">NAB</button>
<button class="btn" data-v="1">IEC/CCIR</button>
</div>
</div>
</div>
<div class="tech-col">
<h3>TAPE TYPE</h3>
<div class="tape-grid" id="gTape">
<button class="btn" data-v="0">406</button>
<button class="btn act" data-v="1">456</button>
<button class="btn" data-v="2">499</button>
<button class="btn" data-v="3">GP9</button>
<button class="btn" data-v="4">SM900</button>
<button class="btn" data-v="5">SM911</button>
</div>
</div>
<div class="toggles">
<div class="tog-item">
<div class="tog act" id="tXfmr"><div class="tog-handle"></div></div>
<span>XFORMER I/O</span>
</div>
<div class="tog-item">
<div class="tog act" id="tCal"><div class="tog-handle"></div></div>
<span>AUTO CAL</span>
</div>
</div>
</div>
</div>
</div>
<script>
(function(){
var $=function(id){return document.getElementById(id)};
var st={inDb:0,outDb:0,bias:0,mon:0,spd:1,flx:1,eq:0,tape:1,xfmr:1,hb:0,cal:1};

function updLamps(){
  var m=st.hb===1;
  $('lCh2').classList.toggle('grn',!m);
  $('vuRwrap').classList.toggle('vu-dim',m);
  $('vuRbl').textContent=m?'VU - R (idle)':'VU - R';
}

function mkKnob(kid,iid,vid,key,mn,mx){
  var knob=$(kid),ind=$(iid),vEl=$(vid);
  var drag=false,startY=0,startV=0;
  function setV(v){
    v=Math.round(Math.min(mx,Math.max(mn,v))*10)/10;
    st[key]=v;
    var pct=(v-mn)/(mx-mn);
    ind.style.transform='rotate('+(270*pct-135)+'deg)';
    vEl.textContent=v.toFixed(1)+' dB';
  }
  setV(st[key]);
  knob.addEventListener('pointerdown',function(e){
    drag=true;startY=e.clientY;startV=st[key];
    knob.setPointerCapture(e.pointerId);
    e.preventDefault();
  });
  knob.addEventListener('pointermove',function(e){
    if(!drag)return;
    var dy=startY-e.clientY;
    setV(startV+dy/120*(mx-mn));
  });
  knob.addEventListener('pointerup',function(e){drag=false;knob.releasePointerCapture(e.pointerId);});
  knob.addEventListener('pointercancel',function(){drag=false;});
  return setV;
}

var setIn=mkKnob('kIn','indIn','vIn','inDb',-12,12);
var setOut=mkKnob('kOut','indOut','vOut','outDb',-24,6);
var setBias=mkKnob('kBias','indBias','vBias','bias',-5,5);

function mkGrp(gid,key,cb){
  var g=$(gid);
  g.addEventListener('click',function(e){
    var btn=e.target.closest('.btn');
    if(!btn)return;
    var v=parseInt(btn.dataset.v);
    st[key]=v;
    g.querySelectorAll('.btn').forEach(function(b){
      b.classList.toggle('act',parseInt(b.dataset.v)===v);
    });
    if(cb)cb();
  });
}

mkGrp('gMon','mon');
mkGrp('gHb','hb',updLamps);
mkGrp('gSpd','spd');
mkGrp('gFlx','flx');
mkGrp('gEq','eq');
mkGrp('gTape','tape');

$('tXfmr').addEventListener('click',function(){
  st.xfmr=st.xfmr?0:1;
  this.classList.toggle('act',st.xfmr);
});
$('tCal').addEventListener('click',function(){
  st.cal=st.cal?0:1;
  this.classList.toggle('act',st.cal);
});

$('techBtn').addEventListener('click',function(){
  var t=$('tech');
  var open=t.classList.toggle('open');
  this.textContent=open?"Technician's Setup Panel \u25B2":"Technician's Setup Panel \u25BC";
});

window.updateVU=function(l,r){
  var needle=function(db){return Math.max(-70,Math.min(20,(db+50)*2.25-70));};
  $('vuL').style.transform='rotate('+needle(l)+'deg)';
  $('vuR').style.transform='rotate('+needle(r)+'deg)';
};

window.setP=function(k,v){
  if(k==='in'){setIn(v);}
  else if(k==='out'){setOut(v);}
  else if(k==='bias'){setBias(v);}
  else if(k==='mon'){
    st.mon=v;
    $('gMon').querySelectorAll('.btn').forEach(function(b){b.classList.toggle('act',parseInt(b.dataset.v)===v);});
  }
  else if(k==='spd'){
    st.spd=v;
    $('gSpd').querySelectorAll('.btn').forEach(function(b){b.classList.toggle('act',parseInt(b.dataset.v)===v);});
  }
  else if(k==='flx'){
    st.flx=v;
    $('gFlx').querySelectorAll('.btn').forEach(function(b){b.classList.toggle('act',parseInt(b.dataset.v)===v);});
  }
  else if(k==='eq'){
    st.eq=v;
    $('gEq').querySelectorAll('.btn').forEach(function(b){b.classList.toggle('act',parseInt(b.dataset.v)===v);});
  }
  else if(k==='tape'){
    st.tape=v;
    $('gTape').querySelectorAll('.btn').forEach(function(b){b.classList.toggle('act',parseInt(b.dataset.v)===v);});
  }
  else if(k==='xfmr'){
    st.xfmr=v?1:0;
    $('tXfmr').classList.toggle('act',v);
  }
  else if(k==='hb'){
    st.hb=v;
    $('gHb').querySelectorAll('.btn').forEach(function(b){b.classList.toggle('act',parseInt(b.dataset.v)===v);});
    updLamps();
  }
  else if(k==='cal'){
    st.cal=v?1:0;
    $('tCal').classList.toggle('act',v);
  }
};

updLamps();
})();
</script>
</body>
</html>
)HTML";
}
