#include "HtmlResponses.h"

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

const char html_connecting[] PROGMEM = R"rawliteral(
<div class="page active">
  <h1 id="title">Подключение</h1>
  <div class="card">
    <h3 id="stitle" class="secondary-title">Идет подключение к вашей сети</h3>
    <p id="warn" style="color: var(--warning);">Не покидайте эту страницу</p>
    <p id="msg">После успешного подключения запомните новый адрес для управления</p>
  </div>
</div>
<script>
  function checkConnection() {
    fetch('/api/connection_status')
      .then(r => r.json())
      .then(data => {
        if (data.connected == 1) {
          document.getElementById('msg').innerHTML = 'Подключитесь к вашей сети и откройте <a style="font-weight: bold; color: var(--primary); font-size: large;" href="http://' + data.ip + '">http://' + data.ip + '</a> или <a style="font-weight: bold; color: var(--primary); font-size: large;" href="http://lamp.local">http://lamp.local</a>';
          document.getElementById('title').innerHTML = 'Подключено!';
          document.getElementById('stitle').innerHTML = 'Подключение к вашей сети завершено';
          document.getElementById('warn').innerHTML = 'Запомните этот адрес иначе вы не сможете подключиться к светильнику';
        } else if(data.connected == 2) {
          document.getElementById('msg').innerHTML = 'Ошибка подключения, поменяйте данные для подключения к сети и повторите попытку';
        } else {
          setTimeout(checkConnection, 1000);
        }
      });
  }
  window.onload = checkConnection;
