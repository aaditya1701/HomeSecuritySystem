#include "ESPAsyncWebServer.h"
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>

StaticJsonDocument<3> sensorDataJson;

#define buzzerPin 32
#define flameSensorPin 35
#define gasSensorPin 27
#define pirSensorPin 33
#define ledPin 12
#define aqiPin 34

AsyncWebServer server(80);
WebSocketsServer websockets(81);

bool fireButtonStatus = 1;
bool motionButtonStatus = 1;
bool lightButtonStatus = 1;
unsigned long previousMillis = 0;
const long interval = 2500;

void WebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.printf("[%u] Disconnected!\n", num);
        break;

    case WStype_CONNECTED:
    {
        IPAddress ip = websockets.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
        websockets.sendTXT(num, "Connected");
    }
    break;

    case WStype_TEXT:
    {
        String data = String((char *)payload);
        websockets.broadcastTXT((char *)payload);
        StaticJsonDocument<3> buttonStatusJson;
        deserializeJson(buttonStatusJson, data);

        fireButtonStatus = buttonStatusJson["fireButton"];
        motionButtonStatus = buttonStatusJson["motionButton"];
        lightButtonStatus = buttonStatusJson["lightButton"];

        break;
    }
    }
}

const char *ssid = "Galaxy M21";
const char *password = "00000000";
void setup()
{
    pinMode(buzzerPin, OUTPUT);
    pinMode(flameSensorPin, INPUT);
    pinMode(gasSensorPin, INPUT);
    pinMode(pirSensorPin, INPUT);
    //  pinMode(panicButtonPin,INPUT);
    //  digitalWrite(panicButtonPin,HIGH);
    pinMode(ledPin, OUTPUT);
    digitalWrite(buzzerPin, HIGH);
    delay(1500);
    digitalWrite(buzzerPin, LOW);

    Serial.begin(115200);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", R"=====(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Home Security and Automation</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #f0f0f0;
        }

        .container {
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background-color: #fff;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            position: relative;
            /* Added position relative for container */
        }

        h1 {
            text-align: center;
            font-weight: bold;
            margin-bottom: 30px;
            font-size: 24px;
            background-color: #333;
            color: #fff;
            padding: 10px 20px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
        }

        .box {
            display: flex;
            flex-wrap: wrap;
            justify-content: space-evenly;
            gap: 20px;
            margin-bottom: 20px;
        }

        .box-part {
            width: calc(50% - 20px);
            margin-bottom: 40px;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            position: relative;
            /* Added position relative for box-part */
        }

        h2 {
            text-align: center;
            font-size: 18px;
            margin-bottom: 10px;
        }

        .switch-container {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            border: 1px solid #ccc;
            border-radius: 10px;
            padding: 20px;
            width: 180px;
            background-color: #e0e0e0;
        }

        .switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 36px;
            border: none;
        }

        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }

        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            -webkit-transition: .4s;
            transition: .4s;
            border-radius: 36px;
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 28px;
            width: 28px;
            left: px;
            bottom: 4px;
            background-color: white;
            -webkit-transition: .4s;
            transition: .4s;
            border-radius: 50%;
        }

        input:checked+.slider {
            background-color: #2196F3;
        }

        input:focus+.slider {
            box-shadow: 0 0 1px #2196F3;
        }

        input:checked+.slider:before {
            -webkit-transform: translateX(32px);
            -ms-transform: translateX(32px);
            transform: translateX(32px);
        }

        .slider.round {
            border-radius: 36px;
        }

        .slider.round:before {
            border-radius: 50%;
        }

        /* Style for the image */
        .center-image {
            position: absolute;
            top: 52%;
            left: 49%;
            transform: translate(-50%, -50%);
            max-width: 26%;
            max-height: 26%;
        }
    </style>
</head>

<script>
    var connection = new WebSocket('ws://' + location.hostname + ':81');
    var aqi;
    connection.onmessage = function (event) {
        try {
            let sensorData;
            let buttonData;
            let receivedJson = JSON.parse(event.data);
            if (Object.keys(receivedJson).length === 3) 
            {
                sensorData = receivedJson;
                console.log(sensorData);
                aqi = sensorData.aqi;
                document.getElementById("aqi").innerHTML=aqi;
                if (sensorData.fireCount) 
                {
                    alert("Fire Detected");
                    location.reload();
                }
             else if (sensorData.motionCount)
             {
                alert("Motion Detected");
                location.reload();
             }
            } else {
                buttonData = receivedJson;
                if (buttonData.fireButton) {
                    document.getElementById("fireButton").checked = true;
                } else {
                    document.getElementById("fireButton").checked = false;
                }
                if (buttonData.motionButton) {
                    document.getElementById("motionButton").checked = true;
                } else {
                    document.getElementById("motionButton").checked = false;
                }
                if (buttonData.lightButton) {
                    document.getElementById("lightButton").checked = true;
                } else {
                    document.getElementById("lightButton").checked = false;
                }
            }

            
        } catch (e) {
            console.error(e.stack);
        }
    };

