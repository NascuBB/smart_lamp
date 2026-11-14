#include "HtmlResponses.h"

#include <memory>

const char html_template_p1[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Умный светильник</title>
  <style>
    :root {
      --primary: #6c5ce7;
      --primary-dark: #5649c0;
      --bg: #1a1a2e;
      --bg-light: #16213e;
      --text: #e6e6e6;
      --text-dim: #b8b8b8;
      --success: #00b894;
      --warning: #fdcb6e;
      --danger: #d63031;
    }

    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    }

    body {
      background-color: var(--bg);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
    }

    header {
      background-color: var(--bg-light);
      padding: 1rem;
      box-shadow: 0 2px 10px rgba(0, 0, 0, 0.3);
    }

    nav {
      display: flex;
      justify-content: end;
      gap: 1rem;
    }

    .nav-btn {
      background: none;
      border: none;
      color: var(--text);
      padding: 0.5rem 1rem;
      border-radius: 4px;
      cursor: pointer;
      font-size: 1rem;
      transition: all 0.2s;
      text-decoration: none;
    }

    .nav-btn:hover {
      background-color: rgba(108, 92, 231, 0.2);
    }

    .nav-btn.active {
      background-color: var(--primary);
    }

    main {
      flex: 1;
      padding: 1rem;
      max-width: 800px;
      margin: 0 auto;
      width: 100%;
    }

    .page {
      animation: fadeIn 0.3s ease;
      transition: 0.2s;
      display: grid;
    }

    @keyframes fadeIn {
      from { opacity: 0; }
      to { opacity: 1; }
    }

    h1 {
      text-align: center;
      margin-bottom: 1.5rem;
      color: var(--primary);
    }

    .status-row {
      display: flex;
      justify-content: space-between;
      margin-bottom: 0.5rem;
    }

    .status-label {
      color: var(--text-dim);
    }

    .status-value {
      font-weight: bold;
    }

    .power-btn {
      display: block;
      width: 100%;
      padding: 1rem;
      margin: 1rem 0;
      border: none;
    }

    .power-btn:hover {
      transform: translateY(-2px);
      border-color: var(--primary);
    }

    .power-btn:active {
      transform: translateY(0);
    }

    .power-btn.off {
      background-color: var(--danger);
    }
    .power-btn.on {
      background-color: var(--success);
    }

    .mode-selector {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
      gap: 1rem;
      margin: 1.5rem 0;
    }

    .speed-selector{
      display: flex;
      justify-content: space-around;
      margin: 1.5rem 0;
    }

    .btn{
      border-radius: 8px;
      padding: 1rem;
      text-align: center;
      cursor: pointer;
      transition: all 0.2s;
      color: var(--text);
      font-size: large;
    }

    .btn-primary{
      background-color: var(--primary-dark);
      border: 2px solid var(--primary-dark);
    }

    .btn-primary:hover{
      background-color: var(--primary);
      border-color: var(--primary);
    }

    .mode-btn {
      background-color: var(--bg-light);
      border: 2px solid var(--bg-light);
    }

    .mode-btn:hover {
      border-color: var(--primary);
    }

    .mode-btn.active {
      border-color: var(--primary);
      background-color: rgba(108, 92, 231, 0.2);
    }

    .speed-btn {
      min-width: 3rem;
      background-color: var(--bg-light);
      border: 2px solid var(--bg-light);
    }

    .speed-btn:hover {
      border-color: var(--primary);
    }

    .speed-btn.active {
      border-color: var(--primary);
      background-color: rgba(108, 92, 231, 0.2);
    }

    .color-picker {
      width: 100%;
      height: 50px;
      margin: 1.5rem 0;
      cursor: pointer;
      border: 2px solid var(--primary);
      border-radius: 8px;
      background-color: var(--bg);
    }

    .slider-container {
      margin: 1.5rem 0;
    }

    .slider-label {
      display: flex;
      justify-content: space-between;
      margin-bottom: 0.5rem;
    }

    .slider {
      width: 100%;
      height: 8px;
      -webkit-appearance: none;
      appearance: none;
      background: var(--bg-light);
      border-radius: 4px;
      outline: none;
    }

    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 20px;
      height: 20px;
      background: var(--primary);
      border-radius: 50%;
      cursor: pointer;
    }

    .checkbox-container {
      display: flex;
      align-items: center;
      margin: 0.5rem 0 0 5px;
    }

    .checkbox {
      width: 18px;
      height: 18px;
      margin-right: 0.5rem;
      cursor: pointer;
      background-color: #1c2b52;
      accent-color: var(--primary);
    }

    .textfield{
      margin-top: 0.5rem;
      transition: 0.2s;
      background-color: #1c2b52;
      color: white;
      border-radius: 5px;
      border-color: var(--primary-dark);
      height: 30px;
    }

    .textfield:focus{
        box-shadow: 0 0 0 0.2rem var(--bg-light), 0 0 0 0.35rem var(--primary);
        transition: 0.2s;
      }

    .textfield-container{
      display: flex;
      flex-direction: column;
      align-items: start;
      margin: 0.5rem 0 0 5px;
      justify-self: start;
    }

    @media (max-width: 600px) {
      .mode-selector {
        grid-template-columns: 1fr 1fr;
      }
    }

    @media (min-width: 600px) {
      .speed-btn{
        min-width: 5rem;
      }
    }

    .card {
      display: flex;
      gap: 5px;
      flex-direction: column;
      background-color: var(--bg-light);
      border-radius: 8px;
      padding: 1rem;
      margin-bottom: 1.5rem;
      box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);
    }

    .secondary-title {
      margin-bottom: 0.5rem;
      color: var(--primary);
    }

    .gradient-preview {
      width: 100%;
      height: 50px;
      margin-bottom: 10px;
      border-radius: 8px;
      border: 2px solid var(--bg-light);
      background: linear-gradient(90deg, #0000ff, #00ffff);
      box-shadow: 0 2px 5px rgba(0, 0, 0, 0.2);
      transition: all 0.3s ease;
    }
    .btn-danger {
      background-color: var(--danger);
      border: 2px solid var(--danger);
    }
    .btn-danger:hover {
      background-color: #b71c1c;
      border-color: #b71c1c;
    }
    .btn-danger:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
  </style>
</head>
<body>
  <header>
    <nav>
      <a class="nav-btn" data-page="home" href="/">Главная</a>
      <a class="nav-btn" data-page="settings" href="/settings">Настройки</a>
    </nav>
  </header>
  <main>
)rawliteral";

