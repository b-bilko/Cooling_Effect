#pragma once

// The portal's single page. Served from flash; talks to /api/* with fetch().

static const char PORTAL_PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Cooling Effect</title>
<style>
:root {
  --bg: #faf7f5; --card: #fff; --text: #241c18; --muted: #6f625b;
  --line: #e6ddd7; --accent: #c2410c; --accent-text: #fff;
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #161210; --card: #211b18; --text: #f3ece7; --muted: #a8998f;
    --line: #3a302b; --accent: #f97316; --accent-text: #1a0f08;
  }
}
* { box-sizing: border-box; }
body {
  margin: 0; padding: 16px; background: var(--bg); color: var(--text);
  font: 16px/1.45 system-ui, -apple-system, "Segoe UI", sans-serif;
}
main { max-width: 480px; margin: 0 auto; }
h1 { font-size: 1.4rem; margin: 4px 0 12px; }
h2 { font-size: 1rem; margin: 0 0 12px; }
section {
  background: var(--card); border: 1px solid var(--line);
  border-radius: 12px; padding: 16px; margin-bottom: 16px;
}
#swatch {
  height: 56px; border-radius: 8px; border: 1px solid var(--line);
  margin-bottom: 10px; transition: background .5s;
}
.bar { height: 6px; background: var(--line); border-radius: 3px; overflow: hidden; }
.bar > div { height: 100%; width: 0; background: var(--accent); transition: width .5s; }
.status { color: var(--muted); font-size: .9rem; margin: 8px 0 0; }
label { display: block; margin: 12px 0 4px; font-weight: 600; font-size: .92rem; }
label span { font-weight: 400; color: var(--muted); }
input[type=number], input[type=text], input[type=password] {
  width: 100%; padding: 10px; font: inherit; color: var(--text);
  background: var(--bg); border: 1px solid var(--line); border-radius: 8px;
}
input[type=range] { width: 100%; accent-color: var(--accent); }
.row { display: flex; gap: 8px; margin-top: 14px; flex-wrap: wrap; }
button {
  flex: 1 1 auto; padding: 10px 14px; font: inherit; font-weight: 600;
  border-radius: 8px; border: 1px solid var(--line); cursor: pointer;
  background: var(--card); color: var(--text);
}
button.primary { background: var(--accent); color: var(--accent-text); border-color: var(--accent); }
.note { color: var(--muted); font-size: .85rem; margin: 6px 0 0; }
#msg { min-height: 1.4em; font-size: .9rem; }
</style>
</head>
<body>
<main>
<h1>Cooling Effect</h1>

<section>
  <div id="swatch"></div>
  <div class="bar"><div id="progress"></div></div>
  <p class="status" id="status">Connecting…</p>
  <div class="row">
    <button class="primary" id="restart">Restart fade</button>
    <button id="sleep">Sleep now</button>
  </div>
</section>

<section>
  <h2>Fade</h2>
  <form id="fade">
    <label for="fadeMinutes">Length <span>(minutes)</span></label>
    <input type="number" id="fadeMinutes" min="0.5" max="240" step="0.5" required>

    <label for="startHeat">Starting color <span id="startHeatVal"></span></label>
    <input type="range" id="startHeat" min="1" max="255">

    <label for="brightness">Brightness <span id="brightnessVal"></span></label>
    <input type="range" id="brightness" min="1" max="255">

    <label for="maxMilliamps">Power limit <span>(mA at 5 V)</span></label>
    <input type="number" id="maxMilliamps" min="100" max="5000" step="50" required>

    <label for="sleepDelaySeconds">Stay awake after fade <span>(seconds)</span></label>
    <input type="number" id="sleepDelaySeconds" min="0" max="3600" step="5" required>
    <p class="note">The device powers down once the fade ends and this delay
    passes, unless this page is open or a device is joined to its Wi-Fi.
    Press the button to wake it.</p>

    <div class="row"><button class="primary" type="submit">Save</button></div>
  </form>
</section>

<section>
  <h2>Wi-Fi</h2>
  <form id="wifi">
    <p class="note" id="wifiInfo"></p>
    <label for="wifiSsid">Home network <span>(optional, blank to disable)</span></label>
    <input type="text" id="wifiSsid" maxlength="32" autocomplete="off">

    <label for="wifiPassword">Home network password <span>(blank keeps current)</span></label>
    <input type="password" id="wifiPassword" maxlength="63" autocomplete="new-password">

    <label for="apPassword">Device access point password <span>(8+ chars, blank keeps current)</span></label>
    <input type="password" id="apPassword" minlength="8" maxlength="63" autocomplete="new-password">

    <div class="row"><button type="submit">Save &amp; reboot</button></div>
  </form>
