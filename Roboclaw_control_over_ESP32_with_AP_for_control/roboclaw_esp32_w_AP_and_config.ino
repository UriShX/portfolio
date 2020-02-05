/*
 * A basic 2*DC motors controller based on ESP32 in AP mode for configuration and maintanence
 * Motor controller is Roboclaw, DC motors are DC geared motors w/ encoder.
 * 
 * Used to power a mechanized exhibit in a museum setting.
 *
 * Uri Shani, 09/2019
 *
 * changed sdk config AMPDU_TX and AMPDU_RX from 1 to 0 (disabled) for AP to work
 * based on https://github.com/espressif/arduino-esp32/issues/2382
 * 
 * AP based on https://resources.basicmicro.com/esp8266-advanced-control-of-roboclaw/
 *  
*/


//Includes required to use Roboclaw library
#include <RoboClaw.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <FS.h>
// #include "html_file.h"

//Serial2 pins
#define RXD2 16
#define TXD2 17
#define M1_HOME 27
#define M2_HOME 14

//RoboClaw address
#define address 0x80

//Velocity PID coefficients
#define Kp 4.1
#define Ki 0.5
#define Kd 0.25
unsigned int qpps = 6718; //44000

bool M1_fault = false; // false = no fault; true = could not home
bool M2_fault = false; // false = no fault; true = could not home
int smalldelay = 60; // value in seconds
int nSmallCycle = 3; // zero indexed
int smallCycleIndex = 0; // manual (if) loop
int delayPeriondInSeconds = 60 * 10; // self explainatory
int nTimesToMove = 4; // complete cycle movement
bool enableAutoMovement = true; // true = auto mode; false = manual mode
int speedMultiplier = 4; // speed = qpps * speedMultiplier
int accelMultiplier = 6; // acceleration = qpps * accelMultiplier
float decelMultiplier = 1; // deceleration = qpps * decelMultiplier
int distance = 120; // distance to move before deceleration begins

bool homing = false;

unsigned long currentTime = millis();
unsigned long smallCycleTime = millis();

Preferences preferences;
RoboClaw roboclaw(&Serial2,20000);

// Replace with your network credentials
const char* ssid     = "AP_SSID";
const char* password = "Password";

// Set web server port number to 80
WebServer server(80);

const char home_page[] = R"=====(
<!DOCTYPE html>
<html>
<head>
<script>
  function manualControl() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
 
      }
    };
    xhttp.open("GET", "manualorauto", true);
    xhttp.send();
  }
  function sendCommand(chan,state) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
 
      }
    };
    xhttp.open("GET", "handleMotor?motorChannel="+chan+"&motorState="+state, true);
    xhttp.send();
  }
  function home() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("homed").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "homeMotors", true);
    xhttp.send();
  }
  function move() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("movement").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "testmovement", true);
    xhttp.send();
  }
  function LoadSavePreferences(action) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("preferences").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "loadsavepreferences?loadorsave="+action, true);
    xhttp.send();
  }
  function downloadLog(action) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("log_status").innerHTML = this.responseText;
      } else if (this.readyState == 4) {
        _OBJECT_URL = URL.createObjectURL(this.response);

    		// Set href as a local object URL
    		document.querySelector('#save-file').setAttribute('href', _OBJECT_URL);
    		
    		// Set name of download
    		document.querySelector('#save-file').setAttribute('download', 'img.jpeg');
      }
    };
    xhttp.open("GET", "handleLogFile?action="+action, true);
    xhttp.send();
  }
  setInterval(function(){
   getManualOrAuto();
  }, 1000);
  setInterval(function(){
   getEnc_1();
  }, 1100);
  setInterval(function(){
   getEnc_2();
  }, 1200);
   setInterval(function(){
   getTemp();
  }, 5000);
   setInterval(function(){
   getVoltage();
  }, 2000);
  function getManualOrAuto() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if(this.readyState == 4 && this.status == 200){
        document.getElementById("manualOrAuto").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET","manualorautostate",true);
    xhttp.send();
  }
  function getEnc_1() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if(this.readyState == 4 && this.status == 200){
        document.getElementById("encoder_1_value").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET","readEncoder_1",true);
    xhttp.send();
  }
  function getEnc_2() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if(this.readyState == 4 && this.status == 200){
        document.getElementById("encoder_2_value").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET","readEncoder_2",true);
    xhttp.send();
  }
  function getTemp() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if(this.readyState == 4 && this.status == 200){
        document.getElementById("temp").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET","readTemp",true);
    xhttp.send();
  }
  function getVoltage() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if(this.readyState == 4 && this.status == 200){
        document.getElementById("voltage").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET","readVoltage",true);
    xhttp.send();
  }