const char html_template_p2[] PROGMEM = R"rawliteral(
  </main>
</body>
</html>
)rawliteral";

const char html_index_p1[] PROGMEM = R"rawliteral(
    <style>

      .d {
        display: grid;
        grid-template-rows: 1fr;
        opacity: 1;
        transition: grid-template-rows 0.15s ease-out, opacity 0.15s ease-out;
      }

      .d.hidden {
        grid-template-rows: 0fr;
        opacity: 0;
      }

      .di {
        overflow: hidden;
      }

      .gradient-editor-container {
          margin: 1.5rem 0;
          user-select: none;
      }

      .gradient-bar-wrap {
          position: relative;
          touch-action: none;
          height: 40px;
          margin-bottom: 10px;
          cursor: crosshair;
      }

      .gradient-preview-bg {
          width: 100%;
          height: 100%;
          border-radius: 4px;
          border: 2px solid var(--bg-light);
          background: linear-gradient(90deg, #fff, #000);
      }

      .gradient-stop-marker {
          position: absolute;
          top: -5px;
          width: 14px;
          height: 50px;
          background-color: #fff;
          border: 2px solid #333;
          border-radius: 4px;
          transform: translateX(-50%);
          cursor: grab;
          z-index: 10;
          box-shadow: 0 2px 4px rgba(0,0,0,0.5);
          transition: transform 0.1s, border-color 0.1s;
      }

      @media (hover: hover) {
        .gradient-stop-marker:hover {
            z-index: 11;
            transform: translateX(-50%) scale(1.1);
        }
      }

      .gradient-stop-marker::before {
          content: '';
          position: absolute;
          inset: -4px -12px;
      }

      .gradient-stop-marker.active {
          border-color: #fff;
          background-color: var(--text);
          z-index: 12;
          box-shadow: 0 0 0 2px var(--primary);
      }

      .gradient-stop-marker::after {

          content: '';
          display: block;
          width: 100%;
          height: 20px;
          background-color: inherit;
          margin-top: 26px;
          border-top: 1px solid #ccc;
      }

      .gradient-controls-row .btn {
          padding: 0.5rem 0.75rem;
          font-size: 1rem;
      }

      .gradient-controls-row {
          display: flex;
          align-items: center;
          gap: 15px;
          background: var(--bg-light);
          padding: 10px;
          border-radius: 8px;
      }
    </style>

    <div class="page" id="home">
      <h1>Управление светильником</h1>

      <div class="card">
        <div class="status-row">
          <span class="status-label">Состояние:</span>
          <span class="status-value" id="status-value">Выключен</span>
        </div>
        <div class="status-row">
          <span class="status-label">Последний режим:</span>
          <span class="status-value" id="mode-value">Выключен</span>
        </div>
          <div class="status-row">
          <span class="status-label">Скорость:</span>
          <span class="status-value" id="speed-value">3</span>
        </div>
        <div class="status-row">
          <span class="status-label">Яркость:</span>
          <span class="status-value" id="brightness-value">0%</span>
        </div>
      </div>

      <button class="power-btn btn off" id="power-btn">Включить</button>

      <h2>Режимы</h2>
      <div class="mode-selector">
        <div class="btn mode-btn" data-mode="3">Статичный</div>
        <div class="btn mode-btn" data-mode="2">Сплошной</div>
        <div class="btn mode-btn" data-mode="1">Радуга</div>
        <div class="btn mode-btn" data-mode="4">Огонь</div>
        <div class="btn mode-btn" data-mode="5">Градиент</div>
      </div>

      <h2>Скорость</h2>
      <div class="speed-selector">
        <div class="btn speed-btn" data-speed="1">1</div>
        <div class="btn speed-btn" data-speed="2">2</div>
        <div class="btn speed-btn" data-speed="3">3</div>
        <div class="btn speed-btn" data-speed="4">4</div>
        <div class="btn speed-btn" data-speed="5">5</div>
      </div>

      <div id="col-div" class="d">
        <div class="di">
          <h2>Цвет</h2>
          <input type="color" class="color-picker" id="color-picker" value="#6c5ce7">
        </div>
      </div>

      <div id="grad-div" class="d">
        <div class="di">
          <h2>Градиент</h2>
          <div class="gradient-editor-container">
              <div class="gradient-bar-wrap" id="grad-box">
                  <div class="gradient-preview-bg" id="grad-preview"></div>
                  </div>

              <div class="gradient-controls-row">
                  <div class="textfield-container" style="margin: 0;">
                      <label style="font-size: 0.8rem; color: var(--text-dim);">Цвет точки</label>
                      <input type="color" class="color-picker" id="stop-color-picker" style="width: 60px; height: 35px; margin: 0;">
                  </div>

                  <div class="textfield-container" style="flex: 1; margin: 0;">
                      <label style="font-size: 0.8rem; color: var(--text-dim);">Позиция (%)</label>
                      <span id="stop-position-val" style="font-weight: bold;">0%</span>
                  </div>

                  <button class="btn btn-danger" id="btn-delete-stop">Удалить точку</button>
              </div>

              <p style="margin-top: 5px; font-size: 0.8rem; color: var(--text-dim);">
                  Кликните по полосе, чтобы добавить точку (макс 10). Перетаскивайте для смены позиции.
              </p>
          </div>
        </div>
      </div>

      <h2>Яркость</h2>
      <div class="slider-container">
      <div class="slider-label">
        <span>Уровень яркости:</span>
        <span id="brightness-slider-value">50%</span>
      </div>
        <input type="range" min="0" max="255" value="50" class="slider" id="brightness-slider">
        <span id="brightness-warn-span" style="color: var(--warning); transition: 0.5s; opacity: 0;">При понижении яркости в Статичном режиме меняеться цветопередача</span>
      </div>
    </div>

<script>
  const state = )rawliteral";

