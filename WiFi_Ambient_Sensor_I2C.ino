/*################################# An example to connect thingcontro.io MQTT over TLS 1.2 ###############################
 * Using thingcontrol board V 1.0 read ambient temperature and humidity values from an FS200-SHT 1X sensor via I2C send to 
 * thingcontrol.io by MQTT over TLS 1.2 via WiFi (WiFi Manager)
 *########################################################################################################################*/
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

#include <Arduino.h>
#include <SHT1x-ESP.h>

#define WIFI_AP ""
#define WIFI_PASSWORD ""

String deviceToken = "xxxxxxxxxxxxxxxxxxxx";
char thingsboardServer[] = "mqtt.thingcontrol.io";

String json = "";

unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long startTeleMillis;
unsigned long starSendTeletMillis;
unsigned long currentMillis;
const unsigned long periodSendTelemetry = 10000;  //the value is a number of milliseconds

WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;
int PORT = 8883;

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

struct Soil
{
  String temp;
  String hum;
};

// Specify data and clock connections and instantiate SHT1x object
#define dataPin  21
#define clockPin 22

// if 5v board is used
//SHT1x sht1x(dataPin, clockPin);

// if 3.3v board is used
SHT1x sht1x(dataPin, clockPin, SHT1x::Voltage::DC_3_3v);

Soil sensor[3] ;

void setup() {
  Serial.begin(9600); // Open serial connection to report values to host
  Serial.println(F("Starting... Ambient Temperature/Humidity Monitor"));
  Serial.println();
  Serial.println(F("***********************************"));

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect("@Thingcontrol_AP")) {
    Serial.println("failed to connect and hit timeout");
    delay(1000);
  }
  client.setServer( thingsboardServer, PORT );
  reconnectMqtt();

  Serial.print("Start.....");
  startMillis = millis();  //initial start time
}

void loop()
{
  status = WiFi.status();
  if ( status == WL_CONNECTED)
  {
    if ( !client.connected() )
    {
      reconnectMqtt();
    }
    client.loop();
  }

  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  //send 
  if (currentMillis - starSendTeletMillis >= periodSendTelemetry)  //test whether the period has elapsed
  {
    read_Sensor() ;
    delay(2000);
    sendtelemetry();
    starSendTeletMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }
}

void setWiFi()
{
  Serial.println("OK");
  Serial.println("+:Reconfig WiFi  Restart...");
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  if (!wifiManager.startConfigPortal("ThingControlCommand"))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    delay(5000);
  }
  Serial.println("OK");
  Serial.println("+:WiFi Connect");
  client.setServer( thingsboardServer, PORT );  // secure port over TLS 1.2
  reconnectMqtt();
}

void processTele(char jsonTele[])
{
  char *aString = jsonTele;
  Serial.println("OK");
  Serial.print(F("+:topic v1/devices/me/ , "));
  Serial.println(aString);
  client.publish( "v1/devices/me/telemetry", aString);
}

void reconnectMqtt()
{
  if ( client.connect("Thingcontrol_AT", deviceToken.c_str(), NULL) )
  {
    Serial.println( F("Connect MQTT Success."));
    client.subscribe("v1/devices/me/rpc/request/+");
  }
}

void read_Sensor()
{
  float temp_c;
  float temp_f;
  float humidity;  
  char buf[20];

  // Read values from the sensor
  temp_c = sht1x.readTemperatureC();
//  temp_f = sht1x.readTemperatureF();
  humidity = sht1x.readHumidity();

  // Print the values to the serial port
  Serial.print("Temperature: ");
  Serial.print(temp_c,2);
  Serial.print(" C");
//  Serial.print(temp_f, DEC);
  Serial.print(" Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  dtostrf(temp_c , 2, 2, buf);
  sensor[0].temp = buf;
  dtostrf(humidity , 2, 2, buf);
  sensor[0].hum =  buf;
  delay(2000);
//  Serial.println("");
//  Serial.println("----------------------");
}

void sendtelemetry()
{
  String json = "";
  json.concat("{\"temp\":");
  json.concat(sensor[0].temp);
  json.concat(",\"hum\":");
  json.concat(sensor[0].hum);
  json.concat("}");
  Serial.println(json);
  // Length (with one extra character for the null terminator)
  int str_len = json.length() + 1;
  // Prepare the character array (the buffer)
  char char_array[str_len];
  // Copy it over
  json.toCharArray(char_array, str_len);
  processTele(char_array);
}