</script>
<style>
      .round {
        border-radius: 25px;
        background-color: #cde6f5;
        width: 50%;
        height: 50%;
        margin: 20px;
        padding: 20px;
      }
      .round_big {
        border-radius: 25px;
        background-color: #cdf5f2;
        width: 50%;
        height: 50%;
        margin: 20px;
        padding: 20px;
      }
      h2 {
        color: #707078;
      }
      .button {
        background-color: #87919e;
        display: inline-block;
        border-radius: 10px;
        border: none;
        color: white;
        font-size: 15px;
        padding: 25px;
      }
      span {
        color: #707078;
      }
      p {
        color: #707078;
        font-size: 25px;
      }
    </style>
</head>
<body>
    <div class="round">
      <h2>Enable manual control</h2>
      <button type="button" class="button" onclick="manualControl()">Manual Control</button>
      <br />
      <p>Control: <span id="manualOrAuto"></span></p>
    </div>
    <div class="round">
      <h2>Home</h2>
      <button type="button" class="button" onclick="home()">Home Motors</button>
        <br />
        <p><span id="homed"></span></p>
    </div>
    <div class="round_big">
        <h2>Move Motors Manually</h2>
        <p>Encoder Count: <span id="encoder_1_value">0</span></p>
        <p>Encoder Count: <span id="encoder_2_value">0</span></p>
        <svg
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:cc="http://creativecommons.org/ns#"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   viewBox="0 0 455.93368 710.97366"
   xml:space="preserve"
   id="svg907"
   version="1.1"><metadata
     id="metadata913"><rdf:RDF><cc:Work
         rdf:about=""><dc:format>image/svg+xml</dc:format><dc:type
           rdf:resource="http://purl.org/dc/dcmitype/StillImage" /><dc:title></dc:title></cc:Work></rdf:RDF></metadata><defs
     id="defs911"><clipPath
       id="clipPath923"
       clipPathUnits="userSpaceOnUse"><path
         id="path921"
         d="m 1881,3584 h 5979 v 9167 H 1881 V 3584" /></clipPath><clipPath
       id="clipPath943"
       clipPathUnits="userSpaceOnUse"><path
         id="path941"
         d="M 0,0 H 9921 V 14031 H 0 V 0" /></clipPath><clipPath
       id="clipPath973"
       clipPathUnits="userSpaceOnUse"><path
         id="path971"
         d="M 0,0 H 9921 V 14031 H 0 V 0" /></clipPath><clipPath
       id="clipPath985"
       clipPathUnits="userSpaceOnUse"><path
         id="path983"
         d="M 0,0 H 9921 V 14031 H 0 V 0" /></clipPath><clipPath
       id="clipPath1091"
       clipPathUnits="userSpaceOnUse"><path
         id="path1089"
         d="M 0,0 H 9921 V 14031 H 0 V 0" /></clipPath><clipPath
       id="clipPath1103"
       clipPathUnits="userSpaceOnUse"><path
         id="path1101"
         d="M 0,0 H 9921 V 14031 H 0 V 0" /></clipPath></defs><g
     transform="matrix(0.08,0,0,-0.08,-161.66633,1008.88)"
     id="g915"><g
       id="g917"><g
         clip-path="url(#clipPath923)"
         id="g919"><path
           id="path925"
           style="fill:none;stroke:#000000;stroke-width:24;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:10;stroke-dasharray:none;stroke-opacity:1"
           d="M 6008,4683 V 8463 H 5890 V 4683 h 118" /><path
           id="path927"
           style="fill:none;stroke:#000000;stroke-width:24;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:10;stroke-dasharray:none;stroke-opacity:1"
           d="m 3764,7872 h 2126 v 118 H 3764 v -118" /><path
           id="path929"
           style="fill:none;stroke:#000000;stroke-width:24;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:10;stroke-dasharray:none;stroke-opacity:1"
           d="m 3646,4683 v 3780 h 118 V 4683 h -118" /><path
           id="path931"
           style="fill:none;stroke:#000000;stroke-width:24;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:10;stroke-dasharray:none;stroke-opacity:1"
           d="m 3764,5156 v 118 H 5890 V 5156 H 3764" /><path
           id="path933"
           style="fill:none;stroke:#000000;stroke-width:24;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:10;stroke-dasharray:none;stroke-opacity:1"
           d="M 2033,3736 H 7708 V 8109 L 2745,12599 H 2033 V 3736" /><path
           id="path935"
           style="fill:none;stroke:#000000;stroke-width:24;stroke-linecap:round;stroke-linejoin:round;stroke-miterlimit:10;stroke-dasharray:none;stroke-opacity:1"
           d="M 2035,3738 H 7706 V 8108 L 2744,12597 H 2035 V 3738" /></g></g><g
       transform="translate(128.34075,35.694545)"
       onclick="sendCommand('1','2')"
       id="M1_R"><rect
         style="opacity:1;fill:#63f15c;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"
         id="rect1278"
         width="580.51562"
         height="504.7962"
         x="6360.4321"
         y="-8127.2183"
         transform="scale(1,-1)"
         rx="107.73063"
         ry="125" /><path
         style="opacity:1;fill:#ff0000;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"
         id="path1280"
         d="m 6951.5,-8013.4106 -314.5576,181.6099 -314.5576,181.6099 0,-363.2199 0,-363.2198 314.5576,181.6099 z"
         transform="matrix(0.47517734,0,0,-0.5086381,3514.4504,3803.6304)" /></g><g
       onclick="sendCommand('1','1')"
       id="M1_L"
       transform="rotate(179.77952,4806.6309,7889.47)"><rect
         style="opacity:1;fill:#63f15c;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"
         id="rect1278-6"
         width="580.51562"
         height="504.7962"
         x="6360.4321"
         y="-8127.2183"
         transform="scale(1,-1)"
         rx="107.73063"
         ry="125" /><path
         style="opacity:1;fill:#ff0000;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"
         id="path1280-7"
         d="m 6951.5,-8013.4106 -314.5576,181.6099 -314.5576,181.6099 0,-363.2199 0,-363.2198 314.5576,181.6099 z"
         transform="matrix(0.47517734,0,0,-0.5086381,3514.4504,3803.6304)" /></g><g
       onclick="sendCommand('2','2')"
       id="M2_R"
       transform="translate(128.34075,-2678.555)"><rect
         style="opacity:1;fill:#63f15c;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"
         id="rect1278-3"
         width="580.51562"
         height="504.7962"
         x="6360.4321"
         y="-8127.2183"
         transform="scale(1,-1)"
         rx="107.73063"
         ry="125" /><path
         style="opacity:1;fill:#ff0000;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"
         id="path1280-5"
         d="m 6951.5,-8013.4106 -314.5576,181.6099 -314.5576,181.6099 0,-363.2199 0,-363.2198 314.5576,181.6099 z"
         transform="matrix(0.47517734,0,0,-0.5086381,3514.4504,3803.6304)" /></g><g
       onclick="sendCommand('2','1')"
       id="M2_L"
       transform="rotate(179.77952,4809.2435,6531.644)"><rect
         style="opacity:1;fill:#63f15c;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"
         id="rect1278-6-2"
         width="580.51562"
         height="504.7962"
         x="6360.4321"
         y="-8127.2183"
         transform="scale(1,-1)"
         rx="107.73063"
         ry="125" /><path
         style="opacity:1;fill:#ff0000;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"
         id="path1280-7-9"
         d="m 6951.5,-8013.4106 -314.5576,181.6099 -314.5576,181.6099 0,-363.2199 0,-363.2198 314.5576,181.6099 z"
         transform="matrix(0.47517734,0,0,-0.5086381,3514.4504,3803.6304)" /></g><g
       onclick="sendCommand('1','0')"
       id="M1_stop"
       transform="translate(-1779.8534,35.694545)"><rect
         style="opacity:1;fill:#63f15c;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"
         id="rect1278-2"
         width="580.51562"
         height="504.7962"
         x="6360.4321"
         y="-8127.2183"
         transform="scale(1,-1)"
         rx="107.73063"
         ry="125" /><rect
         ry="7.0754719"
         rx="67.826851"
         transform="scale(1,-1)"
         y="-8008.6748"
         x="6498.9888"
         height="267.70865"
         width="303.40311"
         id="rect5080"
         style="opacity:1;fill:#ff0000;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:14.1020689;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1" /></g><g
       onclick="sendCommand('2','0')"
       id="M2_stop"
       transform="translate(-1779.8534,-2678.555)"><rect
         style="opacity:1;fill:#63f15c;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1"
         id="rect1278-2-3"
         width="580.51562"
         height="504.7962"
         x="6360.4321"
         y="-8127.2183"
         transform="scale(1,-1)"
         rx="107.73063"
         ry="125" /><rect
         ry="7.0754719"
         rx="67.826851"
         transform="scale(1,-1)"
         y="-8008.6748"
         x="6498.9888"
         height="267.70865"
         width="303.40311"
         id="rect5080-6"
         style="opacity:1;fill:#ff0000;fill-opacity:1;fill-rule:evenodd;stroke:none;stroke-width:14.1020689;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:none;stroke-opacity:1" /></g><rect
       ry="125"
       rx="125"
       transform="scale(1,-1)"
       y="-8841.9111"
       x="2383.1101"
       height="1463.9089"
       width="4997.4824"
       id="rect5141"
       style="opacity:1;fill:none;fill-opacity:1;fill-rule:evenodd;stroke:#807171;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:70.86614175, 23.62204725;stroke-dashoffset:0;stroke-opacity:1" /><text
       transform="scale(1,-1)"
       id="text5145"
       y="-8390.8818"
       x="4608.6787"
       style="font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:500px;line-height:1.25;font-family:'Courier 10 Pitch';-inkscape-font-specification:'Courier 10 Pitch';letter-spacing:0px;word-spacing:0px;fill:#000000;fill-opacity:1;stroke:none;stroke-width:12.5"
       xml:space="preserve"><tspan
         style="stroke-width:12.5"
         y="-8390.8818"
         x="4608.6787"
         id="tspan5143">M1</tspan></text>