const char html_index_p2[] PROGMEM = R"rawliteral(;
  let gradientStops = [];

  function init() {
    changeCards();
    bprecent = Math.round((state.brightness * 100) / 255);
    brightnessValue.textContent = `${bprecent}%`;
    brightnessSliderValue.textContent = `${bprecent}%`;
    brightnessSlider.value = state.brightness;
    colorPicker.value = state.color;
    colorPicker.style.backgroundColor = state.color;
    modeValue.textContent = getModeName(state.mode);
    speedValue.textContent = state.speed;
    updatePowerState();
    initGradient();
  }

  let selectedStopId = 1;
  let dragStopId = null;

  const powerBtn = document.getElementById('power-btn');
  const statusValue = document.getElementById('status-value');
  const modeValue = document.getElementById('mode-value');
  const brightnessValue = document.getElementById('brightness-value');
  const speedValue = document.getElementById('speed-value');
  const modeBtns = document.querySelectorAll('.mode-btn');
  const speedBtns = document.querySelectorAll('.speed-btn');
  const colorPicker = document.getElementById('color-picker');
  const brightnessSlider = document.getElementById('brightness-slider');
  const brightnessSliderValue = document.getElementById('brightness-slider-value');
  const brightnessWarnSpan = document.getElementById('brightness-warn-span');

  const gradBox = document.getElementById('grad-box');
  const gradPreview = document.getElementById('grad-preview');
  const stopColorPicker = document.getElementById('stop-color-picker');
  const btnDeleteStop = document.getElementById('btn-delete-stop');
  const stopPosLabel = document.getElementById('stop-position-val');

  const gradDiv = document.getElementById('grad-div');
  const colDiv = document.getElementById('col-div');
  powerBtn.addEventListener('click', () => {
    const nextPower = !state.power;
    fetch('/api/setPower', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ power: nextPower })
    }).then(response => {
      if (!response.ok) throw new Error('Ошибка запроса');
      state.power = nextPower;
      updatePowerState();
    })
    .catch(err => console.error('Ошибка:', err));
  });

  function updatePowerState() {
    if (state.power) {
      powerBtn.textContent = 'Выключить';
      powerBtn.classList.remove('off');
      powerBtn.classList.add('on');
      statusValue.textContent = 'Включен';
      modeBtns.forEach(btn => {
        if(btn.dataset.mode == state.mode){
          btn.classList.add('active');
          return;
        }
      });
      speedBtns.forEach(btn => {
        if(btn.dataset.speed == state.speed){
          btn.classList.add('active');
          return;
        }
      });
    } else {
      powerBtn.textContent = 'Включить';
      powerBtn.classList.add('off');
      powerBtn.classList.remove('on');
      statusValue.textContent = 'Выключен';
      modeBtns.forEach(btn => {
        if(btn.dataset.mode == state.mode) {
          btn.classList.remove('active');
          return;
        }
      });
      speedBtns.forEach(btn => {
        if(btn.dataset.speed == state.speed) {
          btn.classList.remove('active');
          return;
        }
      });
    }
  }
  modeBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      if (!state.power) return;
      modeBtns.forEach(b => b.classList.remove('active'));

      state.mode = +btn.dataset.mode;
      changeCards();

      fetch('/api/setMode', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode: +btn.dataset.mode })
      }).then(response => {
        if (!response.ok) throw new Error('Ошибка запроса');
        return response.json();
      })
      .then(data => {
        state.mode = +btn.dataset.mode;
        changeCards();
        modeValue.textContent = getModeName(state.mode);
        btn.classList.add('active');
      })
      .catch(err => {
        console.error('Ошибка:', err);
        modeBtns.forEach(btn => {
          if(btn.dataset.mode == state.mode){
            btn.classList.add('active');
            return;
          }
        });
      });

    });
  });

  speedBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      if (!state.power) return;

      speedBtns.forEach(b => b.classList.remove('active'));

      fetch('/api/setSpeed', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ speed: +btn.dataset.speed })
      }).then(response => {
        if (!response.ok) throw new Error('Ошибка запроса');
        return response.json();
      })
      .then(data => {
        state.speed = +btn.dataset.speed;
        speedValue.textContent = state.speed;
        btn.classList.add('active');
      })
      .catch(err => {
        console.error('Ошибка:', err);
        speedBtns.forEach(btn => {
          if(btn.dataset.speed == state.speed){
            btn.classList.add('active');
            return;
          }
        });
      });

    });
  });

  function changeCards(){
    switch(state.mode){
      case 5:
          colDiv.classList.add('hidden');
          gradDiv.classList.remove('hidden');
        break;
      case 3:
          gradDiv.classList.add('hidden');
          colDiv.classList.remove('hidden');
        break;
      default:
          colDiv.classList.add('hidden');
          gradDiv.classList.add('hidden');
        break;
    }
  }

  function getModeName(mode) {
    const modes = {
      1 : 'Радуга',
      2 : 'Сплошной (RGB)',
      3 : 'Статичный',
      4 : 'Огонь',
      5 : 'Градиент'
    };
    return modes[mode] || mode;
  }

const generateId = () => Date.now() + Math.random();

function initGradient() {
  state.gSs.forEach((s, index) => {
    gradientStops.push({
      id: generateId(),
      pos: s.pos * 100,
      color: s.color,
      outline: s.color
    });
  });
  selectedStopId = gradientStops[0].id;
  renderStops();
  updatePreview();
  updateControls();
}

const markers = new Map();

function renderStops() {
  markers.forEach((el, id) => {
    if (!gradientStops.some(s => s.id === id)) {
      el.remove();
      markers.delete(id);
    }
  });

  gradientStops.forEach(stop => {
    let el = markers.get(stop.id);
    if (!el) {
      el = document.createElement('div');
      el.className = 'gradient-stop-marker';
      el.addEventListener('pointerdown', (e) => {
        e.stopPropagation();
        startDrag(stop.id, e);
      });
      el.addEventListener('pointermove', (e) => {
        if (dragStopId === stop.id) moveStop(e.clientX);
      });
      el.addEventListener('pointerup', endDrag);
      el.addEventListener('pointercancel', endDrag);
      gradBox.appendChild(el);
      markers.set(stop.id, el);
    }
    el.style.left = `${stop.pos}%`;
    el.style.backgroundColor = stop.color;
    el.classList.toggle('active', stop.id === selectedStopId);
  });
}

function updatePreview() {
  const sorted = [...gradientStops].sort((a, b) => a.pos - b.pos);
  const cssGradient = sorted.map(s => `${s.color} ${s.pos}%`).join(', ');
  gradPreview.style.background = `linear-gradient(90deg, ${cssGradient})`;
}

