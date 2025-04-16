#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <DHT.h>

// Wi-Fi và MQTT thông số
const char *ssid = "nghuuhuongg laptop";
const char *password = "333333333";
const char *mqtt_server = "192.168.137.97"; // Địa chỉ của host
const int mqtt_port = 1883;
const char *mqtt_id = "esp32_node1";
const char *topic1_publish = "esp32_node1/t";
const char *topic2_publish = "esp32_node1/h";
const char *topic3_publish = "esp32_node1/c";

// Cảm biến DHT
#define DHTPIN 16
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

WiFiClient client;
PubSubClient mqtt_client(client);

// Kết nối lại MQTT nếu mất kết nối
void reconnectMQTT() {
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt_client.connect(mqtt_id)) {
      Serial.println("connected to MQTT broker.");
    } else {
      Serial.print("Failed, rc=");
      Serial.println(mqtt_client.state());
      delay(5000);
    }
  }
}

// Chân tín hiệu LED
#define LED_PIN 17

void setup() {
  dht.begin();
  Serial.begin(9600);

  // Cấu hình LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

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
  reconnectMQTT();
}

void loop() {
  if (!mqtt_client.connected()) {
    reconnectMQTT();
  }
  mqtt_client.loop();

  // Đọc dữ liệu từ cảm biến DHT
  float temperature2 = dht.readTemperature();
  float humidity2 = dht.readHumidity();

  if (isnan(temperature2) || isnan(humidity2)) {
    Serial.println("Failed to read data from DHT sensor!");
    return;
  }

  // Điều khiển LED và xác định trạng thái báo động
  String baodong2;
  if (temperature2 > 27 || humidity2 > 50) {
    digitalWrite(LED_PIN, HIGH);
    baodong2 = "BAO DONG";
  } else {
    digitalWrite(LED_PIN, LOW);
    baodong2 = "ON DINH";
  }

  // Gửi dữ liệu qua MQTT
  bool success1 = mqtt_client.publish(topic1_publish, String(temperature2).c_str());
  bool success2 = mqtt_client.publish(topic2_publish, String(humidity2).c_str());
  bool success3 = mqtt_client.publish(topic3_publish, baodong2.c_str());

  if (success1 && success2 && success3) {
    Serial.println("Data sent successfully:");
    Serial.println("Temperature: " + String(temperature2));
    Serial.println("Humidity: " + String(humidity2));
    Serial.println("Alert: " + baodong2);
  } else {
    Serial.println("Failed to send one or more data packets!");
  }

  delay(5000); // Gửi dữ liệu mỗi 3 giây
}