<rect
       transform="scale(1,-1)"
       ry="125"
       rx="125"
       y="-6115.5767"
       x="2404.4172"
       height="1463.9089"
       width="4997.4824"
       id="rect5141-0"
       style="opacity:1;fill:none;fill-opacity:1;fill-rule:evenodd;stroke:#807171;stroke-width:23.62204742;stroke-linejoin:miter;stroke-miterlimit:4;stroke-dasharray:70.86614175, 23.62204725;stroke-dashoffset:0;stroke-opacity:1" /><text
       transform="scale(1,-1)"
       id="text5145-6"
       y="-5664.5474"
       x="4629.9863"
       style="font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:500px;line-height:1.25;font-family:'Courier 10 Pitch';-inkscape-font-specification:'Courier 10 Pitch';letter-spacing:0px;word-spacing:0px;fill:#000000;fill-opacity:1;stroke:none;stroke-width:12.5"
       xml:space="preserve"><tspan
         style="stroke-width:12.5"
         y="-5664.5474"
         x="4629.9863"
         id="tspan5143-2">M2</tspan></text>
</g></svg>
    </div>
    <div class="round">
      <h2>Test movement</h2>
      <button type="button" class="button" onclick="move()">Test movement</button>
        <br />
        <p><span id="movement"></span></p>
    </div>
    <div class="round">
      <h2>Info</h2>
        <p>Voltage: <span id="voltage">0</span>V</p>
        <p>Temperature: <span id="temp">0</span>C</p>
    </div>
    <div class="round">
      <h2>Motor Channel 1</h2>
      <button type="button" class="button" onclick="sendCommand('1','2')">Move to wall</button>
      <button type="button" class="button" onclick="sendCommand('1','1')">Move away from wall</button>
        <button type="button" class="button" onclick="sendCommand('1','0')">Motor Off</button>
        <br />
        <br />
        <p>Encoder Count: <span id="encoder_1_value">0</span></p>
    </div>
    <div class="round">
      <h2>Motor Channel 2</h2>
      <button type="button" class="button" onclick="sendCommand('2','2')">Move to wall</button>
      <button type="button" class="button" onclick="sendCommand('2','1')">Move away from wall</button>
        <button type="button" class="button" onclick="sendCommand('2','0')">Motor Off</button>
        <br />
        <br />
        <p>Encoder Count: <span id="encoder_2_value">0</span></p>
    </div>
    <div class="round">
      <h2>Save state</h2>
      <button type="button" class="button" onclick="LoadSavePreferences(2)">Clear preferences and return to defaults</button>
      <br />
      <p>Confirmation: <span id="preferences"></span></p>
    </div>
    <div class="round">
      <h2>Handle log</h2>
      <button type="button" class="button" onclick="downloadLog(0)">Download log file</button>
      <br />
      <button type="button" class="button" onclick="downloadLog(1)">Delete log file</button>
      <br />
      <p>Confirmation: <span id="log_status"></span></p>
    </div>
    