function updateControls(changePicker = true) {
  const stop = gradientStops.find(s => s.id === selectedStopId);
  if (stop) {
    if (changePicker) stopColorPicker.value = stop.color;
    stopColorPicker.style.backgroundColor = stop.outline;
    stopPosLabel.textContent = Math.round(stop.pos) + '%';
    btnDeleteStop.disabled = gradientStops.length <= 2;
  }
}

stopColorPicker.addEventListener('input', (e) => {
  const stop = gradientStops.find(s => s.id === selectedStopId);
  if (stop) {
    stop.color = e.target.value;
    renderStops();
    updatePreview();
    sendGradientDebounced();
  }
});

btnDeleteStop.addEventListener('click', () => {
  if (gradientStops.length <= 2) return;
  gradientStops = gradientStops.filter(s => s.id !== selectedStopId);
  selectedStopId = gradientStops[0].id;
  renderStops();
  updatePreview();
  updateControls();
  sendGradientDebounced();
});

function percentFromClientX(clientX) {
  const rect = gradBox.getBoundingClientRect();
  const percent = ((clientX - rect.left) / rect.width) * 100;
  return Math.max(0, Math.min(100, percent));
}

gradBox.addEventListener('pointerdown', (e) => {
  if (e.target !== gradBox && e.target !== gradPreview) return;
  if (gradientStops.length >= 10) {
    alert('Максимум 10 точек');
    return;
  }

  const color = stopColorPicker.value;
  const id = generateId();
  gradientStops.push({ id, pos: percentFromClientX(e.clientX), color, outline: color });
  startDrag(id, e);
  updatePreview();
});

function startDrag(id, e) {
  e.preventDefault();
  dragStopId = id;
  selectedStopId = id;
  renderStops();
  markers.get(id).setPointerCapture(e.pointerId);
  updateControls();
}

function endDrag() {
  if (dragStopId === null) return;
  dragStopId = null;
  sendGradientDebounced();
}

function moveStop(clientX) {
  const stop = gradientStops.find(s => s.id === dragStopId);
  if (stop) {
    stop.pos = percentFromClientX(clientX);
    renderStops();
    updatePreview();
    updateControls();
  }
}

let gradDebounceTimer = null;
function sendGradientDebounced() {
  if (!state.power) return;
  if (state.mode != 5) return;
  clearTimeout(gradDebounceTimer);
  gradDebounceTimer = setTimeout(() => {
    const sortedStops = [...gradientStops].sort((a, b) => a.pos - b.pos);

  const payload = {
      colors: sortedStops.map(s => s.color),
      stops: sortedStops.map(s => parseFloat((s.pos / 100).toFixed(4)))
    };

    fetch('/api/setGradient', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
    }).then(response => {
      if (!response.ok) throw new Error('Ошибка запроса');
      return response.json();
    })
    .then(data => {
      gradientStops.forEach(stop => {
        stop.outline = stop.color;
      });
      updateControls(false);
    }).catch(err => console.error(err));

  }, 300);
}

  let colorDebounceTimer = null;

  colorPicker.addEventListener('input', (e) => {
    if (!state.power) return;
    if (state.mode != 3) return;
    state.color = e.target.value;

    clearTimeout(colorDebounceTimer);

    colorDebounceTimer = setTimeout(() => {
      fetch('/api/setColor', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ color: state.color })
      }).then(response => {
        if (!response.ok) throw new Error('Ошибка запроса');
        return response.json();
      })
      .then(data => {
        colorPicker.style.backgroundColor = state.color;
      })
      .catch(err => {
        console.error('', err);
      });
    }, 300);
  });

  let brightnessDebounceTimer = null;

  brightnessSlider.addEventListener('input', (e) => {
    if (!state.power) return;

    state.brightness = +e.target.value;
    brightnessPercent = Math.round((state.brightness * 100) / 255);

    brightnessSliderValue.textContent = `${brightnessPercent}%`;
    if(e.target.value < 128){
      brightnessWarnSpan.style.opacity = 1;
    } else {
      brightnessWarnSpan.style.opacity = 0;
    }

    clearTimeout(brightnessDebounceTimer);

    brightnessDebounceTimer = setTimeout(() => {
      fetch('/api/setBrightness', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ brightness: state.brightness })
      })
      .then(response => {
        if (!response.ok) throw new Error('Ошибка запроса');
        return response.json();
      })
      .then(data => {
        brightnessValue.textContent = `${brightnessPercent}%`;
      })
      .catch(err => {
        console.error('Ошибка:', err);
      });

    }, 300);
  });

  document.addEventListener('DOMContentLoaded', (event) => {
    init();
  });
</script>
)rawliteral";