</script>
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
      /* --- Стили для редактора градиента --- */
      .gradient-editor-container {
          margin: 1.5rem 0;
          user-select: none; /* Чтобы текст не выделялся при перетаскивании */
      }

      .gradient-bar-wrap {
          position: relative;
          height: 40px; /* Высота области с ползунками */
          margin-bottom: 10px;
          cursor: crosshair; /* Курсор-крестик, намекающий на добавление */
      }

      .gradient-preview-bg {
          width: 100%;
          height: 100%;
          border-radius: 4px;
          border: 2px solid var(--bg-light);
          background: linear-gradient(90deg, #fff, #000); /* Заглушка */
      }

      .gradient-stop-marker {
          position: absolute;
          top: -5px; /* Чуть выше полоски */
          width: 14px;
          height: 50px; /* Выходит за пределы полоски вниз */
          background-color: #fff;
          border: 2px solid #333;
          border-radius: 4px;
          transform: translateX(-50%); /* Центрируем относительно точки */
          cursor: grab;
          z-index: 10;
          box-shadow: 0 2px 4px rgba(0,0,0,0.5);
          transition: transform 0.1s, border-color 0.1s;
      }

      .gradient-stop-marker:hover {
          z-index: 11;
          transform: translateX(-50%) scale(1.1);
      }

      .gradient-stop-marker.active {
          border-color: #fff;
          background-color: var(--text);
          z-index: 12;
          box-shadow: 0 0 0 2px var(--primary);
      }

      .gradient-stop-marker::after {
          /* Цветовой квадратик внутри маркера */
          content: '';
          display: block;
          width: 100%;
          height: 20px;
          background-color: inherit; /* Будем менять через JS */
          margin-top: 26px;
          border-top: 1px solid #ccc;
      }

      .gradient-controls-row {
          display: flex;
          align-items: center;
          gap: 15px;
          background: var(--bg-light);
          padding: 10px;
          border-radius: 8px;
      }

      .btn-danger {
          background-color: var(--danger);
          color: white;
          border: none;
          padding: 0.5rem 1rem;
          border-radius: 4px;
          cursor: pointer;
      }
      .btn-danger:disabled {
          opacity: 0.5;
          cursor: not-allowed;
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

                  <button class="btn-danger" id="btn-delete-stop">Удалить точку</button>
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
  const state = {
)rawliteral";

const char html_index_p2[] PROGMEM = R"rawliteral(
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
  let isDragging = false;
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
    state.power = !state.power;
    fetch('/api/setPower', {
        method: 'POST',
      }).then(response => {
        if (!response.ok){
          state.power = !state.power;
          throw new Error('Ошибка запроса');
        }
        return response.json();
      })
      .then(data => {
        updatePowerState();
      })
      .catch(err => {
        console.error('Ошибка:', err);
        state.power = !state.power;
      });
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
        body: JSON.stringify({ mode: btn.dataset.mode })
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
        body: JSON.stringify({ speed: btn.dataset.speed })
      }).then(response => {
        if (!response.ok) throw new Error('Ошибка запроса');
        return response.json();
      })
      .then(data => {
        state.speed = btn.dataset.speed;
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

function renderStops() {
  const oldMarkers = gradBox.querySelectorAll('.gradient-stop-marker');
  oldMarkers.forEach(m => m.remove());

  gradientStops.forEach(stop => {
    const el = document.createElement('div');
    el.className = 'gradient-stop-marker';
    if (stop.id === selectedStopId) el.classList.add('active');

    el.style.left = `${stop.pos}%`;
    el.style.backgroundColor = stop.color;

    el.addEventListener('mousedown', (e) => {
      e.stopPropagation();
      startDrag(stop.id, e);
    });

    el.addEventListener('touchstart', (e) => {
      e.stopPropagation();
      startDrag(stop.id, e.touches[0]);
    }, {passive: false});

    gradBox.appendChild(el);
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
    if(changePicker) stopColorPicker.value = stop.color;
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

gradBox.addEventListener('mousedown', (e) => {
  if (e.target !== gradBox && e.target !== gradPreview) return;
  addStopAtEvent(e);
});

function addStopAtEvent(e) {
  if (gradientStops.length >= 10) {
    alert("Максимум 10 точек");
    return;
  }

  const rect = gradBox.getBoundingClientRect();
  let x = e.clientX - rect.left;
  let percent = (x / rect.width) * 100;
  percent = Math.max(0, Math.min(100, percent));

  const newColor = stopColorPicker.value;

  const newId = generateId();
  gradientStops.push({ id: newId, pos: percent, color: newColor, outline: newColor });

  selectedStopId = newId;

  renderStops();
  updatePreview();
  updateControls();
  sendGradientDebounced();
}

function startDrag(id, eventClient) {
  isDragging = true;
  dragStopId = id;
  selectedStopId = id;
  renderStops(); 
  updateControls();
}


document.addEventListener('mousemove', (e) => {
  if (!isDragging) return;
  moveStop(e.clientX);
});
document.addEventListener('mouseup', () => {
  if (isDragging) {
    isDragging = false;
    dragStopId = null;
    sendGradientDebounced();
  }
});

document.addEventListener('touchmove', (e) => {
  if (!isDragging) return;
  e.preventDefault();
  moveStop(e.touches[0].clientX);
}, {passive: false});
document.addEventListener('touchend', () => {
  if (isDragging) {
    isDragging = false;
    sendGradientDebounced();
  }
});

function moveStop(clientX) {
  const rect = gradBox.getBoundingClientRect();
  let x = clientX - rect.left;
  let percent = (x / rect.width) * 100;
  percent = Math.max(0, Math.min(100, percent));

  const stop = gradientStops.find(s => s.id === dragStopId);
  if (stop) {
    stop.pos = percent;
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

    // console.log("Sending:", payload);

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

    state.brightness = e.target.value;
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

  <div class="card">
    <h3 class="secondary-title">Настройка сети (STA mode)</h3>
    <div class="textfield-container">
      <label for="ssid-sta">Название сети</label>
      <input type="text" class="textfield" id="ssid-sta">
    </div>
    <div class="textfield-container">
      <label for="pass-sta">Пароль сети</label>
      <input type="text" class="textfield" id="pass-sta">
    </div>
    <div class="checkbox-container">
      <input type="checkbox" class="checkbox" id="sta-mode">
      <label for="sta-mode">Подключаться к существующей сети (режим STA)</label>
    </div>
    <button class="btn btn-primary" id="send-sta">Сохранить</button>
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
    <button class="btn btn-primary" onclick="connectToWifi()">Подключиться к сети</button>
    <button class="btn btn-primary" onclick="reboot()">Перезапустить</button>
    <button class="btn btn-primary" onclick="update()">Обновить прошивку</button>
  </div>
</div>

<script>
  const settings = {
)rawliteral";

    
    
    
    
    

const char html_settings_p2[] PROGMEM = R"rawliteral(
  const ssidStaInput = document.getElementById('ssid-sta');
  const passStaInput = document.getElementById('pass-sta');
  const staModeCheckbox = document.getElementById('sta-mode');
  const ssidApInput = document.getElementById('ssid-ap');
  const passApInput = document.getElementById('pass-ap');
  const passReqApCheckbox = document.getElementById('pass-req-ap');

  document.addEventListener('DOMContentLoaded', (event) => {
    ssidStaInput.value = settings.ssidSta;
    passStaInput.value = settings.passwordSta;
    staModeCheckbox.checked = settings.useStaMode;
    ssidApInput.value = settings.ssidAp;
    passApInput.value = settings.passwordAp;
    passReqApCheckbox.checked = settings.requiresPassAp;
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

  document.getElementById('send-sta').addEventListener('click', () => {
    const ssidSta = ssidStaInput.value;
    const passSta = passStaInput.value;
    const useStaMode = staModeCheckbox.checked;

    fetch('/api/save_sta', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        ssid: ssidSta,
        password: passSta,
        useStaMode: useStaMode
      })
    })
    .then(response => {
      if (response.ok) {
        alert("STA настройки сохранены!");
      } else {
        alert("Ошибка сохранения STA настроек.");
      }
    })
    .catch(error => {
      console.error("Ошибка сети:", error);
    });
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
  function connectToWifi() {
    fetch('/api/connect', { method: 'POST' })
      .then(() => {
        window.location.href = '/connecting';
      });
  }
</script>
)rawliteral";

void sendIndex(AsyncWebServerRequest *request, const uint8_t brightness, const uint8_t mode, const uint8_t speed, const char* colorHex, const GradientStop* gradientStops, const int stopsCount, const bool power) {
  auto currentPart = std::make_shared<uint8_t>(0);
  auto sentBytes = std::make_shared<size_t>(0);

  AsyncWebServerResponse *response = new AsyncChunkedResponse("text/html",
    [=](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      size_t len = 0;
      size_t totalLen;
      size_t remaining;

      while(true)
      {
        switch((*currentPart)){
          case 0:
            totalLen = strlen_P(html_template_p1);
            if (*sentBytes >= totalLen) {
              (*currentPart) = 1;
              *sentBytes = 0;
              continue;
            } else {
              remaining = totalLen - (*sentBytes);
              len = remaining > maxLen ? maxLen : remaining;
              memcpy_P(buffer, html_template_p1 + (*sentBytes), len);
              *sentBytes += len;
              break; 
            }

          case 1:
            totalLen = strlen_P(html_index_p1);
            if (*sentBytes >= totalLen) {
              (*currentPart) = 2;
              *sentBytes = 0;
              continue;
            } else {
              remaining = totalLen - (*sentBytes);
              len = remaining > maxLen ? maxLen : remaining;
              memcpy_P(buffer, html_index_p1 + (*sentBytes), len);
              *sentBytes += len;
              break;
            }

          case 2:
            {
              int offset = 0;
              offset += snprintf((char*)buffer + offset, maxLen - offset,
                "brightness: %d, mode: %d, speed: %d, power: %s, color: '%s', gSs: [",
                brightness, mode, speed, power ? "true" : "false", colorHex);

              for (size_t i = 0; i < stopsCount; i++) {
                if ((maxLen - offset) < 64) break;

                offset += snprintf((char*)buffer + offset, maxLen - offset,
                  "{pos:%.2f,color:'%s'}%s", 
                  gradientStops[i].position, 
                  gradientStops[i].colorHex,
                  (i < stopsCount - 1) ? "," : ""
                );
              }
              offset += snprintf((char*)buffer + offset, maxLen - offset, "]};\n");

              len = offset;
              
              (*currentPart)++;
            }
            break;


          case 3:
            totalLen = strlen_P(html_index_p2);
            if (*sentBytes >= totalLen) {
              (*currentPart) = 4;
              *sentBytes = 0;
              continue;
            } else {
              remaining = totalLen - (*sentBytes);
              len = remaining > maxLen ? maxLen : remaining;
              memcpy_P(buffer, html_index_p2 + (*sentBytes), len);
              *sentBytes += len;
              break;
            }

          case 4:
            len = strlen_P(html_template_p2);
            if (len > maxLen) len = maxLen; 
            memcpy_P(buffer, html_template_p2, len);
            (*currentPart)++;
            break;

          case 5:
            return 0;
        }
        return len;
      }
      return 0;
    });

  request->send(response);
}

// void sendIndex(AsyncWebServerRequest *request, const uint8_t brightness, const uint8_t mode, const uint8_t speed, const char* colorHex) {
//   Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
//   AsyncResponseStream *response = request->beginResponseStream("text/html");

//   response->print(FPSTR(html_template_p1));
//   response->print(FPSTR(html_index_p1));

//   response->printf("brightness: %d, mode: %d, speed: %d, power: %s, color: '%s'};\n", brightness, mode, speed, mode != 0 ? "1" : "0", colorHex);

//   response->print(FPSTR(html_index_p2));
//   response->print(FPSTR(html_template_p2));

//   request->send(response);
  
// }

// void sendIndex(AsyncWebServerRequest *request, const uint8_t brightness, const uint8_t mode, const uint8_t speed, const char* colorHex) {
//   AsyncResponseStream *response = request->beginResponseStream("text/html");
//   for(int i = 0; i < 5000; i++){
//     response->printf("%d ", i);
//     if(i % 100 == 0){
//       Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
//     }
//   }
//   Serial.printf("Freeeeeeee heap: %d\n", ESP.getFreeHeap());
//   request->send(response);
  
// }

void sendWrap(AsyncWebServerRequest *request, const char* progmem_content) {
  AsyncResponseStream *response = request->beginResponseStream("text/html");

  response->print(FPSTR(html_template_p1));
  response->print(FPSTR(progmem_content));
  response->print(FPSTR(html_template_p2));

  request->send(response);
}