</section>

<p id="msg" role="status"></p>
</main>

<script>
// FastLED HeatColors_p, interpolated like ColorFromPalette
const HEAT = [0x000000,0x330000,0x660000,0x990000,0xCC0000,0xFF0000,0xFF3300,0xFF6600,
              0xFF9900,0xFFCC00,0xFFFF00,0xFFFF33,0xFFFF66,0xFFFF99,0xFFFFCC,0xFFFFFF];
function heatColor(i) {
  const pos = i / 16, a = Math.floor(pos), f = pos - a;
  const c1 = HEAT[a], c2 = HEAT[(a + 1) % 16];
  const ch = s => Math.round(((c1 >> s) & 255) * (1 - f) + ((c2 >> s) & 255) * f);
  return `rgb(${ch(16)},${ch(8)},${ch(0)})`;
}

const $ = id => document.getElementById(id);
let loaded = false;

function fmt(s) {
  const m = Math.floor(s / 60), r = s % 60;
  return m ? `${m}m ${String(r).padStart(2, "0")}s` : `${r}s`;
}

function show(msg) { $("msg").textContent = msg; }

function render(st) {
  $("swatch").style.background = heatColor(st.heat);
  $("progress").style.width = st.percent + "%";
  let text = st.cooling ? `Cooling — ${fmt(st.secondsLeft)} left` : "Fade finished";
  if (!st.cooling) {
    text += st.sleepInSeconds < 0 ? " · staying awake while you're connected"
                                  : ` · sleeping in ${fmt(st.sleepInSeconds)}`;
  }
  $("status").textContent = text;

  let info = `Device network: ${st.apSsid} (http://192.168.4.1)`;
  if (st.wifiSsid) {
    info += st.staIp ? ` · on ${st.wifiSsid} at http://${st.staIp} or http://cooling-effect.local`
                     : ` · not connected to ${st.wifiSsid}`;
  }
  $("wifiInfo").textContent = info;

  if (!loaded) {
    loaded = true;
    $("fadeMinutes").value = st.fadeSeconds / 60;
    $("startHeat").value = st.startHeat;
    $("brightness").value = st.brightness;
    $("maxMilliamps").value = st.maxMilliamps;
    $("sleepDelaySeconds").value = st.sleepDelaySeconds;
    $("wifiSsid").value = st.wifiSsid;
    updateLabels();
  }
}

function updateLabels() {
  $("startHeatVal").textContent = $("startHeat").value;
  $("startHeatVal").style.color = heatColor(+$("startHeat").value);
  $("brightnessVal").textContent = Math.round($("brightness").value / 2.55) + "%";
}
$("startHeat").oninput = updateLabels;
$("brightness").oninput = updateLabels;

async function post(path, fields) {
  const res = await fetch(path, { method: "POST", body: new URLSearchParams(fields) });
  if (!res.ok) throw new Error(await res.text());
  return res;
}

async function poll() {
  try {
    const res = await fetch("/api/state");
    render(await res.json());
  } catch (e) {
    $("status").textContent = "Device unreachable (asleep?)";
  }
}

$("restart").onclick = async () => {
  render(await (await post("/api/restart", {})).json());
  show("Fade restarted.");
};

$("sleep").onclick = async () => {
  if (!confirm("Turn the lights off and power down? Press the button to wake it.")) return;
  show(await (await post("/api/sleep", {})).text());
};

$("fade").onsubmit = async e => {
  e.preventDefault();
  try {
    const st = await (await post("/api/settings", {
      fadeSeconds: Math.round($("fadeMinutes").value * 60),
      startHeat: $("startHeat").value,
      brightness: $("brightness").value,
      maxMilliamps: $("maxMilliamps").value,
      sleepDelaySeconds: $("sleepDelaySeconds").value,
    })).json();
    render(st);
    show("Saved.");
  } catch (err) { show(err.message); }
};

$("wifi").onsubmit = async e => {
  e.preventDefault();
  try {
    show(await (await post("/api/wifi", {
      wifiSsid: $("wifiSsid").value,
      wifiPassword: $("wifiPassword").value,
      apPassword: $("apPassword").value,
    })).text());
  } catch (err) { show(err.message); }
};

poll();
setInterval(poll, 2000);
</script>
</body>
</html>
)HTML";