</body>
</html>
)=====";

// Variable to store the HTTP request
String header;

void movement();
void displayspeed();
void home(uint16_t targetInit);
void handlePreferences();
void handleHome();
void homeMotors();
void handleManualOrAuto();
void handleManualOrAutoState();
void handleEncoder_1();
void handleEncoder_2();
void handleMotor();
void handleTemp();
void handleVoltage();
void handleMovement();
void handleLogFile();

void setup() {
    pinMode(M1_HOME, INPUT_PULLUP);
    pinMode(M2_HOME, INPUT_PULLUP);

    preferences.begin("roboclaw_defines", false);
    M1_fault = preferences.getBool("M1_fault", false);
    M2_fault = preferences.getBool("M2_fault", false);
    preferences.end();

    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }

    delay(500);


    Serial.begin(115200);
    roboclaw.begin(115200);

    // Connect to Wi-Fi network with SSID and password, and only one allowed connection
    Serial.print("Setting AP (Access Point)…");
    WiFi.softAP(ssid, password, 1, 0, 1);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    server.on("/", handleHome);
    server.on("/readEncoder_1", handleEncoder_1);
    server.on("/readEncoder_2", handleEncoder_2);
    server.on("/readTemp", handleTemp);
    server.on("/readVoltage", handleVoltage);
    server.on("/handleMotor", handleMotor);
    server.on("/manualorauto", handleManualOrAuto);
    server.on("/manualorautostate", handleManualOrAutoState);
    server.on("/homeMotors", homeMotors);
    server.on("/testmovement",handleMovement);
    server.on("/loadsavepreferences",handlePreferences);
    server.on("/handleLogFile", handleLogFile);

    server.begin(); 
      
    delay(10);

    roboclaw.ResetEncoders(address);

    home(0);

    delay(10);
    // roboclaw.SetPinFunctions(address, 0, 3, 0); // set pin 4 as voltage clamp

    if (!M1_fault && !M2_fault) {
      movement();
      delay(100);
      home(100); 
    }
}