const char html_settings_p1[] PROGMEM = R"rawliteral(
<div class="page active" id="settings">
  <h1>Настройки</h1>

  <style>
    .network-header-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 0.5rem;
      margin: 0.5rem 0 0 5px;
    }

    .network-header-row label {
      font-size: 1rem;
      color: var(--text);
    }

    #sta-network-list {
      width: 100%;
      min-height: 120px;
      max-height: 320px;
      overflow-y: auto;
      margin-top: 0.6rem;
      border: 1px solid #2a365b;
      border-radius: 8px;
      background: #121b35;
      padding: 0.35rem;
    }

    .network-item {
      width: 100%;
      text-align: left;
      border: 1px solid #2f4576;
      background: #1a2748;
      color: var(--text);
      border-radius: 8px;
      padding: 0.7rem;
      margin-bottom: 0.4rem;
      cursor: pointer;
      transition: background 0.15s, border-color 0.15s;
    }

    .network-item:last-child {
      margin-bottom: 0;
    }

    .network-item:hover {
      border-color: var(--primary);
      background: #243766;
    }

    .network-item.selected {
      border-color: var(--primary);
      box-shadow: 0 0 0 1px var(--primary);
    }

    .network-item-title {
      font-weight: 600;
      display: flex;
      justify-content: space-between;
      gap: 0.5rem;
      align-items: baseline;
      margin-bottom: 0.3rem;
    }

    .network-item-meta {
      font-size: 0.88rem;
      color: var(--text-dim);
      display: flex;
      flex-wrap: wrap;
      gap: 0.5rem;
    }

    .network-badge {
      border-radius: 6px;
      padding: 0.1rem 0.4rem;
      background: #2a3b67;
      color: #d7e5ff;
      font-size: 0.78rem;
    }

    .network-badge.connected {
      background: #1e6b4d;
      color: #d9ffef;
    }

    .network-badge.saved {
      background: #4b5a1c;
      color: #f2ffd4;
    }

    .network-status {
      color: var(--text-dim);
      min-height: 1.2rem;
      margin: 0.45rem 0 0 5px;
    }

    .network-hint {
      color: var(--text-dim);
      margin: 0.4rem 0 0 5px;
      font-size: 0.9rem;
    }

    .manual-form {
      margin-top: 0.7rem;
      border-top: 1px dashed #3b4a72;
      padding-top: 0.7rem;
      display: none;
    }

    .manual-form.open {
      display: block;
    }

    .toggle-manual {
      margin-top: 0.7rem;
    }
  </style>

  <div class="card">
    <h3 class="secondary-title">Настройка сети (STA mode)</h3>
    <div class="network-header-row">
      <label for="sta-network-list">Доступные сети в радиусе</label>
      <button class="btn btn-primary" style="padding: 0.25rem;" id="refresh-networks" type="button">Обновить</button>
    </div>
    <div id="sta-network-list"></div>
    <span class="network-status" id="scan-status">Сети еще не загружены</span>
    <span class="network-hint">Нажмите на сеть для подключения. Если сеть новая, будет запрошен пароль.</span>

    <div class="checkbox-container">
      <input type="checkbox" class="checkbox" id="sta-mode">
      <label for="sta-mode">Подключаться к существующей сети (режим STA)</label>
    </div>

    <button class="btn btn-primary toggle-manual" id="toggle-manual-sta" type="button">Показать ручной ввод</button>
    <div class="manual-form" id="manual-sta-form">
      <div class="textfield-container">
        <label for="ssid-sta">Название сети</label>
        <input type="text" class="textfield" id="ssid-sta">
      </div>
      <div class="textfield-container">
        <label for="pass-sta">Пароль сети (пусто для открытой сети)</label>
        <input type="text" class="textfield" id="pass-sta">
      </div>
      <button class="btn btn-primary" style="margin-top: 0.7rem; width: 100%;" id="send-sta">Подключиться</button>
    </div>
  </div>

  <div class="card">
    <h3 class="secondary-title">Настройка собственной сети (AP mode)</h3>
    <div class="textfield-container">
      <label for="ssid-sta">Название сети</label>
      <input type="text" class="textfield" id="ssid-ap">
    </div>
    <div class="textfield-container">
      <label for="pass-sta">Пароль сети</label>
      <input type="text" class="textfield" id="pass-ap">
    </div>
    <div class="checkbox-container">
      <input type="checkbox" class="checkbox" id="pass-req-ap">
      <label for="pass-req-ap">Требовать пароль</label>
    </div>
    <button class="btn btn-primary" id="send-ap">Сохранить</button>
    <span style="color: var(--warning);">Если не удасться подключиться к вашей сети то поумолчанию светильник раздаст свою сеть</span>
  </div>

  <div class="card">
    <h3 class="secondary-title">Дополнительно</h3>
    <a class="btn btn-primary" href="/saved_networks" style="text-decoration:none;">Сохраненные сети</a>
    <button class="btn btn-primary" onclick="reboot()">Перезапустить</button>
    <button class="btn btn-primary" onclick="update()">Обновить прошивку с GitHub</button>
    <label class="btn btn-primary" for="fw-file">Загрузить прошивку из файла (.bin)</label>
    <input type="file" id="fw-file" accept=".bin" style="display: none;">
    <span class="network-status" id="fw-status"></span>
  </div>
</div>

<script>
  const settings = )rawliteral";

