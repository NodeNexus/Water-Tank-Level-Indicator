// ----------------------------------------------------
// Water Tank Level Indicator with Pump Control: ESP32 Code
// Microcontroller: ESP32/ESP8266
// ----------------------------------------------------

#include <WiFi.h>
#include <WebServer.h>

// --- Configuration ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Pin Definitions
const int TRIG_PIN = 5;      // Ultrasonic Sensor Trig pin
const int ECHO_PIN = 18;     // Ultrasonic Sensor Echo pin
const int FLOAT_PIN = 19;    // Float Sensor pin (DANGER/FULL signal)
const int PUMP_RELAY_PIN = 27; // Relay pin (HIGH = Pump ON)

// Tank Dimensions and Control Thresholds
const float TANK_HEIGHT_CM = 100.0; // Total height of the tank in cm
const float MAX_DISTANCE_CM = 95.0; // Max distance sensor can read (empty tank)
const float MIN_DISTANCE_CM = 10.0; // Min distance sensor can read (full tank)

// Control Thresholds (as a percentage of tank full)
const int AUTO_START_LEVEL = 20; // Start pump when level is below 20%
const int AUTO_STOP_LEVEL = 95;  // Stop pump when level reaches 95%

// State Variables
bool autoMode = true; // Flag for automatic pump control
WebServer server(80);

// --- Functions ---

/**
 * Reads the distance from the ultrasonic sensor.
 * Returns the distance in centimeters.
 */
float readDistanceCm() {
  // Clear the Trig pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Send a 10us pulse to trigger the sensor
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure the duration of the echo pulse
  long duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate the distance (Sound speed = 0.0343 cm/us)
  float distanceCm = duration * 0.0343 / 2;

  // Simple filter for out-of-range values
  if (distanceCm > MAX_DISTANCE_CM * 1.5 || distanceCm < 0) {
    return -1.0; // Indicate error
  }

  return distanceCm;
}

/**
 * Calculates the water level percentage.
 * Returns level as an integer 0-100.
 */
int calculateLevel(float distanceCm) {
  // Ensure the distance is within the measurable range (from the sensor's perspective)
  if (distanceCm < MIN_DISTANCE_CM) distanceCm = MIN_DISTANCE_CM;
  if (distanceCm > MAX_DISTANCE_CM) distanceCm = MAX_DISTANCE_CM;

  // The water height is the max sensor distance minus the measured distance
  float waterHeight = MAX_DISTANCE_CM - distanceCm;

  // Calculate percentage based on the usable tank height range
  float usableRange = MAX_DISTANCE_CM - MIN_DISTANCE_CM;
  int levelPercent = map(waterHeight * 100, 0, usableRange * 100, 0, 100);

  // Clamp the result between 0 and 100
  return constrain(levelPercent, 0, 100);
}

/**
 * Core control logic for the water pump.
 */
void pumpControl(int levelPercent, bool floatSensorFull) {
  if (autoMode) {
    // 1. Check for emergency stop (Float Sensor overrides everything)
    if (floatSensorFull) {
      if (digitalRead(PUMP_RELAY_PIN) == HIGH) {
        digitalWrite(PUMP_RELAY_PIN, LOW); // Turn OFF pump
        Serial.println("SAFETY STOP: Float sensor triggered (Tank FULL).");
      }
      return; // Stop checking auto-logic
    }

    // 2. Auto Start Logic (when level is low)
    if (levelPercent <= AUTO_START_LEVEL) {
      if (digitalRead(PUMP_RELAY_PIN) == LOW) {
        digitalWrite(PUMP_RELAY_PIN, HIGH); // Turn ON pump
        Serial.printf("AUTO START: Level is %d%% (below %d%%).\n", levelPercent, AUTO_START_LEVEL);
      }
    } 
    // 3. Auto Stop Logic (when level is high)
    else if (levelPercent >= AUTO_STOP_LEVEL) {
      if (digitalRead(PUMP_RELAY_PIN) == HIGH) {
        digitalWrite(PUMP_RELAY_PIN, LOW); // Turn OFF pump
        Serial.printf("AUTO STOP: Level is %d%% (above %d%%).\n", levelPercent, AUTO_STOP_LEVEL);
      }
    }
  }
}

// --- Web Server Handlers ---