void loop() { //Choose Serial1 or Serial2 as required
  server.handleClient();
  if ((millis() - currentTime > (delayPeriondInSeconds - (smalldelay * nSmallCycle)) * 1000) && enableAutoMovement)
  {
    if (smallCycleIndex < nSmallCycle)
    {
      if ((millis() - smallCycleTime > smalldelay * 1000) && enableAutoMovement)
      {
        // server.handleClient();
        smallCycleTime = millis();
        movement();
        delay(100);
        home(150);
        smallCycleIndex++;
      }
      delay(1);
    }
    else 
    {
      smallCycleIndex = 0;
      currentTime = millis();
    }
  delay(1);
  }
}

void movement()
{
    uint8_t depth1,depth2;
    unsigned int qppsM1 = qpps;
    unsigned int qppsM2 = 0.98 * qpps;

    if (!M1_fault && !M2_fault)
    {
      // roboclaw.SpeedAccelDistanceM1M2(address, uint32_t accel, uint32_t speed1, uint32_t distance1, uint32_t speed2, uint32_t distance2, uint8_t flag=0);
      roboclaw.SpeedAccelDistanceM1(address,qppsM1*accelMultiplier,qppsM1*speedMultiplier,distance,1);
      roboclaw.SpeedAccelDistanceM2(address,qppsM2*accelMultiplier,qppsM2*speedMultiplier,distance,1);
      roboclaw.SpeedAccelDistanceM1(address,qppsM1*decelMultiplier,0,0);  //distance traveled is v*v/2a = 12000*12000/2*12000 = 6000
      roboclaw.SpeedAccelDistanceM2(address,qppsM2*decelMultiplier,0,0);  //that makes the total move in ondirection 48000
      do{
          // displayspeed();
          roboclaw.ReadBuffers(address,depth1,depth2);
      }while(depth1!=0x80 && depth2!=0x80);	//loop until distance command completes

      delay(10);

      for (int i = 0; i < 4; i++) {
          // roboclaw.SpeedAccelDistanceM1M2(address, uint32_t accel, uint32_t speed1, uint32_t distance1, uint32_t speed2, uint32_t distance2, uint8_t flag=0);
          roboclaw.SpeedAccelDistanceM1(address,qppsM1*accelMultiplier,-qppsM1*speedMultiplier,distance*2,1);
          roboclaw.SpeedAccelDistanceM2(address,qppsM2*accelMultiplier,-qppsM2*speedMultiplier,distance*2,1);
          roboclaw.SpeedAccelDistanceM1(address,qppsM1*decelMultiplier,0,0);  //distance traveled is v*v/2a = 12000*12000/2*12000 = 6000
          roboclaw.SpeedAccelDistanceM2(address,qppsM2*decelMultiplier,0,0);  //that makes the total move in ondirection 48000
          do{
              // displayspeed();
              roboclaw.ReadBuffers(address,depth1,depth2);
          }while(depth1!=0x80 && depth2!=0x80);	//loop until distance command completes

          delay(10);
          
          roboclaw.SpeedAccelDistanceM1(address,qppsM1*accelMultiplier,qppsM1*speedMultiplier,distance*2,1);
          roboclaw.SpeedAccelDistanceM2(address,qppsM2*accelMultiplier,qppsM2*speedMultiplier,distance*2,1);
          roboclaw.SpeedAccelDistanceM1(address,qppsM1*decelMultiplier,0,0);
          roboclaw.SpeedAccelDistanceM2(address,qppsM2*decelMultiplier,0,0);
          do{
              // displayspeed();
              roboclaw.ReadBuffers(address,depth1,depth2);
          }while(depth1!=0x80 && depth2!=0x80);	//loop until distance command completes

          delay(10);
      }

      // roboclaw.SpeedAccelDistanceM1M2(address, uint32_t accel, uint32_t speed1, uint32_t distance1, uint32_t speed2, uint32_t distance2, uint8_t flag=0);
      roboclaw.SpeedAccelDistanceM1(address,qppsM1*accelMultiplier,-qppsM1*speedMultiplier,distance,1);
      roboclaw.SpeedAccelDistanceM2(address,qppsM2*accelMultiplier,-qppsM2*speedMultiplier,distance,1);
      roboclaw.SpeedAccelDistanceM1(address,qppsM1*decelMultiplier,0,1);  //distance traveled is v*v/2a = 12000*12000/2*12000 = 6000
      roboclaw.SpeedAccelDistanceM2(address,qppsM2*decelMultiplier,0,1);  //that makes the total move in ondirection 48000
      do{
          // displayspeed();
          roboclaw.ReadBuffers(address,depth1,depth2);
      }while(depth1!=0x80 && depth2!=0x80);	//loop until distance command completes

      delay(10);
    }
}

