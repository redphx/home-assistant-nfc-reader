#include <M5Atom.h>
#include <WiFi.h>
#include <MQTTClient.h>
#include <MQTT.h>
#include <driver/i2s.h>
#include "MFRC522_I2C.h"

// Wifi SSID & Password
const char *WIFI_SSID = "ssid";
const char *WIFI_PASSWORD = "password";

// MQTT Server
const char *MQTT_SERVER = "192.168.1.100";
const char *MQTT_USER = "user";
const char *MQTT_PASSWORD = "password";
const char *MQTT_DISCOVERY_TOPIC = "homeassistant/tag/hamiibo/config";
const char *MQTT_TAG_SCANNED_TOPIC = "hamiibo/tag_scanned";

// Home Assistant Device
const char *HA_DEVICE_NAME = "HAmiibo";
const char *HA_DEVICE_ID = "hamiibo";

extern const unsigned char scanned_sound[];
extern const unsigned int scanned_sound_len;
size_t sound_bytes_written;

MFRC522 mfrc522(0x28);  // Create MFRC522 instance

#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

#define SPEAK_I2S_NUMBER I2S_NUM_0
#define SAMPLE_RATE 44100

WiFiClient net;
MQTTClient mqtt_client;

bool init_i2s_speaker() {
  esp_err_t err = ESP_OK;

  i2s_driver_uninstall(SPEAK_I2S_NUMBER);
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 2,
      .dma_buf_len = 128,
  };

  i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  i2s_config.use_apll = false;
  i2s_config.tx_desc_auto_clear = true;

  Serial.println("Init i2s_driver_install");
  err += i2s_driver_install(SPEAK_I2S_NUMBER, &i2s_config, 0, NULL);
  i2s_pin_config_t tx_pin_config;

  tx_pin_config.mck_io_num = GPIO_NUM_0;
  tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
  tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
  tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
  tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;

  Serial.println("Init i2s_set_pin");
  err += i2s_set_pin(SPEAK_I2S_NUMBER, &tx_pin_config);
  Serial.println("Init i2s_set_clk");
  err += i2s_set_clk(SPEAK_I2S_NUMBER, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

  return true;
}


void register_device() {
  String message = "{\"topic\": \"" + String(MQTT_TAG_SCANNED_TOPIC) + "\", \"device\": {\"name\": \"" + String(HA_DEVICE_NAME) + "\", \"identifiers\": \"" + String(HA_DEVICE_ID) + "\"}}";
  // Serial.println(message);
  mqtt_client.publish(MQTT_DISCOVERY_TOPIC, message, true, 2);
}


void connect() {
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Connected");

  Serial.println("Connecting to MQTT");
  while (!mqtt_client.connect("rfid", MQTT_USER, MQTT_PASSWORD)) {
    Serial.print(".");
    delay(1000);
  }

  // Publish HA Device
  register_device();
  Serial.println("Connected");

  // Play boot sound
  i2s_write(SPEAK_I2S_NUMBER, scanned_sound, scanned_sound_len, &sound_bytes_written, portMAX_DELAY);
}

void setup() {
  M5.begin(true, false, true);

  Serial.println("Init Speaker");
  init_i2s_speaker();
  delay(100);

  // Init MFRC522
  Serial.println("Init MFRC522");
  Wire.begin(26, 32);
  mfrc522.PCD_Init();

  // Init Wifi
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqtt_client.begin(MQTT_SERVER, net);

  connect();
}

void loop() {
  mqtt_client.loop();
  if (!mqtt_client.connected()) {
    connect();
  }

  // No cards found
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(1000);
    return;
  }

  // Read card ID
  String uid_hex = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uid_hex += "0";
    }
    uid_hex += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println(uid_hex);

  // Send card ID to MQTT server
  mqtt_client.publish(MQTT_TAG_SCANNED_TOPIC, uid_hex);

  // Play scanned sound
  i2s_write(SPEAK_I2S_NUMBER, scanned_sound, scanned_sound_len, &sound_bytes_written, portMAX_DELAY);

  // Sleep for 5 seconds
  delay(5000);
}
