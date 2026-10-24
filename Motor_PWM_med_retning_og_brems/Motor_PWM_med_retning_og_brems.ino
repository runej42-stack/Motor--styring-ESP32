#include <WiFi.h>
#include <WebSocketsServer.h>
#include <driver/ledc.h>
#include <WebServer.h>

// ====== Nettverksinnstillinger ======
// Skriv inn navn og passord for ditt eget WiFi før du laster opp programmet.
const char* ssid     = "DITT_WIFI_NAVN";
const char* password = "DITT_WIFI_PASSORD";

// ====== Pins ======
const int motorPin = 7;     // PWM utgang
const int dirPin   = 8;     // Retning
const int brakePin = 9;     // Brems

// ====== PWM ======
const int pwmChannel = 0;
const int pwmFreq = 40000;      // 40 kHz
const int pwmResolutionBits = 8; // 8-bit (0..255)

// ====== Web & WS ======
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// ====== Auto-program variabler ======
bool autoMode = false;
int lowSpeed = 50;
int highSpeed = 200;
int lowTime = 500;   // ms (50..3000)
int highTime = 500;  // ms (50..3000)
unsigned long lowTimeMs = lowTime;
unsigned long highTimeMs = highTime;

enum AutoState { IDLE, RUN_LOW, RUN_HIGH };
AutoState autoState = IDLE;
unsigned long stateStart = 0;

// ====== Enkel HTML (slidere + knapper) ======
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Motorstyring ESP32</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 18px; }
    .slider { width: 95%; height: 60px; margin: 8px 0; }
    button { font-size: 18px; padding: 12px 28px; margin: 10px; }
    .small { font-size: 14px; color: #444; }
  </style>
</head>
<body>
  <h2>Motorstyring ESP32</h2>

  <h3>Manuell</h3>
  <input id="pwmSlider" type="range" min="0" max="255" value="0" class="slider">
  <p>Fart (RAW 0-255): <span id="pwmVal">0</span></p>
  <button onclick="sendCmd('DIR:TOGGLE')">Bytt Retning</button>
  <button onclick="sendCmd('BRAKE:TOGGLE')">Brems</button>

  <hr>
  <h3>Auto Program</h3>
  <p>Lav hastighet: <span id="lowSpeedVal">50</span></p>
  <input id="lowSpeed" type="range" min="0" max="255" value="50" class="slider">
  <p>Høy hastighet: <span id="highSpeedVal">200</span></p>
  <input id="highSpeed" type="range" min="0" max="255" value="200" class="slider">
  <p>Tid lav (ms): <span id="lowTimeVal">500</span></p>
  <input id="lowTime" type="range" min="50" max="3000" value="500" class="slider">
  <p>Tid høy (ms): <span id="highTimeVal">500</span></p>
  <input id="highTime" type="range" min="50" max="3000" value="500" class="slider">

  <p><button onclick="sendCmd('AUTO:TOGGLE')">Start/Stop Auto</button></p>

<script>
  var ws;
  function init() {
    ws = new WebSocket('ws://' + window.location.hostname + ':81/');
    ws.onopen = function(){ console.log('ws open'); };
    ws.onmessage = function(evt){ console.log('recv', evt.data); };

    var pwm = document.getElementById('pwmSlider');
    var pwmVal = document.getElementById('pwmVal');
    pwm.oninput = function() {
      pwmVal.innerText = this.value;
      ws.send('PWM:' + this.value);
    };

    var ls = document.getElementById('lowSpeed');
    var hs = document.getElementById('highSpeed');
    var lt = document.getElementById('lowTime');
    var ht = document.getElementById('highTime');

    ls.oninput = function(){ document.getElementById('lowSpeedVal').innerText = this.value; ws.send('AUTO:LOWSPD:' + this.value); };
    hs.oninput = function(){ document.getElementById('highSpeedVal').innerText = this.value; ws.send('AUTO:HGHSPD:' + this.value); };
    lt.oninput = function(){ document.getElementById('lowTimeVal').innerText = this.value; ws.send('AUTO:LOWT:' + this.value); };
    ht.oninput = function(){ document.getElementById('highTimeVal').innerText = this.value; ws.send('AUTO:HGHT:' + this.value); };
  }

  function sendCmd(cmd){
    if (ws && ws.readyState === WebSocket.OPEN) ws.send(cmd);
  }
  window.addEventListener('load', init);
</script>
</body>
</html>
)rawliteral";

// ====== Hjelpefunksjon ======
int valueAfterLastColon(const String &s) {
  int idx = s.lastIndexOf(':');
  if (idx < 0) return 0;
  return s.substring(idx + 1).toInt();
}