void displayspeed()
{
    uint8_t status1,status2,status3,status4;
    bool valid1,valid2,valid3,valid4;
    
    int32_t enc1= roboclaw.ReadEncM1(address, &status1, &valid1);
    int32_t enc2 = roboclaw.ReadEncM2(address, &status2, &valid2);
    int32_t speed1 = roboclaw.ReadSpeedM1(address, &status3, &valid3);
    int32_t speed2 = roboclaw.ReadSpeedM2(address, &status4, &valid4);
    Serial.print("Encoder1:");
    if(valid1){
        Serial.print(enc1,DEC);
        Serial.print(" ");
        Serial.print(status1,HEX);
        Serial.print(" ");
    }
    else{
        Serial.print("failed ");
    }
    Serial.print("Encoder2:");
    if(valid2){
        Serial.print(enc2,DEC);
        Serial.print(" ");
        Serial.print(status2,HEX);
        Serial.print(" ");
    }
    else{
        Serial.print("failed ");
    }
    Serial.print("Speed1:");
    if(valid3){
        Serial.print(speed1,DEC);
        Serial.print(" ");
    }
    else{
        Serial.print("failed ");
    }
    Serial.print("Speed2:");
    if(valid4){
        Serial.print(speed2,DEC);
        Serial.print(" ");
    }
    else{
        Serial.print("failed ");
    }
    Serial.println();
}

