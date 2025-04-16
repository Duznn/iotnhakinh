#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// Wi-Fi và MQTT thông số
const char *ssid = "Eri Eri";
const char *password = "123@5678";
const char *mqtt_server = "192.168.1.143"; // Địa chỉ của host
const int mqtt_port = 1883;
const char *mqtt_id = "esp32_host";
const char *topic1_subscribe = "esp32_node1/t"; // Topic nhiệt độ từ Node
const char *topic2_subscribe = "esp32_node1/h"; // Topic độ ẩm từ Node
const char *topic3_subscribe = "esp32_node1/c";

String baodong1 = "ON DINH";

// DHT trên Host
#define LED_PIN 17
#define DHTPIN1 16 // Chân GPIO kết nối cảm biến DHT trên Host
#define DHTTYPE1 DHT22
DHT dht1(DHTPIN1, DHTTYPE1);

// Dữ liệu toàn cục
float temperature1 = 0.0;
float humidity1 = 0.0;
String temperature2 = "--";
String humidity2 = "--";
String baodong2 = "--";

WiFiClient client;
PubSubClient mqtt_client(client);
AsyncWebServer server(80);

unsigned long previousMillis = 0;
const long interval = 5000; // Khoảng thời gian 5 giây

// Callback xử lý dữ liệu MQTT từ Node
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String data = "";
  for (unsigned int i = 0; i < length; i++) {
    data += (char)payload[i];
  }
  Serial.print("Received data from topic: ");
  Serial.println(topic);

  if (String(topic) == topic1_subscribe) {
    temperature2 = data;
    Serial.println("Node Temperature updated: " + temperature2);
  } else if (String(topic) == topic2_subscribe) {
    humidity2 = data;
    Serial.println("Node Humidity updated: " + humidity2);
  } else if (String(topic) == topic3_subscribe) {
    baodong2 = data;
    Serial.println("Received alert from Node: " + baodong2);
  } else {
    Serial.println("Unknown topic, ignoring data.");
  }
}

// Kết nối lại MQTT nếu mất kết nối
void reconnectMQTT() {
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt_client.connect(mqtt_id, nullptr, nullptr, nullptr, 0, false, nullptr, 60)) {
      // Thời gian giữ kết nối (keep-alive) là 60 giây
      Serial.println("connected.");
      mqtt_client.subscribe(topic1_subscribe);
      mqtt_client.subscribe(topic2_subscribe);
      mqtt_client.subscribe(topic3_subscribe);
    } else {
      Serial.print("Failed, rc=");
      Serial.println(mqtt_client.state());
      delay(5000);
    }
  }
}

// Hàm đọc cảm biến DHT trên Host
void readDHTHost() {
  float t1 = dht1.readTemperature();
  float h1 = dht1.readHumidity();

  if (!isnan(t1) && !isnan(h1)) {
    temperature1 = t1;
    humidity1 = h1;
    Serial.print("Host DHT - Temperature: ");
    Serial.print(temperature1);
    Serial.println(" °C");
    Serial.print("Host DHT - Humidity: ");
    Serial.print(humidity1);
    Serial.println(" %");

    // Cập nhật báo động dựa trên cảm biến của Host
    if (temperature1 > 30 || humidity1 > 60) {
      baodong1 = "BAO DONG";
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Host Alert: BAO DONG");
    } else {
      baodong1 = "ON DINH";
      digitalWrite(LED_PIN, LOW);
      Serial.println("Host Alert: ON DINH");
    }
  } else {
    Serial.println("Failed to read from DHT sensor on Host!");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Khởi động DHT trên Host
  dht1.begin();

  // Kết nối Wi-Fi
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Kết nối MQTT
  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  reconnectMQTT();

  // Khởi tạo SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("SPIFFS Mounted Successfully");

  // Thiết lập Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/temperature1", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(temperature1).c_str());
  });

  server.on("/humidity1", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", String(humidity1).c_str());
  });
  server.on("/tlu.jpg", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/tlu.jpg", "image/jpeg");
});


  server.on("/baodong1", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", baodong1.c_str());
  });

  server.on("/temperature2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", temperature2.c_str());
  });

  server.on("/humidity2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", humidity2.c_str());
  });

  server.on("/baodong2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", baodong2.c_str());
  });

  server.begin();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected, reconnecting...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWi-Fi reconnected.");
  }

  if (!mqtt_client.connected()) {
    reconnectMQTT();
  }
  mqtt_client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readDHTHost();
  }
}


