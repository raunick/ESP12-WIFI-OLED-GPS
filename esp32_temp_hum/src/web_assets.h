#ifndef WEB_ASSETS_H
#define WEB_ASSETS_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Environment Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg-color: #121212;
      --card-bg: #1e1e1e;
      --text-primary: #ffffff;
      --text-secondary: #b0b0b0;
      --accent-temp: #ff5252;
      --accent-hum: #448aff;
      --glass: rgba(255, 255, 255, 0.05);
    }
    body {
      font-family: 'Roboto', sans-serif;
      background-color: var(--bg-color);
      color: var(--text-primary);
      margin: 0;
      padding: 20px;
      display: flex;
      flex-direction: column;
      align-items: center;
      min-height: 100vh;
    }
    h1 { font-weight: 300; letter-spacing: 2px; margin-bottom: 30px; }
    .status-bar {
      display: flex;
      gap: 15px;
      margin-bottom: 30px;
      font-size: 0.9rem;
      color: var(--text-secondary);
    }
    .pulse {
      width: 10px; height: 10px;
      background-color: #00e676;
      border-radius: 50%;
      box-shadow: 0 0 0 rgba(0, 230, 118, 0.4);
      animation: pulse 2s infinite;
    }
    @keyframes pulse {
      0% { box-shadow: 0 0 0 0 rgba(0, 230, 118, 0.4); }
      70% { box-shadow: 0 0 0 10px rgba(0, 230, 118, 0); }
      100% { box-shadow: 0 0 0 0 rgba(0, 230, 118, 0); }
    }
    .dashboard {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 20px;
      width: 100%;
      max-width: 800px;
      margin-bottom: 30px;
    }
    .card {
      background: var(--card-bg);
      border-radius: 15px;
      padding: 25px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.5);
      position: relative;
      overflow: hidden;
      transition: transform 0.3s ease;
    }
    .card:hover { transform: translateY(-5px); }
    .card::before {
      content: '';
      position: absolute;
      top: 0; left: 0; right: 0; height: 4px;
    }
    .card-temp::before { background: linear-gradient(90deg, #ff8a80, #ff5252); }
    .card-hum::before { background: linear-gradient(90deg, #82b1ff, #448aff); }
    
    .card-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 20px;
    }
    .card-title { font-size: 1.2rem; font-weight: 700; color: var(--text-secondary); text-transform: uppercase; }
    .icon { font-size: 1.5rem; }
    
    .value-container { display: flex; align-items: baseline; gap: 5px; }
    .value { font-size: 4rem; font-weight: 700; }
    .unit { font-size: 1.5rem; color: var(--text-secondary); }
    
    .stats {
      margin-top: 20px;
      display: flex;
      justify-content: space-between;
      font-size: 0.9rem;
      color: var(--text-secondary);
      border-top: 1px solid rgba(255,255,255,0.1);
      padding-top: 15px;
    }
    
    .chart-container {
      background: var(--card-bg);
      border-radius: 15px;
      padding: 20px;
      width: 100%;
      max-width: 800px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.5);
    }
  </style>
</head>
<body>
  <div class="status-bar">
    <div style="display:flex; align-items:center; gap:8px;">
      <div class="pulse"></div> <span>SYSTEM ONLINE</span>
    </div>
    <div id="clock">00:00:00</div>
  </div>

  <h1>ESP32 MONITOR</h1>

  <div class="dashboard">
    <!-- Card Temperatura -->
    <div class="card card-temp">
      <div class="card-header">
        <span class="card-title">Temperatura</span>
        <span class="icon">🌡️</span>
      </div>
      <div class="value-container">
        <span id="temp" class="value">--</span>
        <span class="unit">°C</span>
      </div>
      <div class="stats">
        <span>Min: <span id="minTemp">--</span></span>
        <span>Max: <span id="maxTemp">--</span></span>
      </div>
    </div>

    <!-- Card Umidade -->
    <div class="card card-hum">
      <div class="card-header">
        <span class="card-title">Umidade</span>
        <span class="icon">💧</span>
      </div>
      <div class="value-container">
        <span id="hum" class="value">--</span>
        <span class="unit">%</span>
      </div>
      <div class="stats">
        <span>Min: <span id="minHum">--</span></span>
        <span>Max: <span id="maxHum">--</span></span>
      </div>
    </div>
  </div>

  <div class="chart-container">
    <canvas id="envChart"></canvas>
  </div>

  <script>
    // Clock
    setInterval(() => {
      document.getElementById('clock').innerText = new Date().toLocaleTimeString();
    }, 1000);

    // Chart Setup
    const ctx = document.getElementById('envChart').getContext('2d');
    const chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [{
          label: 'Temperatura (°C)',
          borderColor: '#ff5252',
          backgroundColor: 'rgba(255, 82, 82, 0.1)',
          yAxisID: 'y',
          data: [],
          tension: 0.4,
          fill: true
        }, {
          label: 'Umidade (%)',
          borderColor: '#448aff',
          backgroundColor: 'rgba(68, 138, 255, 0.1)',
          yAxisID: 'y1',
          data: [],
          tension: 0.4,
          fill: true
        }]
      },
      options: {
        responsive: true,
        interaction: { mode: 'index', intersect: false },
        scales: {
          x: { ticks: { color: '#b0b0b0' }, grid: { color: '#333' } },
          y: { 
            type: 'linear', display: true, position: 'left',
            ticks: { color: '#ff5252' }, grid: { color: '#333' }
          },
          y1: { 
            type: 'linear', display: true, position: 'right',
            grid: { drawOnChartArea: false }, ticks: { color: '#448aff' }
          }
        },
        plugins: { legend: { labels: { color: '#fff' } } }
      }
    });

    // Data Fetching
    let minT = 100, maxT = -100, minH = 100, maxH = 0;

    function fetchData() {
      fetch('/events')
        .then(response => response.json())
        .then(data => {
          // Update Values
          document.getElementById('temp').innerText = data.temperature.toFixed(1);
          document.getElementById('hum').innerText = data.humidity.toFixed(0);

          // Update Stats
          minT = Math.min(minT, data.temperature);
          maxT = Math.max(maxT, data.temperature);
          minH = Math.min(minH, data.humidity);
          maxH = Math.max(maxH, data.humidity);
          
          document.getElementById('minTemp').innerText = minT.toFixed(1);
          document.getElementById('maxTemp').innerText = maxT.toFixed(1);
          document.getElementById('minHum').innerText = minH.toFixed(0);
          document.getElementById('maxHum').innerText = maxH.toFixed(0);

          // Update Chart
          const now = new Date().toLocaleTimeString();
          if (chart.data.labels.length > 20) {
            chart.data.labels.shift();
            chart.data.datasets[0].data.shift();
            chart.data.datasets[1].data.shift();
          }
          chart.data.labels.push(now);
          chart.data.datasets[0].data.push(data.temperature);
          chart.data.datasets[1].data.push(data.humidity);
          chart.update();
        });
    }

    setInterval(fetchData, 2000);
  </script>
</body>
</html>
)rawliteral";

#endif