String getPage(int levelPercent, bool floatSensorFull) {
  // Simple HTML/CSS for the local web interface
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Water Tank Controller</title>
<style>
body { font-family: Arial, sans-serif; background: #e9ecef; margin: 0; padding: 20px;}
.container { max-width: 500px; margin: auto; background: #fff; padding: 30px; border-radius: 10px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); }
h1 { color: #007bff; text-align: center; }
.level-gauge { background: #f1f1f1; border: 1px solid #ccc; height: 200px; position: relative; border-radius: 8px; margin-bottom: 25px; overflow: hidden; }
.water-fill { position: absolute; bottom: 0; left: 0; width: 100%; background: #4dabf7; transition: height 0.5s; }
.level-text { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); font-size: 2.5em; font-weight: bold; color: #1e3a8a; text-shadow: 1px 1px 2px white; z-index: 10; }
.status-box { padding: 10px; border-radius: 6px; margin-bottom: 20px; text-align: center; font-weight: bold; }
.status-box.on { background-color: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
.status-box.off { background-color: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
.float-status { font-size: 0.9em; margin-top: 10px; color: %s; }
.control button { padding: 10px 15px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; font-size: 1em; width: 45%%; }
.control .auto { background-color: %s; color: white; }
.control .manual { background-color: %s; color: white; }
.thresholds { margin-top: 20px; font-size: 0.9em; color: #666; text-align: center;}
</style>
<script>
function sendCommand(mode, state) {
    fetch('/control?mode=' + mode + '&state=' + state, { method: 'POST' })
        .then(() => location.reload());
}
</script>
</head>
<body>
<div class="container">
<h1>Water Tank Monitor</h1>

<div class="level-gauge">
    <div class="water-fill" style="height: %d%%;"></div>
    <div class="level-text">%d%%</div>
</div>

<div class="status-box %s">
    Pump Status: %s
    <div class="float-status">Float Sensor Status: %s</div>
</div>

<h2>Control Panel</h2>
<div class="control" style="text-align: center;">
    <button class="auto" onclick="sendCommand('mode', '%s')">
        %s Mode
    </button>
    <button class="manual" onclick="sendCommand('pump', '%s')">
        Pump %s
    </button>
</div>

<div class="thresholds">
    Auto Start Below: %d%% | Auto Stop Above: %d%%
</div>
</div>
</body>
</html>
)rawliteral";

  // Determine pump status for display
  bool pumpIsOn = (digitalRead(PUMP_RELAY_PIN) == HIGH);
  String pump_class = pumpIsOn ? "on" : "off";
  String pump_text = pumpIsOn ? "ON (Filling)" : "OFF";
  
  // Determine Float sensor status
  String float_text = floatSensorFull ? "Tank FULL (Safety ON)" : "Tank OK";
  String float_color = floatSensorFull ? "#dc3545" : "#28a745";

  // Determine control mode button states
  String mode_btn_text = autoMode ? "SWITCH TO MANUAL" : "SWITCH TO AUTO";
  String mode_btn_action = autoMode ? "manual" : "auto";
  String mode_btn_color = autoMode ? "#ffc107" : "#007bff";
  
  String pump_btn_text = pumpIsOn ? "MANUAL STOP" : "MANUAL START";
  String pump_btn_action = pumpIsOn ? "off" : "on";
  String pump_btn_color = pumpIsOn ? "#dc3545" : "#28a745";
  
  // Format the HTML content
  return String::format(
    html.c_str(),
    float_color.c_str(), // %s
    mode_btn_color.c_str(), // %s
    pump_btn_color.c_str(), // %s
    levelPercent, // %d
    levelPercent, // %d
    pump_class.c_str(), // %s
    pump_text.c_str(), // %s
    float_text.c_str(), // %s
    mode_btn_action.c_str(), // %s
    mode_btn_text.c_str(), // %s
    pump_btn_action.c_str(), // %s
    pump_btn_text.c_str(), // %s
    AUTO_START_LEVEL, // %d
    AUTO_STOP_LEVEL  // %d
  );
}

// Handler for the root page ("/")
void handleRoot() {
  float distance = readDistanceCm();
  int level = calculateLevel(distance);
  bool floatSensor = (digitalRead(FLOAT_PIN) == LOW); // LOW means water is HIGH/FULL
  server.send(200, "text/html", getPage(level, floatSensor));
}

// Handler for manual control
void handleControl() {
  if (server.hasArg("mode")) {
    String mode = server.arg("mode");
    if (mode == "auto") {
      autoMode = true;
      Serial.println("Control mode set to AUTO.");
    } else if (mode == "manual") {
      autoMode = false;
      Serial.println("Control mode set to MANUAL.");
    }
  }
  if (server.hasArg("pump")) {
    if (!autoMode) { // Only allow manual pump control when autoMode is OFF
      String state = server.arg("pump");
      if (state == "on") {
        digitalWrite(PUMP_RELAY_PIN, HIGH);
        Serial.println("Manual Pump ON.");
      } else if (state == "off") {
        digitalWrite(PUMP_RELAY_PIN, LOW);
        Serial.println("Manual Pump OFF.");
      }
    } else {
      Serial.println("Manual control denied: Auto mode is active.");
    }
  }
  // Redirect back to the main page
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);

  // Pin setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(FLOAT_PIN, INPUT_PULLUP); // Use INPUT_PULLUP, assuming float sensor pulls LOW when water is HIGH
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, LOW); // Ensure pump starts OFF

  // --- WiFi Connection ---
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // --- Web Server Initialization ---
  server.on("/", HTTP_GET, handleRoot);
  server.on("/control", HTTP_POST, handleControl);
  server.begin();
  Serial.println("HTTP Server started.");
}

void loop() {
  server.handleClient(); // Handle incoming web requests

  // Read Sensors
  float distance = readDistanceCm();
  bool floatSensorFull = (digitalRead(FLOAT_PIN) == LOW); // LOW for full tank

  if (distance != -1.0) { // Check for valid ultrasonic reading
    int level = calculateLevel(distance);
    pumpControl(level, floatSensorFull); // Run the control logic
    Serial.printf("Level: %d%% | Float: %s | Pump: %s\n", 
                  level, floatSensorFull ? "FULL" : "OK", 
                  digitalRead(PUMP_RELAY_PIN) == HIGH ? "ON" : "OFF");
  } else {
    Serial.println("Ultrasonic read error. Maintaining current pump state.");
  }
  
  delay(2000); // Check and update every 2 seconds
}