const char html_settings_p2[] PROGMEM = R"rawliteral(;
  const ssidStaInput = document.getElementById('ssid-sta');
  const passStaInput = document.getElementById('pass-sta');
  const staModeCheckbox = document.getElementById('sta-mode');
  const ssidApInput = document.getElementById('ssid-ap');
  const passApInput = document.getElementById('pass-ap');
  const passReqApCheckbox = document.getElementById('pass-req-ap');
  const staNetworksList = document.getElementById('sta-network-list');
  const refreshNetworksButton = document.getElementById('refresh-networks');
  const scanStatus = document.getElementById('scan-status');
  const toggleManualStaButton = document.getElementById('toggle-manual-sta');
  const manualStaForm = document.getElementById('manual-sta-form');

  let networksTimerId = null;
  let connectPollTimerId = null;
  let selectedNetworkSsid = '';

  function escapeHtml(value) {
    return String(value)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }

  function signalLabelFromRssi(rssi) {
    if (rssi >= -60) return 'сильный';
    if (rssi >= -75) return 'средний';
    return 'слабый';
  }

  function renderNetworks(networks) {
    staNetworksList.innerHTML = '';

    if (!Array.isArray(networks) || networks.length === 0) {
      const empty = document.createElement('div');
      empty.className = 'network-item';
      empty.textContent = 'Сети не найдены';
      staNetworksList.appendChild(empty);
      return;
    }

    networks.sort((a, b) => b.rssi - a.rssi);

    networks.forEach((network) => {
      const ssid = network.ssid || '(скрытая сеть)';
      const signalLabel = signalLabelFromRssi(network.rssi || -100);
      const securityLabel = network.secure ? 'защищена' : 'открытая';

      const item = document.createElement('button');
      item.type = 'button';
      item.className = 'network-item';
      if (network.ssid && network.ssid === selectedNetworkSsid) {
        item.classList.add('selected');
      }
      item.dataset.ssid = network.ssid || '';

      const badges = [];
      if (network.connected) {
        badges.push('<span class="network-badge connected">подключено</span>');
      }
      if (network.saved) {
        badges.push('<span class="network-badge saved">сохранена</span>');
      }
      badges.push(`<span class="network-badge">${securityLabel}</span>`);

      item.innerHTML = `
        <div class="network-item-title">
          <span>${escapeHtml(ssid)}</span>
          <span>${network.rssi} dBm</span>
        </div>
        <div class="network-item-meta">
          <span>Сигнал: ${signalLabel}</span>
          ${badges.join('')}
        </div>
      `;

      item.addEventListener('click', () => {
        if (!network.ssid) {
          return;
        }
        selectedNetworkSsid = network.ssid;
        ssidStaInput.value = network.ssid;
        renderNetworks(networks);

        connectToNetwork(network.ssid).catch((error) => {
          scanStatus.textContent = 'Ошибка запуска подключения';
          console.error('Ошибка подключения к выбранной сети:', error);
        });
      });

      staNetworksList.appendChild(item);
    });
  }

  async function scanNetworks(force = false) {
    if (!staModeCheckbox.checked) {
      scanStatus.textContent = 'STA режим отключен, сканирование остановлено';
      staNetworksList.innerHTML = '';
      return;
    }

    scanStatus.textContent = 'Сканирование...';
    refreshNetworksButton.disabled = true;

    try {
      const response = await fetch(force ? '/api/networks?force=1' : '/api/networks');
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }

      const data = await response.json();
      const networks = Array.isArray(data.networks) ? data.networks : [];

      if (data.disabled) {
        staModeCheckbox.checked = false;
        if (networksTimerId) {
          clearInterval(networksTimerId);
          networksTimerId = null;
        }
        staNetworksList.innerHTML = '';
        scanStatus.textContent = 'STA режим отключен на контроллере';
        return;
      }

      renderNetworks(networks);
      if (data.busy) {
        scanStatus.textContent = 'Сканирование приостановлено на время подключения';
      } else if (data.scanning) {
        scanStatus.textContent = 'Сканирование...';
        setTimeout(() => scanNetworks(false), 1500);
      } else {
        scanStatus.textContent = `Найдено сетей: ${networks.length}`;
      }
    } catch (error) {
      scanStatus.textContent = 'Ошибка сканирования сетей';
      console.error('Ошибка сканирования Wi-Fi:', error);
    } finally {
      refreshNetworksButton.disabled = false;
    }
  }

  async function connectToNetwork(ssid, password = null) {
    const payload = { ssid };
    if (password !== null) {
      payload.password = password;
      payload.passwordProvided = true;
    }

    const response = await fetch('/api/connect_network', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    let data = {};
    try {
      data = await response.json();
    } catch (e) {
      data = {};
    }

    if (!response.ok) {
      if (data.requiresPassword) {
        const entered = prompt(`Введите пароль для сети ${ssid}`);
        if (entered === null) {
          return;
        }

        await connectToNetwork(ssid, entered);
        return;
      }

      throw new Error(data.error || `HTTP ${response.status}`);
    }

    selectedNetworkSsid = ssid;
    ssidStaInput.value = ssid;
    scanStatus.textContent = `Подключение к ${ssid}...`;
    pollConnectionStatus(ssid);
  }

  async function pollConnectionStatus(ssidForRetry) {
    if (connectPollTimerId) {
      clearTimeout(connectPollTimerId);
      connectPollTimerId = null;
    }

    try {
      const response = await fetch('/api/connection_status');
      const data = await response.json();

      if (data.connected === 0) {
        connectPollTimerId = setTimeout(() => pollConnectionStatus(ssidForRetry), 1000);
        return;
      }

      if (data.connected === 1) {
        scanStatus.textContent = `Подключено, IP: ${data.ip}`;
        alert(`Светильник подключен. Новый адрес: http://${data.ip}`);
        return;
      }

      if (data.connected === 2 && data.needsPassword && ssidForRetry) {
        const entered = prompt(`Пароль для ${ssidForRetry} не подошел. Введите новый пароль`);
        if (entered !== null && entered.length > 0) {
          await connectToNetwork(ssidForRetry, entered);
          return;
        }
      }

      scanStatus.textContent = 'Не удалось подключиться. Контроллер пытается вернуться к предыдущей сети.';
    } catch (error) {
      scanStatus.textContent = 'Устройство могло уже переключиться на новую сеть. Подключитесь к ней и откройте lamp.local или новый IP.';
    }
  }

  document.addEventListener('DOMContentLoaded', (event) => {
    ssidStaInput.value = settings.ssidSta;
    staModeCheckbox.checked = settings.useStaMode;
    ssidApInput.value = settings.ssidAp;
    passApInput.value = settings.passwordAp;
    passReqApCheckbox.checked = settings.requiresPassAp;

    if (settings.ssidSta) {
      selectedNetworkSsid = settings.ssidSta;
    }

    if (networksTimerId) {
      clearInterval(networksTimerId);
    }

    if (staModeCheckbox.checked) {
      scanNetworks(true);
      networksTimerId = setInterval(() => scanNetworks(false), 30000);
    } else {
      scanStatus.textContent = 'STA режим отключен, сканирование остановлено';
      staNetworksList.innerHTML = '';
    }
  });

  window.addEventListener('beforeunload', () => {
    if (networksTimerId) {
      clearInterval(networksTimerId);
    }
  });

  refreshNetworksButton.addEventListener('click', () => {
    if (!staModeCheckbox.checked) {
      scanStatus.textContent = 'Включите STA режим, чтобы сканировать сети';
      return;
    }

    scanNetworks(true);
  });

  staModeCheckbox.addEventListener('change', async () => {
    const enabled = staModeCheckbox.checked;

    try {
      await saveStaModeSetting(enabled);
    } catch (error) {
      staModeCheckbox.checked = !enabled;
      scanStatus.textContent = 'Не удалось сохранить STA режим';
      console.error('Ошибка сохранения STA режима:', error);
      return;
    }

    if (networksTimerId) {
      clearInterval(networksTimerId);
      networksTimerId = null;
    }

    if (enabled) {
      scanNetworks(true);
      networksTimerId = setInterval(() => scanNetworks(false), 30000);
    } else {
      scanStatus.textContent = 'STA режим отключен, сканирование остановлено';
      staNetworksList.innerHTML = '';
    }
  });

  toggleManualStaButton.addEventListener('click', () => {
    const opened = manualStaForm.classList.toggle('open');
    toggleManualStaButton.textContent = opened ? 'Скрыть ручной ввод' : 'Показать ручной ввод';
  });

  function reboot() {
    fetch("/api/reboot", {
      method: "POST"
    })
    .then(response => response.json())
    .then(data => {
      alert("Контроллер сейчас перезагрузиться")
    })
    .catch(err => {
      alert("Ошибка перезапуска");
    });
  }

  function update() {
    fetch("/api/update", {
      method: "POST"
    })
    .then(response => {
      alert("Контроллер начал процесс обновления пожалуйста не выключайте светильник")
    })
    .catch(err => {
      alert("Ошибка обновления");
    });
  }

  async function saveStaModeSetting(useStaModeValue) {
    const response = await fetch('/api/save_sta_mode', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        useStaMode: useStaModeValue
      })
    });

    if (!response.ok) {
      throw new Error('save_sta_mode_failed');
    }

    settings.useStaMode = useStaModeValue;
  }

  document.getElementById('send-sta').addEventListener('click', async () => {
    const ssid = ssidStaInput.value.trim();
    if (!ssid) {
      alert('Введите название сети');
      return;
    }

    try {
      await connectToNetwork(ssid, passStaInput.value);
    } catch (error) {
      scanStatus.textContent = 'Ошибка запуска подключения';
      console.error('Ошибка подключения:', error);
    }
  });

  const fwStatus = document.getElementById('fw-status');
  document.getElementById('fw-file').addEventListener('change', (event) => {
    const file = event.target.files[0];
    event.target.value = '';
    if (!file || !confirm(`Прошить светильник файлом ${file.name}?`)) {
      return;
    }

    const form = new FormData();
    form.append('firmware', file, file.name);

    const xhr = new XMLHttpRequest();
    xhr.upload.onprogress = (e) => {
      if (e.lengthComputable) {
        fwStatus.textContent = `Загрузка: ${Math.round(e.loaded * 100 / e.total)}%`;
      }
    };
    xhr.onload = () => {
      fwStatus.textContent = xhr.status === 200
        ? 'Прошивка загружена, светильник перезагружается...'
        : `Ошибка прошивки: ${xhr.responseText}`;
    };
    xhr.onerror = () => {
      fwStatus.textContent = 'Ошибка загрузки файла';
    };
    xhr.open('POST', '/api/firmware');
    xhr.send(form);
  });

  document.getElementById('send-ap').addEventListener('click', () => {
    const ssidAp = ssidApInput.value;
    const passAp = passApInput.value;
    const requirePass = passReqApCheckbox.checked;

    fetch('/api/save_ap', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        ssid: ssidAp,
        password: passAp,
        requirePass: requirePass
      })
    })
    .then(response => {
      if (response.ok) {
        alert("AP настройки сохранены!");
      } else {
        alert("Ошибка сохранения AP настроек.");
      }
    })
    .catch(error => {
      console.error("Ошибка сети:", error);
    });
  });
