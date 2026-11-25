#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <esp_now.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"   // WIFI_SSID, WIFI_PASSWORD (copia secrets.example.h)

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Dati ricevuti via ESP-NOW
// L'ordine dei campi deve essere identico a quello del sensor node (temp, hum, lux, ter),
// altrimenti lux e ter arrivano scambiati.
typedef struct struct_message {
    float temp;
    float hum;
    int lux;
    int ter;
} struct_message;

struct_message myData;

// Variabili Wi-Fi
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Inizializzazione del server
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Array per memorizzare i dati storici
#define DATA_POINTS 100
float tempHistory[DATA_POINTS] = {0};
float humHistory[DATA_POINTS] = {0};
int terHistory[DATA_POINTS] = {0};
int luxHistory[DATA_POINTS] = {0};

// Callback per ricezione dati ESP-NOW
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&myData, incomingData, sizeof(myData));
    Serial.println("Data received via ESP-NOW:");
    Serial.printf("Temp: %.2f, Hum: %.2f, Ter: %d, Lux: %d\n", 
        myData.temp, myData.hum, myData.ter, myData.lux);

    // Aggiorna i dati storici
    for (int i = DATA_POINTS - 1; i > 0; i--) {
        tempHistory[i] = tempHistory[i - 1];
        humHistory[i] = humHistory[i - 1];
        terHistory[i] = terHistory[i - 1];
        luxHistory[i] = luxHistory[i - 1];
    }
    tempHistory[0] = myData.temp;
    humHistory[0] = myData.hum;
    terHistory[0] = myData.ter;
    luxHistory[0] = myData.lux;

    // Invia i dati aggiornati al client WebSocket
    String message = String(myData.temp) + "," + String(myData.hum) + "," + String(myData.ter) + "," + String(myData.lux);
    ws.textAll(message);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 10);
    display.print("Temp: ");
    display.println(myData.temp);
    display.print("Hum: ");
    display.println(myData.hum);
    display.print("Ter: ");
    display.println(myData.ter);
    display.print("Lux: ");
    display.println(myData.lux);
    display.display(); 
}

// Funzione per gestire gli eventi WebSocket
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
    } else if (type == WS_EVT_DATA) {
        Serial.println("WebSocket data received");
    }
}

// Funzione per generare la pagina HTML
String generateHTML() {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>ESP32 Data Monitor</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                text-align: center;
                margin: 0;
                padding: 0;
                background-color: #f0f8ff;
            }
            h1 {
                color: #333;
            }
            .chart-container {
                width: 45%;
                display: inline-block;
                margin: 10px;
            }
        </style>
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
        <script>
            var ws = new WebSocket('ws://' + window.location.hostname + '/ws');
            ws.onopen = function() {
                console.log('WebSocket connection established');
            };
            ws.onmessage = function(event) {
                console.log('Data received: ' + event.data);
                var data = event.data.split(',');
                updateCharts(data);
            };
            ws.onerror = function(error) {
                console.log('WebSocket error: ' + error);
            };
            ws.onclose = function() {
                console.log('WebSocket connection closed');
            };

            const tempData = [%TEMP%].reverse();
            const humData = [%HUM%].reverse();
            const terData = [%TER%].reverse();
            const luxData = [%LUX%].reverse();

            console.log('Initial tempData: ' + tempData);
            console.log('Initial humData: ' + humData);
            console.log('Initial terData: ' + terData);
            console.log('Initial luxData: ' + luxData);

            var tempChart, humChart, terChart, luxChart;

            function createChart(ctx, label, data) {
                return new Chart(ctx, {
                    type: 'line',
                    data: {
                        labels: Array.from({length: data.length}, (_, i) => data.length - i),
                        datasets: [{
                            label: label,
                            data: data,
                            borderColor: 'rgba(75, 192, 192, 1)',
                            borderWidth: 2,
                            fill: false
                        }]
                    },
                    options: {
                        responsive: true,
                        scales: {
                            y: {
                                beginAtZero: true
                            }
                        }
                    }
                });
            }

            window.onload = function() {
                const tempCtx = document.getElementById('tempChart').getContext('2d');
                tempChart = createChart(tempCtx, 'Temperature (°C)', tempData);

                const humCtx = document.getElementById('humChart').getContext('2d');
                humChart = createChart(humCtx, 'Humidity (%)', humData);

                const terCtx = document.getElementById('terChart').getContext('2d');
                terChart = createChart(terCtx, 'Terrain Moisture', terData);

                const luxCtx = document.getElementById('luxChart').getContext('2d');
                luxChart = createChart(luxCtx, 'Light Intensity (Lux)', luxData);
            };

            function updateCharts(data) {
                console.log('Updating charts with data: ' + data);
                tempChart.data.datasets[0].data.shift();
                tempChart.data.datasets[0].data.push(parseFloat(data[0]));
                tempChart.update();

                humChart.data.datasets[0].data.shift();
                humChart.data.datasets[0].data.push(parseFloat(data[1]));
                humChart.update();

                terChart.data.datasets[0].data.shift();
                terChart.data.datasets[0].data.push(parseInt(data[2]));
                terChart.update();

                luxChart.data.datasets[0].data.shift();
                luxChart.data.datasets[0].data.push(parseInt(data[3]));
                luxChart.update();
            }
        </script>
    </head>
    <body>
        <h1>ESP32 Data Monitor</h1>
        <div class="chart-container">
            <canvas id="tempChart"></canvas>
        </div>
        <div class="chart-container">
            <canvas id="humChart"></canvas>
        </div>
        <div class="chart-container">
            <canvas id="terChart"></canvas>
        </div>
        <div class="chart-container">
            <canvas id="luxChart"></canvas>
        </div>
    </body>
    </html>
    )rawliteral";

    // Sostituzione valori
    String tempValues = "";
    String humValues = "";
    String terValues = "";
    String luxValues = "";
    for (int i = 0; i < DATA_POINTS; i++) {
        tempValues += String(tempHistory[i]);
        humValues += String(humHistory[i]);
        terValues += String(terHistory[i]);
        luxValues += String(luxHistory[i]);
        if (i < DATA_POINTS - 1) {
            tempValues += ",";
            humValues += ",";
            terValues += ",";
            luxValues += ",";
        }
    }
    html.replace("%TEMP%", tempValues);
    html.replace("%HUM%", humValues);
    html.replace("%TER%", terValues);
    html.replace("%LUX%", luxValues);
    return html;
}

void setup() {
    // Serial monitor
    Serial.begin(115200);

    // Inizializza Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to Wi-Fi...");
    }
    Serial.println("Connected to Wi-Fi");

    // Stampa indirizzo IP
    Serial.println("IP Address: ");
    Serial.println(WiFi.localIP());

    // Inizializza ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

    // Configura WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Configura endpoint per la pagina HTML
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = generateHTML();
        Serial.println(html); // Aggiungi questo per vedere il contenuto della pagina HTML
        request->send(200, "text/html", html);
    });

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

    // Avvia server
    server.begin();
}

void loop() {
    ws.cleanupClients();
}
