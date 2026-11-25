#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h" // Libreria per la gestione del Wi-Fi
#include "DHT.h"
#include <Preferences.h>
#include "secrets.h"   // WIFI_SSID, WIFI_PASSWORD, RECEIVER_MAC (copia secrets.example.h)


// MAC Address del ricevitore (definito in secrets.h)
uint8_t broadcastAddress[] = RECEIVER_MAC;

// Definizione sensori e PIN
#define DHTPIN 4
#define DHTTYPE DHT11
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1         /* Time ESP32 will andare a riposo (in secondi) */
int pinter = 35;
int pinlux = 34;
Preferences preferences;


DHT dht(DHTPIN, DHTTYPE);

// Struttura dati da inviare
typedef struct struct_message {
    float temp;
    float hum;
    int lux;
    int ter;
} struct_message;

// Variabili globali
struct_message myData;
esp_now_peer_info_t peerInfo;

// Callback per l'invio
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nStato invio ultimo pacchetto:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Inviato con successo" : "Errore nell'invio");
    esp_deep_sleep_start();
}

// Funzione per ottenere il canale del router
int getWiFiChannel() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // credenziali in secrets.h
    Serial.print("Connessione al Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println("\nConnesso!");
    int channel = WiFi.channel();
    WiFi.disconnect(); // Disconnetti per liberare il Wi-Fi per ESP-NOW
    Serial.printf("Canale Wi-Fi trovato: %d\n", channel);
    return channel;
}

void setup() {
    // Monitor seriale
    Serial.begin(115200);
    int channel;
    preferences.begin("wifiinfo", false);
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
    {
      channel = preferences.getInt("channel", 0);
      Serial.printf("Canale wifi da eeprom: %u\n", channel);
    }
    else
    {
      // Ottieni il canale del router e configuralo per ESP-NOW
      channel = getWiFiChannel();
      preferences.putInt("channel", channel);
      
    }

    preferences.end();
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    // Inizializza ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Errore inizializzazione ESP-NOW");
        return;
    }

    // Callback invio
    esp_now_register_send_cb(OnDataSent);

    // Configura peer
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = channel;
    peerInfo.encrypt = false;

    // Aggiungi peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Errore aggiunta peer");
        return;
    }

    // Configura sensori
    dht.begin();
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

}

void loop() {
    // Leggi i sensori
    myData.temp = dht.readTemperature(); // Temperatura
    myData.hum  = dht.readHumidity();    // Umidità
    myData.ter  = analogRead(pinter);    // Sensore terreno
    myData.lux  = analogRead(pinlux);    // Sensore luce

    // Invia i dati
    esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
}