</script>
)rawliteral";

const char html_saved_networks_p1[] PROGMEM = R"rawliteral(
<script>
  window.initialSavedNetworks = )rawliteral";

const char html_saved_networks_p2[] PROGMEM = R"rawliteral(;
</script>
<div class="page active" id="saved-networks">
  <h1>Сохраненные сети</h1>

  <style>
    .saved-list {
      display: flex;
      flex-direction: column;
      gap: 0.5rem;
    }

    .saved-item {
      border: 1px solid #2f4576;
      background: #1a2748;
      border-radius: 8px;
      padding: 0.7rem;
    }

    .saved-item-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 0.5rem;
      margin-bottom: 0.5rem;
    }

    .saved-item-title {
      font-weight: 600;
      word-break: break-all;
    }

    .saved-badge {
      background: #2a3b67;
      color: #d7e5ff;
      border-radius: 6px;
      padding: 0.12rem 0.45rem;
      font-size: 0.78rem;
    }

    .saved-actions {
      display: flex;
      gap: 0.5rem;
      flex-wrap: wrap;
    }

    .saved-status {
      color: var(--text-dim);
      margin-top: 0.6rem;
      min-height: 1.2rem;
    }
  </style>

  <div class="card">
    <h3 class="secondary-title">Список сохраненных сетей</h3>
    <div id="saved-list" class="saved-list"></div>
    <div id="saved-status" class="saved-status">Загрузка...</div>
  </div>
</div>