// ====== WebSocket event ======
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type != WStype_TEXT) return;
  String msg = String((char*)payload);
  Serial.printf("WS mottatt: %s\n", msg.c_str());

  if (msg.startsWith("PWM:")) {
    int duty = constrain(valueAfterLastColon(msg), 0, 255);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)pwmChannel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)pwmChannel);
    autoMode = false;
    autoState = IDLE;
    Serial.printf("Manuell PWM %d -> auto OFF\n", duty);
  }
  else if (msg.startsWith("DIR:TOGGLE")) {
    digitalWrite(dirPin, !digitalRead(dirPin));
    Serial.printf("DIR toggled -> %d\n", digitalRead(dirPin));
  }
  else if (msg.startsWith("BRAKE:TOGGLE")) {
    digitalWrite(brakePin, !digitalRead(brakePin));
    Serial.printf("BRAKE toggled -> %d\n", digitalRead(brakePin));
  }
  else if (msg.startsWith("AUTO:LOWSPD:")) {
    lowSpeed = constrain(valueAfterLastColon(msg), 0, 255);
    Serial.printf("Set lowSpeed=%d\n", lowSpeed);
  }
  else if (msg.startsWith("AUTO:HGHSPD:")) {
    highSpeed = constrain(valueAfterLastColon(msg), 0, 255);
    Serial.printf("Set highSpeed=%d\n", highSpeed);
  }
  else if (msg.startsWith("AUTO:LOWT:")) {
    lowTime = constrain(valueAfterLastColon(msg), 50, 3000);
    lowTimeMs = (unsigned long)lowTime;
    Serial.printf("Set lowTime=%d ms\n", lowTime);
  }
  else if (msg.startsWith("AUTO:HGHT:")) {
    highTime = constrain(valueAfterLastColon(msg), 50, 3000);
    highTimeMs = (unsigned long)highTime;
    Serial.printf("Set highTime=%d ms\n", highTime);
  }
  else if (msg.startsWith("AUTO:TOGGLE")) {
    autoMode = !autoMode;
    if (autoMode) {
      autoState = RUN_LOW;
      stateStart = millis();
      lowTimeMs = (unsigned long)lowTime;
      highTimeMs = (unsigned long)highTime;
      ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)pwmChannel, lowSpeed);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)pwmChannel);
      Serial.println("Auto mode ON -> startet RUN_LOW");
    } else {
      autoState = IDLE;
      ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)pwmChannel, 0);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)pwmChannel);
      Serial.println("Auto mode OFF -> motor stoppet");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // ====== WiFi uten fast IP (DHCP) ======
  WiFi.begin(ssid, password);
  Serial.print("Kobler til WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // ====== HTTP-server ======
  server.on("/", [](){
    server.send_P(200, "text/html", htmlPage);
  });
  server.begin();

  // ====== WebSocket ======
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  // ====== PWM (LEDC) ======
  ledc_timer_config_t timer_cfg = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = (ledc_timer_bit_t)pwmResolutionBits,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = pwmFreq,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&timer_cfg);

  ledc_channel_config_t ch_cfg = {
    .gpio_num = motorPin,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = (ledc_channel_t)pwmChannel,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = 0,
    .hpoint = 0
  };
  ledc_channel_config(&ch_cfg);

  // ====== I/O ======
  pinMode(dirPin, OUTPUT);
  pinMode(brakePin, OUTPUT);
  digitalWrite(dirPin, LOW);
  digitalWrite(brakePin, LOW);

  Serial.println("Setup ferdig.");
}

void loop() {
  webSocket.loop();
  server.handleClient();

  if (!autoMode) return;

  unsigned long now = millis();
  unsigned long elapsed = now - stateStart;

  switch (autoState) {
    case RUN_LOW:
      if (elapsed >= lowTimeMs) {
        autoState = RUN_HIGH;
        stateStart = now;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)pwmChannel, highSpeed);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)pwmChannel);
        Serial.printf("Auto: bytter til RUN_HIGH (hast %d, varighet %lums)\n", highSpeed, highTimeMs);
      }
      break;

    case RUN_HIGH:
      if (elapsed >= highTimeMs) {
        autoState = RUN_LOW;
        stateStart = now;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)pwmChannel, lowSpeed);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)pwmChannel);
        Serial.printf("Auto: bytter til RUN_LOW (hast %d, varighet %lums)\n", lowSpeed, lowTimeMs);
      }
      break;

    default:
      break;
  }
}