void home(uint16_t targetInit)
{
    const uint8_t bitDirection = 1 << 1; // Bit 1 - Direction (0 = Forward, 1 = Backwards)
    bool M1_state = true;
    bool M2_state = true;
    int distance = 0;
    int speed = 100;
    uint16_t M1target = qpps / 4;
    uint16_t M2target = qpps / 4;

    uint8_t status1,status2;
    bool valid1,valid2;
    int32_t enc1= roboclaw.ReadEncM1(address, &status1, &valid1);
    int32_t enc2 = roboclaw.ReadEncM2(address, &status2, &valid2);
    if(valid1 && valid2){
        Serial.printf("Encoder1: %d\tEncoder2: %d\n", enc1, enc2);
    } else Serial.println("Error reading encoders");

    if (targetInit > 0) {
      if (enc1 != 0) {if (abs(enc1) > M1target) M1target = enc1;}
      if (enc2 != 0) {if (abs(enc2) > M2target) M2target = enc2;}
      M1target += targetInit;
      M2target += targetInit;
    }

    if (enc1 > 0) speed *= -1; // Bit 1 - Direction (0 = Forward, 1 = Backwards)
    do
    {
        roboclaw.SpeedDistanceM1(address, speed, 1, 1);
        distance++;
        M1_state = digitalRead(M1_HOME);
    } while (M1_state && distance < M1target);

    if (!M1_state) 
    {
      roboclaw.SpeedM1(address, 0);
      roboclaw.SetEncM1(address, 0);
      M1_fault = false;
    }
    else
    {
        distance = 0;
        do
        {
            roboclaw.SpeedDistanceM1(address, -speed, 1, 1);
            distance++;
            M1_state = digitalRead(M1_HOME);
        } while (M1_state && distance < M1target * 2);

        if (!M1_state)
        {
          roboclaw.SpeedM1(address, 0);
          roboclaw.SetEncM1(address, 0);
          M1_fault = false;
        }
        else 
        {
            do
            {
                roboclaw.SpeedDistanceM1(address, speed, 1, 1);
                distance--;
                M1_state = digitalRead(M1_HOME);
            } while (M1_state && distance > M1target);
            roboclaw.SpeedM1(address, 0);
            M1_fault = true;
            Serial.println("could not home M1");
        }
    }
    
    distance = 0;
    if (enc2 > 0 && speed < 0) speed *= -1; // Need to check
    do
    {
        roboclaw.SpeedDistanceM2(address, speed, 1, 1);
        distance++;
        M2_state = digitalRead(M2_HOME);
    } while (M2_state && distance < M2target);

    if (!M2_state) 
    {
      roboclaw.SpeedM2(address, 0);
      roboclaw.SetEncM2(address, 0);
      M2_fault = false;
    }
    else
    {
        distance = 0;
        do
        {
            roboclaw.SpeedDistanceM2(address, -speed, 1, 1);
            distance++;
            M2_state = digitalRead(M2_HOME);
        } while (M2_state && distance < M2target * 2);

        if (!M2_state) 
        {
          roboclaw.SpeedM2(address, 0);
          roboclaw.SetEncM2(address, 0);
          M2_fault = false;
        }
        else {
            do
            {
                roboclaw.SpeedDistanceM2(address, speed, 1, 1);
                distance--;
                M2_state = digitalRead(M2_HOME);
            } while (M2_state && distance > M2target);
            roboclaw.SpeedM2(address, 0);
            M2_fault = true;
            Serial.println("could not home M2");
        }
    }
    
    Preferences preferences;
    preferences.begin("roboclaw_defines", false);
    preferences.putBool("M1_fault", M1_fault);
    preferences.putBool("M2_fault", M2_fault);
    preferences.end();

    // Code to save log in SPIFFS
    // if (M1_fault || M2_fault)
    // {
    //   File file = (!SPIFFS.exists("/log.txt"))?SPIFFS.open("/log.txt", FILE_WRITE):SPIFFS.open("/log.txt", FILE_APPEND);      

    //   if(!file){
    //       Serial.println("There was an error opening the file for writing");
    //       return;
    //   }
      
    //   if(file.printf("enc1: %i, enc1_direction: %s, M1_fault: %s, enc2: %i, enc2_direction: %s, M2_fault: %s\n", enc1, (~bitDirection & status1)?"fwd":"bwd"), M1_fault?"fail":"success", enc2, (~bitDirection & status2)?"fwd":"bwd", M2_fault?"fail":"success") {
    //       Serial.println("File was written");
    //   }else {
    //       Serial.println("File write failed");
    //   }

    //   file.close();
    // }

    roboclaw.ResetEncoders(address);
}