</script>

<body>
    <div class="container">
        <h1>HOME SECURITY AND AUTOMATION</h1>
        <div class="box">
            <div class="box-part">
                <div class="switch-container">
                    <h2>Fire Sensor</h2>
                    <label class="switch">
                        <input type="checkbox" id="fireButton" onclick="statusCheck()" checked>
                        <span class="slider round"></span>
                    </label>
                </div>
            </div>
            <div class="box-part">
                <div class="switch-container">
                    <h2>Motion Sensor</h2>
                    <label class="switch">
                        <input type="checkbox" id="motionButton" onclick="statusCheck()" checked>
                        <span class="slider round"></span>
                    </label>
                </div>
            </div>
            <div class="box-part">
                <div class="switch-container">
                    <h2>Lights</h2>
                    <label class="switch">
                        <input type="checkbox" id="lightButton" onclick="statusCheck()" checked>
                        <span class="slider round"></span>
                    </label>
                </div>
            </div>
            <div class="box-part">
                <div class="switch-container">
                    <h2>Air Quality Index</h2>
                        <label class="switch">
                    <h4 align="center" id="aqi">Processing....</h4>
                        </label>
                </div>
            </div>
        </div>
    </div>
</body>
<script>
    let switchData;

    function statusCheck() {
        switchData = {
            fireButton: document.getElementById("fireButton").checked,
            motionButton: document.getElementById("motionButton").checked,
            lightButton: document.getElementById("lightButton").checked,
//            panicButton: document.getElementById("panicButton").checked,
            isButtonData: true
        };
        console.log(switchData);
        connection.send(JSON.stringify(switchData));
    }

</script>

</html>
)====="); });

    server.begin();
    websockets.begin();
    websockets.onEvent(WebSocketEvent);

    if (MDNS.begin("HomeSecuritySystem"))
    {
        Serial.println("MDNS responder started");
    }
}

void loop()
{
    unsigned long currentMillis = millis();
    websockets.loop();
    bool fireCount = (digitalRead(flameSensorPin) == 0 || digitalRead(gasSensorPin) == 0);
    bool motionCount = (digitalRead(pirSensorPin) == 1);
    int aqiValue = ((analogRead(aqiPin)*7.77) / 100);
    String sensorData;
    if (lightButtonStatus)
    {
        digitalWrite(ledPin, HIGH);
    }
    else
    {
        digitalWrite(ledPin, LOW);
    }
    if (fireButtonStatus)
    {
        if (fireCount)
        {

            sensorData = dataJson(fireCount, motionCount, aqiValue);
            websockets.broadcastTXT(sensorData);
            while (fireCount)
            {

                Serial.println("Fire Alert");
                alarm(buzzerPin);
            }
        }
    }
    if (motionButtonStatus)
    {
        if (motionCount)
        {

            sensorData = dataJson(fireCount, motionCount, aqiValue);
            websockets.broadcastTXT(sensorData);
            while (motionCount)
            {

                Serial.println("Motion Alert");
                Serial.println(digitalRead(pirSensorPin));
                alarm(buzzerPin);
            }
        }
    }
    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        sensorData = dataJson(fireCount, motionCount, aqiValue);
        websockets.broadcastTXT(sensorData);
    }
}

void alarm(int buzzPin)
{
    int i = 0;
    for (i = 0; i < 255; i = i + 10)
    {
        analogWrite(buzzPin, i);
        delay(30);
    }
    i = 0;
    delay(200);
}

String dataJson(bool fireDetected, bool motionDetected, int aqi)
{
    sensorDataJson["fireCount"] = fireDetected;
    sensorDataJson["motionCount"] = motionDetected;
    sensorDataJson ["aqi"] = aqi;
    String jsonString = "";
    serializeJson(sensorDataJson, jsonString);
    return jsonString;
}
