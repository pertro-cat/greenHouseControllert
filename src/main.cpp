#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DHT.h>

#define SENSOR_PIN 12
#define POWER_PIN 13
#define HALL_PIN 14
#define PH_PIN A0

#define DELAY_READ 2000
#define STABILIZE_TIME 100

#define DHT_PIN 5
DHT dht;

const char *ssid = "admin";
const char *password = "domestos1216";

IPAddress serverIP(192, 168, 31, 222); // Замінити на IP сервера
const uint16_t serverPort = 5000;
WiFiClient client;

float temperature = 0;
float humidity = 0;
float lightAnalog = 0;
float lightLevel = 0;
enum SensorState
{
  IDLE,
  POWER_ON,
  READ_SENSOR
};

SensorState state = IDLE;
unsigned long lastCycleTime = 0;
unsigned long powerOnTime = 0;
bool isSoilDry = false;
bool isDoorOpen = false;

void checkSensor();
void sendData();

void setup()
{
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT);
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW);
  pinMode(HALL_PIN, INPUT);

  dht.setup(DHT_PIN, DHT::DHT22);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");

  while (!client.connect(serverIP, serverPort))
  {
    Serial.println("Trying to connect to server...");
    delay(2000);
  }

  Serial.println("Connected to server!");
}

void loop()
{
  checkSensor();
}

void checkSensor()
{
  unsigned long now = millis();

  switch (state)
  {
  case IDLE:
    if (now - lastCycleTime >= DELAY_READ)
    {
      digitalWrite(POWER_PIN, HIGH);
      powerOnTime = now;
      state = POWER_ON;
    }
    break;

  case POWER_ON:
    if (now - powerOnTime >= STABILIZE_TIME)
    {
      state = READ_SENSOR;
    }
    break;

  case READ_SENSOR:
    uint8_t stateValue = digitalRead(SENSOR_PIN);
    uint8_t hallState = digitalRead(HALL_PIN);
    lightAnalog = analogRead(PH_PIN);
    lightLevel = lightAnalog * 100 / 1023;

    temperature = dht.getTemperature();
    humidity = dht.getHumidity();

    Serial.print("Digital state: ");
    Serial.println(stateValue);

    Serial.print("Hall state: ");
    Serial.println(hallState);

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Analog value of light: ");
    Serial.println(lightAnalog);
    Serial.print("Light level: ");
    Serial.print(lightLevel);
    Serial.println(" %");

    isSoilDry = (stateValue == HIGH);
    Serial.println(isSoilDry ? "Soil is dry" : "Soil is wet"); // сухий вологий

    isDoorOpen = (hallState == LOW);
    Serial.println(hallState ? "Door open" : "Door closed");

    sendData();

    digitalWrite(POWER_PIN, LOW);
    lastCycleTime = now;
    state = IDLE;
    break;
  }
}

void sendData()
{
  if (!client.connected())
  {
    Serial.println("Lost connection. Reconnecting to server...");
    if (!client.connect(serverIP, serverPort))
    {
      Serial.println("Reconnection failed.");
      return;
    }
    Serial.println("Reconnected to server.");
  }

  // Створюємо повідомлення для сервера
  String message = "{";
  message += "\"soil\": \"" + String(isSoilDry ? "dry" : "wet") + "\", ";
  message += "\"door\": \"" + String(isDoorOpen ? "open" : "closed") + "\", ";
  message += "\"temp_c\": " + String(temperature, 1) + ", ";
  message += "\"humidity\": " + String(humidity, 1);
  message += "\"light_precent\": " + String(lightLevel) + " %";
  message += "}";

  // Відправка
  client.println(message);

  Serial.print("Sent to server: ");
  Serial.println(message);
}