void handlePreferences()
{
  String action = server.arg("loadorsave");
  if (action == "2") {
    Preferences preferences;
    preferences.clear();
    preferences.end();
    server.send(200, "text/html","cleared preferences");
  }
}

void handleHome(){
  String page = home_page;
  server.send(200, "text/html",page);
}

void homeMotors(){
  if (homing == false){
    homing = true;
    String status = "";
    if (!enableAutoMovement) {
      home(100);
      if (M1_fault) status += "M1 failed ";
      if (M2_fault) status += "M2 failed";
      if (!M1_fault && !M2_fault) status += "Homed successfully";
    }
    else status = "Cannot home in Auto Mode!";
    server.send(200, "text/html", status);
    homing = false;
  }
}

void handleMovement(){
  String status = "";
  if (!enableAutoMovement) {
    if (!M1_fault && !M2_fault) {
      movement();
      status += "Moved motors";
    }
    else {
      status += "Cannot home ";
      if (M1_fault) status += "M1 ";
      if (M2_fault) status += "M2";
    }
  }
  else status += "Cannot test movement in Auto Mode!";
  server.send(200, "text/html", status);
}

void handleManualOrAuto(){
  enableAutoMovement ^= true;
}

void handleManualOrAutoState(){
  String ManAutoState;
  if (enableAutoMovement) ManAutoState = "Auto";
  else if (!enableAutoMovement) ManAutoState = "Manual";
  server.send(200, "text/html",ManAutoState);
}

void handleEncoder_1(){
  int enc_value = roboclaw.ReadEncM1(address);
  String value = String(enc_value);
  server.send(200, "text/plain", value);
    
}

void handleEncoder_2(){
  int enc_value = roboclaw.ReadEncM2(address);
  String value = String(enc_value);
  server.send(200, "text/plain", value);
    
}

void handleMotor(){
  int speed = 100;
  String motor_channel = server.arg("motorChannel");
  String motor_state = server.arg("motorState");
  if (!enableAutoMovement) {
    if(motor_channel == "1") {
      if(motor_state == "1") {
        roboclaw.SpeedDistanceM1(address, speed, qpps/10, 0);
        // roboclaw.ForwardM1(address, 64);
      }
      else if(motor_state == "2") {
        roboclaw.SpeedDistanceM1(address, -speed, qpps/10, 0);
        // roboclaw.ForwardM1(address, 64);
      }
      else if(motor_state == "0") {
        roboclaw.SpeedDistanceM1(address, 0,0,0);
      }
    }

    else if(motor_channel == "2") {
      if(motor_state == "1") {
        roboclaw.SpeedDistanceM2(address, speed, qpps/10, 0);
        // roboclaw.ForwardM2(address, 64);
      }
      else if(motor_state == "2") {
        roboclaw.SpeedDistanceM2(address, -speed, qpps/10, 0);
        // roboclaw.ForwardM2(address, 64);
      }
      else if(motor_state == "0") {
        roboclaw.SpeedDistanceM2(address, 0,0,0);
      }
    }
  }
}

void handleTemp() {
  uint16_t temp = 0;
  roboclaw.ReadTemp(address, temp);
  float temp_f = (float)temp/10;
  String temp_str = String(temp_f);
  server.send(200, "text/plain", temp_str);
}

void handleVoltage() {
  int voltage = 0;
  voltage = roboclaw.ReadMainBatteryVoltage(address);
  float voltage_f = (float)voltage/10;
  String voltage_str = String(voltage_f);
  server.send(200, "text/plain", voltage_str);
}

void handleLogFile() {
  String action = server.arg("action");
  if (action == "0") {
    if (SPIFFS.exists("/log.txt")) {
      File download = SPIFFS.open("/log.txt", FILE_READ);
      if (download) {
        server.sendHeader("Content-Type", "text/text");
        server.sendHeader("Content-Disposition", "attachment; filename=log.txt");
        server.sendHeader("Connection", "close");
        server.streamFile(download, "application/octet-stream");
        download.close();
        server.client().stop(); // Stop is needed because no content length was sent (?)
      } else server.send(200, "text/plain", "/log.txt empty");
    } else server.send(200, "text/plain", "failed to retrieve /log.txt");
  } else if (action == "1") {
    if (SPIFFS.exists("/log.txt")) {
      SPIFFS.remove("/log.txt");
      server.send(200, "text/plain", "removed /log.txt");
    } else server.send(200, "text/plain", "did not find /log.txt");
  }
}