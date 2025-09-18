// Master_ESPNOW webpage to be included with main Master_ESPNOW sketch
#ifndef WEBPAGE_H
#define WEBPAGE_H

const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>
<head>
  <title>Shuttle Run Status</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 20px;
      background: #f0f0f0;
    }
    h1 { text-align: center; }
    .container {
      max-width: 600px;
      margin: 0 auto;
      background: white;
      padding: 20px;
      border-radius: 10px;
      border: outset;
    }
    .status {
      display: flex;
      justify-content: center;
      margin: 10px 0;
      padding: 15px;
      border-radius: 5px;
      font-weight: bold;
      border: outset;
    }
    .idle { background: #e3f2fd; }
    .running { background: #fff3e0; }
    .success { background: #e8f5e8; }
    .failed { background: #ffebee; }

    .slaves, .master {
      display: flex;
      justify-content: space-around;
      margin-top: 20px;
    }
    .slave, .master {
      text-align: center;
      font-size: 1rem;
    }
    .slave span, .master span {
      display: block;
      margin-top: 5px;
      font-size: 0.9rem;
    }
  </style>
  <script>
    setInterval(() => fetch('/status').then(r => r.json()).then(updateStatus), 1000);

    function updateStatus(data) {
      // Overall game state
      const stateElem = document.getElementById('gameState');
      stateElem.textContent = data.state;
      stateElem.className = 'status ' + data.state.toLowerCase();

      document.getElementById('timer').textContent = data.timer + 's';
      document.getElementById('completionTime').textContent = data.completionTime + 's';

      // Master cone countdown / win-lose
      let masterPath = document.querySelector('#master path');
      if (masterPath && data.masterState) {
        let color;
        if (data.state === "running") {
          // Yellow/orange while counting down
          color = '#ffa500';
        } else if (data.state === "success") {
          // Green if win
          color = '#00ff00';
        } else if (data.state === "failed") {
          // Red if fail
          color = '#ff0000';
        } else {
          // Purple when idle/ready
          color = '#800080';
        }
        masterPath.setAttribute('fill', color);
      }

      // Update each slave cone
      for (let i = 2; i <= 5; i++) {
        let conePath = document.querySelector('#slave' + i + ' path');
        if (conePath && data.slavesState && data.slavesState[i]) {
          let state = data.slavesState[i];
          let color =
            state === 'pressed' ? '#00ff00' :   // green until reset
            state === 'error'   ? '#ff0000' :   // red flash
                                  '#800080';    // purple waiting
          conePath.setAttribute('fill', color);
        }
      }
    }
  </script>
</head>
<body>
  <div class="container">
    <h1><u>Shuttle Run Game</u></h1>

    <!-- Master Cone -->
    <div class="master">
      <div id="master">
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="70" height="70">
          <path d="M20 52h24l-8-28h-8zM14 60h36l2-6H12zM28 4h8l2 8h-12z" fill="#800080"/>
        </svg>
        <span class="label">Master</span>
      </div>
    </div>

    <!-- Slaves -->
    <div class="slaves">
      <div id="slave2" class="slave">
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="50" height="50">
          <path d="M20 52h24l-8-28h-8zM14 60h36l2-6H12zM28 4h8l2 8h-12z" fill="#800080"/>
        </svg>
        <span class="label">Cone 1</span>
      </div>
      <div id="slave3" class="slave">
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="50" height="50">
          <path d="M20 52h24l-8-28h-8zM14 60h36l2-6H12zM28 4h8l2 8h-12z" fill="#800080"/>
        </svg>
        <span class="label">Cone 2</span>
      </div>
      <div id="slave4" class="slave">
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="50" height="50">
          <path d="M20 52h24l-8-28h-8zM14 60h36l2-6H12zM28 4h8l2 8h-12z" fill="#800080"/>
        </svg>
        <span class="label">Cone 3</span>
      </div>
      <div id="slave5" class="slave">
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="50" height="50">
          <path d="M20 52h24l-8-28h-8zM14 60h36l2-6H12zM28 4h8l2 8h-12z" fill="#800080"/>
        </svg>
        <span class="label">Cone 4</span>
      </div>
    </div>

    <!-- Status + Timers -->
    <div id="gameState" class="status idle">READY</div>
    <div>Time Remaining: <span id="timer">--</span></div>
    <div>Completion Time: <span id="completionTime">--</span></div>
  </div>
</body>
</html>

)rawliteral";

#endif
