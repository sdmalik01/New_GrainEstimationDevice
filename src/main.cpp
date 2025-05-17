#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Wi-Fi credentials
#define WIFI_SSID "Malik"
#define WIFI_PASSWORD "Malik@2005"

// Firestore HTTP credentials
const String uid = "c9BTpB2ShgrftbCjG2SU";  // Your user UID
const String FIREBASE_HOST = "https://firestore.googleapis.com/v1/projects/malikee-dd96b/databases/(default)/documents";
const String FIREBASE_API_KEY = "AIzaSyBPoVlzc5CVAN4IoRH8eJNT0rIPkbhp6SM";
const String DOCUMENT_PATH = "/SensorData/" + uid;

// DHT11 setup
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Ultrasonic setup
#define TRIG_PIN 13
#define ECHO_PIN 12
const float JAR_HEIGHT_CM = 20.0;
const float ULTRASONIC_OFFSET = 0.5; // optional offset for calibration
#define SPEED_OF_SOUND 0.0343

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 sec

void setup() {
  Serial.begin(115200);

  // LCD I2C init
  Wire.begin(18, 19);
  lcd.begin(16, 2);
  lcd.backlight();
  dht.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // WiFi Connect
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Ultrasonic reading
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  float duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * SPEED_OF_SOUND) / 2.0;
  distance -= ULTRASONIC_OFFSET;
  distance = constrain(distance, 0, JAR_HEIGHT_CM);

  float fillPercent = (((JAR_HEIGHT_CM - distance) / JAR_HEIGHT_CM) * 100.0) + 40;
  fillPercent = constrain(fillPercent, 0, 100);

  // LCD display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("T:%.1fC H:%.1f%%", temp, hum);
  lcd.setCursor(0, 1);
  lcd.printf("Fill: %.1f%%", fillPercent);

  // Serial debug
  Serial.printf("Temp: %.1f°C, Hum: %.1f%%, Fill: %.1f%%\n", temp, hum, fillPercent);

  // Firestore upload every 10 sec
  if (millis() - lastSendTime > sendInterval && WiFi.status() == WL_CONNECTED) {
    lastSendTime = millis();

    HTTPClient http;
    String url = FIREBASE_HOST + DOCUMENT_PATH + "?key=" + FIREBASE_API_KEY;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{"
      "\"fields\": {"
        "\"temperature\": {\"doubleValue\": " + String(temp, 2) + "},"
        "\"humidity\": {\"doubleValue\": " + String(hum, 2) + "},"
        "\"fill_percentage\": {\"doubleValue\": " + String(fillPercent, 1) + "}"
      "}"
    "}";

    int httpCode = http.PATCH(jsonPayload);
    Serial.println("HTTP Response: " + String(httpCode));
    Serial.println(http.getString());
    http.end();
  }
}
