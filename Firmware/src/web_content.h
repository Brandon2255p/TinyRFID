#ifndef WEB_CONTENT_H
#define WEB_CONTENT_H

#include <Arduino.h>

// Single-page web UI served from the ESP32. Card IDs are stored in the
// browser's localStorage (never on the device); the page POSTs a selected
// card to /api/transmit to start the EM4100 output.
static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>TinyRFID</title>
<style>
  :root{
    --bg:#f4f5f7; --card:#fff; --fg:#1b1f24; --muted:#6b7280;
    --border:#e2e5ea; --accent:#2563eb; --accent-fg:#fff;
    --danger:#dc2626; --ok:#16a34a; --chip:#eef2ff;
  }
  @media (prefers-color-scheme:dark){
    :root{
      --bg:#0f1115; --card:#171a21; --fg:#e8eaed; --muted:#9aa2ad;
      --border:#262b34; --accent:#3b82f6; --accent-fg:#fff;
      --danger:#f05252; --ok:#34d399; --chip:#1e2330;
    }
  }
  *{box-sizing:border-box}
  body{margin:0;background:var(--bg);color:var(--fg);
    font:15px/1.5 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
  header{padding:18px 16px;border-bottom:1px solid var(--border);
    display:flex;align-items:center;gap:12px;background:var(--card)}
  header h1{font-size:18px;margin:0;font-weight:650}
  .wrap{max-width:720px;margin:0 auto;padding:16px}
  .status{display:flex;align-items:center;gap:8px;font-size:13px;color:var(--muted)}
  .dot{width:9px;height:9px;border-radius:50%;background:var(--muted)}
  .dot.on{background:var(--ok);box-shadow:0 0 0 3px color-mix(in srgb,var(--ok) 25%,transparent)}
  .card{background:var(--card);border:1px solid var(--border);border-radius:12px;
    padding:14px;margin-bottom:12px}
  .card h2{margin:0 0 10px;font-size:14px;font-weight:650}
  label{display:block;font-size:12px;color:var(--muted);margin:8px 0 4px}
  input,select{width:100%;padding:9px 10px;border:1px solid var(--border);
    border-radius:8px;background:var(--bg);color:var(--fg);font:inherit}
  input:focus,select:focus{outline:2px solid var(--accent);outline-offset:-1px}
  .row{display:flex;gap:10px;flex-wrap:wrap}
  .row>div{flex:1;min-width:120px}
  button{font:inherit;font-weight:600;padding:9px 14px;border-radius:8px;
    border:1px solid var(--border);background:var(--card);color:var(--fg);cursor:pointer}
  button.primary{background:var(--accent);color:var(--accent-fg);border-color:transparent}
  button.danger{color:var(--danger)}
  button:active{transform:translateY(1px)}
  .btns{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}
  .list{list-style:none;margin:0;padding:0}
  .item{border:1px solid var(--border);border-radius:10px;padding:12px;margin-bottom:10px;
    display:flex;align-items:center;gap:12px;flex-wrap:wrap}
  .item.active{border-color:var(--ok);box-shadow:0 0 0 1px var(--ok)}
  .item .meta{flex:1;min-width:150px}
  .item .name{font-weight:650}
  .item .id{font-family:ui-monospace,Menlo,Consolas,monospace;color:var(--muted);font-size:13px}
  .chip{display:inline-block;font-size:11px;padding:2px 8px;border-radius:99px;
    background:var(--chip);color:var(--accent);margin-left:6px}
  .item .act{display:flex;gap:6px}
  .empty{color:var(--muted);text-align:center;padding:24px 0}
  .hint{font-size:12px;color:var(--muted);margin-top:6px}
  .toggle{display:flex;gap:8px;margin-top:4px}
  .toggle button{flex:1}
  .toggle button.sel{background:var(--accent);color:var(--accent-fg);border-color:transparent}
  .foot{color:var(--muted);font-size:12px;text-align:center;margin:18px 0}
</style>
</head>
<body>
<header>
  <h1>&#128225; TinyRFID</h1>
  <div class="status" style="margin-left:auto">
    <span id="dot" class="dot"></span><span id="stxt">idle</span>
  </div>
</header>

<div class="wrap">

  <div class="card">
    <h2 id="formTitle">Add card</h2>
    <label>Name</label>
    <input id="f_name" placeholder="Front door" maxlength="40">
    <label>Card ID (10 hex characters, EM4100)</label>
    <input id="f_id" placeholder="307FA2535A" maxlength="10"
           autocapitalize="characters" spellcheck="false">
    <div class="hint" id="id_hint">40-bit EM4100 ID, e.g. 307FA2535A</div>

    <label>Transmit rate</label>
    <div class="toggle">
      <button type="button" id="m_sweep" class="sel" onclick="setMode('sweep')">Sweep (compatible)</button>
      <button type="button" id="m_fixed" onclick="setMode('fixed')">Fixed rate</button>
    </div>

    <div id="sweepFields">
      <div class="row">
        <div><label>Sweep min (&micro;s)</label><input id="f_smin" type="number" value="200" min="50" max="2000"></div>
        <div><label>Sweep max (&micro;s)</label><input id="f_smax" type="number" value="220" min="50" max="2000"></div>
      </div>
      <div class="hint">Half-bit delay is swept across the range to match readers with slightly different clocks (default 200&ndash;220&micro;s).</div>
    </div>

    <div id="fixedFields" style="display:none">
      <label>Fixed half-bit delay (&micro;s)</label>
      <input id="f_fixed" type="number" value="200" min="50" max="2000">
      <div class="hint">Uses one exact rate for this card.</div>
    </div>

    <label>Frames per step</label>
    <input id="f_repeat" type="number" value="20" min="1" max="1000">

    <div class="btns">
      <button class="primary" onclick="saveCard()" id="saveBtn">Add card</button>
      <button onclick="resetForm()" id="cancelBtn" style="display:none">Cancel</button>
    </div>
  </div>

  <div class="card">
    <h2>Stored cards</h2>
    <ul class="list" id="list"></ul>
    <div class="btns">
      <button onclick="stopTx()" class="danger">Stop transmit</button>
      <button onclick="exportCards()">Export</button>
      <button onclick="document.getElementById('imp').click()">Import</button>
      <input id="imp" type="file" accept="application/json" style="display:none" onchange="importCards(event)">
    </div>
    <div class="hint">Cards are stored in this browser only (localStorage) and sent to the device when you tap Transmit.</div>
  </div>

  <div class="foot">TinyRFID &middot; ESP32 EM4100 emulator</div>
</div>

<script>
const KEY='tinyrfid_cards';
let cards=[];
let editId=null;
let formMode='sweep';

function load(){ try{cards=JSON.parse(localStorage.getItem(KEY))||[]}catch(e){cards=[]} }
function save(){ try{localStorage.setItem(KEY,JSON.stringify(cards))}catch(e){} }
function uid(){ return Date.now().toString(36)+Math.random().toString(36).slice(2,6) }

function setMode(m){
  formMode=m;
  document.getElementById('m_sweep').classList.toggle('sel',m==='sweep');
  document.getElementById('m_fixed').classList.toggle('sel',m==='fixed');
  document.getElementById('sweepFields').style.display=m==='sweep'?'':'none';
  document.getElementById('fixedFields').style.display=m==='fixed'?'':'none';
}

function validId(v){ return /^[0-9a-fA-F]{10}$/.test(v) }

function saveCard(){
  const name=document.getElementById('f_name').value.trim()||'Unnamed';
  const id=document.getElementById('f_id').value.trim().toUpperCase();
  if(!validId(id)){ alert('Card ID must be exactly 10 hex characters.'); return; }
  const c={
    id: editId||uid(),
    name, cardId:id, mode:formMode,
    sweepMin:+document.getElementById('f_smin').value||200,
    sweepMax:+document.getElementById('f_smax').value||220,
    fixedDelay:+document.getElementById('f_fixed').value||200,
    repeat:+document.getElementById('f_repeat').value||20
  };
  if(editId){ cards=cards.map(x=>x.id===editId?c:x); }
  else cards.push(c);
  save(); resetForm(); render();
}

function editCard(id){
  const c=cards.find(x=>x.id===id); if(!c)return;
  editId=id;
  document.getElementById('f_name').value=c.name;
  document.getElementById('f_id').value=c.cardId;
  document.getElementById('f_smin').value=c.sweepMin;
  document.getElementById('f_smax').value=c.sweepMax;
  document.getElementById('f_fixed').value=c.fixedDelay;
  document.getElementById('f_repeat').value=c.repeat;
  setMode(c.mode);
  document.getElementById('formTitle').textContent='Edit card';
  document.getElementById('saveBtn').textContent='Save changes';
  document.getElementById('cancelBtn').style.display='';
  window.scrollTo({top:0,behavior:'smooth'});
}

function resetForm(){
  editId=null;
  document.getElementById('f_name').value='';
  document.getElementById('f_id').value='';
  document.getElementById('f_smin').value=200;
  document.getElementById('f_smax').value=220;
  document.getElementById('f_fixed').value=200;
  document.getElementById('f_repeat').value=20;
  setMode('sweep');
  document.getElementById('formTitle').textContent='Add card';
  document.getElementById('saveBtn').textContent='Add card';
  document.getElementById('cancelBtn').style.display='none';
}

function delCard(id){
  if(!confirm('Delete this card?'))return;
  cards=cards.filter(x=>x.id!==id); save(); render();
}

let activeCardId=null;

function txCard(id){
  const c=cards.find(x=>x.id===id); if(!c)return;
  const body=new URLSearchParams({
    id:c.cardId, mode:c.mode,
    sweepMin:c.sweepMin, sweepMax:c.sweepMax,
    fixedDelay:c.fixedDelay, repeat:c.repeat
  });
  fetch('/api/transmit',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body})
    .then(r=>r.json()).then(s=>{activeCardId=id;applyStatus(s);render();})
    .catch(()=>alert('Could not reach device.'));
}