<script>
  const savedList = document.getElementById('saved-list');
  const savedStatus = document.getElementById('saved-status');
  const savedNetworks = Array.isArray(window.initialSavedNetworks?.networks)
    ? window.initialSavedNetworks.networks.map((network) => ({
        ssid: network.ssid || '',
        hasPassword: !!network.hasPassword,
        isLast: !!network.isLast
      }))
    : [];

  function escapeHtml(value) {
    return String(value)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/\"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }

  function renderSavedNetworks(networks) {
    savedList.innerHTML = '';

    if (!Array.isArray(networks) || networks.length === 0) {
      const empty = document.createElement('div');
      empty.className = 'saved-item';
      empty.textContent = 'Нет сохраненных сетей';
      savedList.appendChild(empty);
      return;
    }

    networks.forEach((network) => {
      const item = document.createElement('div');
      item.className = 'saved-item';

      const ssid = network.ssid || '(пусто)';
      const hasPassword = !!network.hasPassword;
      const isLast = !!network.isLast;

      item.innerHTML = `
        <div class="saved-item-header">
          <div class="saved-item-title">${escapeHtml(ssid)}</div>
          <div>
            ${isLast ? '<span class="saved-badge">последняя</span>' : ''}
            <span class="saved-badge">${hasPassword ? 'пароль задан' : 'без пароля'}</span>
          </div>
        </div>
        <div class="saved-actions">
          <button class="btn btn-primary" data-action="edit" data-ssid="${escapeHtml(ssid)}" type="button">Изменить</button>
          <button class="btn btn-danger" data-action="delete" data-ssid="${escapeHtml(ssid)}" type="button">Удалить</button>
        </div>
      `;

      savedList.appendChild(item);
    });
  }

  function renderInitialSavedNetworks() {
    renderSavedNetworks(savedNetworks);
    savedStatus.textContent = `Сохранено сетей: ${savedNetworks.length}`;
  }

  function updateLocalStateAfterUpdate(oldSsid, newSsid, passwordProvided, newPassword) {
    const oldIndex = savedNetworks.findIndex((network) => network.ssid === oldSsid);
    const targetIndex = savedNetworks.findIndex((network) => network.ssid === newSsid);

    if (oldIndex < 0) {
      return;
    }

    const oldNetwork = savedNetworks[oldIndex];
    const nextHasPassword = passwordProvided ? newPassword.length > 0 : oldNetwork.hasPassword;

    if (targetIndex >= 0 && targetIndex != oldIndex) {
      const targetNetwork = savedNetworks[targetIndex];
      targetNetwork.hasPassword = passwordProvided ? nextHasPassword : (targetNetwork.hasPassword || oldNetwork.hasPassword);
      targetNetwork.isLast = targetNetwork.isLast || oldNetwork.isLast;
      savedNetworks.splice(oldIndex, 1);
    } else {
      oldNetwork.ssid = newSsid;
      oldNetwork.hasPassword = nextHasPassword;
    }
  }

  function removeLocalStateBySsid(ssid) {
    const index = savedNetworks.findIndex((network) => network.ssid === ssid);
    if (index < 0) {
      return;
    }

    savedNetworks.splice(index, 1);
  }

  async function updateSavedNetwork(oldSsid) {
    const newSsid = prompt(`Новое имя сети для ${oldSsid}`, oldSsid);
    if (newSsid === null) {
      return;
    }

    const trimmedSsid = newSsid.trim();
    if (!trimmedSsid) {
      savedStatus.textContent = 'SSID не может быть пустым';
      return;
    }

    const newPassword = prompt('Новый пароль (оставьте пустым, чтобы не менять)');
    if (newPassword === null) {
      return;
    }

    const passwordProvided = newPassword.length > 0;
    const response = await fetch('/api/saved_networks/update', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        oldSsid,
        newSsid: trimmedSsid,
        password: newPassword,
        passwordProvided
      })
    });

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    updateLocalStateAfterUpdate(oldSsid, trimmedSsid, passwordProvided, newPassword);
    renderSavedNetworks(savedNetworks);
    savedStatus.textContent = `Сеть обновлена. Сохранено сетей: ${savedNetworks.length}`;
  }

  async function deleteSavedNetwork(ssid) {
    const confirmDelete = confirm(`Удалить сеть ${ssid}?`);
    if (!confirmDelete) {
      return;
    }

    const response = await fetch('/api/saved_networks/delete', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ssid })
    });

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    removeLocalStateBySsid(ssid);
    renderSavedNetworks(savedNetworks);
    savedStatus.textContent = `Сеть удалена. Сохранено сетей: ${savedNetworks.length}`;
  }

  savedList.addEventListener('click', async (event) => {
    const target = event.target;
    if (!(target instanceof HTMLElement)) {
      return;
    }

    const action = target.getAttribute('data-action');
    const ssid = target.getAttribute('data-ssid');
    if (!action || !ssid) {
      return;
    }

    try {
      if (action === 'edit') {
        await updateSavedNetwork(ssid);
      } else if (action === 'delete') {
        await deleteSavedNetwork(ssid);
      }
    } catch (error) {
      savedStatus.textContent = 'Операция не выполнена';
      console.error('Ошибка управления сохраненной сетью:', error);
    }
  });

  document.addEventListener('DOMContentLoaded', () => {
    renderInitialSavedNetworks();
  });
</script>
)rawliteral";

namespace {
  struct PageContext {
    PGM_P parts[5];
    String data;
    uint8_t part = 0;
    size_t offset = 0;
  };

  void sendPage(AsyncWebServerRequest* request, PGM_P before, String data, PGM_P after) {
    data.replace("</", "<\\/");

    auto ctx = std::make_shared<PageContext>();
    ctx->parts[0] = html_template_p1;
    ctx->parts[1] = before;
    ctx->parts[2] = nullptr;
    ctx->parts[3] = after;
    ctx->parts[4] = html_template_p2;
    ctx->data = std::move(data);

    request->send(new AsyncChunkedResponse("text/html", [ctx](uint8_t* buffer, size_t maxLen, size_t) -> size_t {
      size_t written = 0;
      while (written < maxLen && ctx->part < 5) {
        PGM_P part = ctx->parts[ctx->part];
        size_t length = part ? strlen_P(part) : ctx->data.length();
        size_t count = std::min(length - ctx->offset, maxLen - written);

        if (part) {
          memcpy_P(buffer + written, part + ctx->offset, count);
        } else {
          memcpy(buffer + written, ctx->data.c_str() + ctx->offset, count);
        }

        written += count;
        ctx->offset += count;
        if (ctx->offset >= length) {
          ctx->part++;
          ctx->offset = 0;
        }
      }
      return written;
    }));
  }

  String toJson(const JsonDocument& doc) {
    String out;
    serializeJson(doc, out);
    return out;
  }
}

void sendIndex(AsyncWebServerRequest* request, const LampState& state) {
  JsonDocument doc;
  doc["brightness"] = state.brightness;
  doc["mode"] = state.mode;
  doc["speed"] = state.speed;
  doc["power"] = state.power;
  doc["color"] = state.colorHex;
  JsonArray stops = doc["gSs"].to<JsonArray>();
  for (uint8_t i = 0; i < state.stopsCount; i++) {
    JsonObject stop = stops.add<JsonObject>();
    stop["pos"] = state.stops[i].position;
    stop["color"] = state.stops[i].colorHex;
  }
  sendPage(request, html_index_p1, toJson(doc), html_index_p2);
}

void sendSettings(AsyncWebServerRequest* request, const SettingsAP& ap, bool useStaMode) {
  SavedWiFiNetwork last;
  JsonDocument doc;
  doc["ssidSta"] = getLastSavedWiFiNetwork(&last) ? last.ssid : "";
  doc["useStaMode"] = useStaMode;
  doc["ssidAp"] = ap.ssid;
  doc["passwordAp"] = ap.password;
  doc["requiresPassAp"] = ap.requirePass;
  sendPage(request, html_settings_p1, toJson(doc), html_settings_p2);
}

void sendSavedNetworks(AsyncWebServerRequest* request, const SavedWiFiStorage& storage) {
  JsonDocument doc;
  doc["lastIndex"] = storage.lastIndex;
  JsonArray networks = doc["networks"].to<JsonArray>();
  for (int i = 0; i < MAX_SAVED_WIFI_NETWORKS; i++) {
    const SavedWiFiNetwork& network = storage.networks[i];
    if (!network.used) continue;
    JsonObject item = networks.add<JsonObject>();
    item["ssid"] = network.ssid;
    item["hasPassword"] = network.password[0] != '\0';
    item["isLast"] = storage.lastIndex == i;
  }
  sendPage(request, html_saved_networks_p1, toJson(doc), html_saved_networks_p2);
}