function stopTx(){
  fetch('/api/stop',{method:'POST'}).then(r=>r.json())
    .then(s=>{activeCardId=null;applyStatus(s);render();}).catch(()=>{});
}

function applyStatus(s){
  const on=s&&s.active;
  document.getElementById('dot').className='dot'+(on?' on':'');
  document.getElementById('stxt').textContent=on
    ? `transmitting ${s.id} (${s.mode}, ${s.delay}µs)` : 'idle';
  if(!on) activeCardId=null;
}

function poll(){
  fetch('/api/status').then(r=>r.json()).then(s=>{
    applyStatus(s);
    // keep active highlight in sync with device
    if(s.active){ const m=cards.find(c=>c.cardId===s.id); if(m)activeCardId=m.id; }
    render();
  }).catch(()=>{});
}

function render(){
  const ul=document.getElementById('list');
  if(!cards.length){ ul.innerHTML='<div class="empty">No cards yet. Add one above.</div>'; return; }
  ul.innerHTML='';
  cards.forEach(c=>{
    const li=document.createElement('li');
    li.className='item'+(c.id===activeCardId?' active':'');
    const rate=c.mode==='sweep'
      ? `sweep ${c.sweepMin}–${c.sweepMax}µs`
      : `fixed ${c.fixedDelay}µs`;
    li.innerHTML=`<div class="meta">
        <div class="name">${esc(c.name)}<span class="chip">${rate}</span></div>
        <div class="id">${esc(c.cardId)}</div>
      </div>
      <div class="act">
        <button class="primary" data-tx>Transmit</button>
        <button data-ed>Edit</button>
        <button class="danger" data-del>Del</button>
      </div>`;
    li.querySelector('[data-tx]').onclick=()=>txCard(c.id);
    li.querySelector('[data-ed]').onclick=()=>editCard(c.id);
    li.querySelector('[data-del]').onclick=()=>delCard(c.id);
    ul.appendChild(li);
  });
}

function esc(s){return String(s).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]))}

function exportCards(){
  const blob=new Blob([JSON.stringify(cards,null,2)],{type:'application/json'});
  const a=document.createElement('a');
  a.href=URL.createObjectURL(blob); a.download='tinyrfid-cards.json'; a.click();
  URL.revokeObjectURL(a.href);
}
function importCards(ev){
  const f=ev.target.files[0]; if(!f)return;
  const r=new FileReader();
  r.onload=()=>{ try{
    const arr=JSON.parse(r.result);
    if(Array.isArray(arr)){ arr.forEach(c=>{if(!c.id)c.id=uid()}); cards=arr; save(); render(); }
  }catch(e){ alert('Invalid file.'); } };
  r.readAsText(f); ev.target.value='';
}

load(); render(); setMode('sweep'); poll(); setInterval(poll,2000);
</script>
</body>
</html>)HTML";

#endif
