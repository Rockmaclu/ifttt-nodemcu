#include <ESP8266WiFi.h>
#include <MQTT.h>

#define BUFFER_SIZE 512
#define MEASURE_INTERVAL 2

// Definimos parámetros WIFI
#define WIFI_SSID "agu"
#define WIFI_PASWORD "11223344"

// Definimos los parámetros para el IFTTT
const String IFTTT_SERVER = "maker.ifttt.com";
const String IFTTT_EVENT = "boton_presionado";
const String IFTTT_KEY = "*******";
const int IFTTT_PORT = 443;
const char IFTTT_FINGERPRINT[] PROGMEM = "AA 75 CB 41 2E D5 F9 97 FF 5D A0 8B 7D AC 12 21 08 4B 00 8C";

// Definimos los parametros para MQTT
const char *MQTT_SERVER = "m16.cloudmqtt.com";
const int MQTT_PORT = 14872;
const char *MQTT_USER = "heclmxnh";
const char *MQTT_PASSWORD = "**********";
const int MQTT_RECONNECT_TIME = 5;
const int MQTT_QOS = 1;
const bool MQTT_RETAIN = false;
const char *INTENSITY_TOPIC = "intensity";

WiFiClientSecure clientIFTTT;
WiFiClient clientMQTT;

MQTTClient mqttClient(BUFFER_SIZE);


// Definimos los pines que usaremos
const int pinBoton = 5;
const int pinLed = 4;

// Variables globales
float currentMillis,lastMillis;
int presionado;
bool cambios = true;

void onIntensityMessage(char payload[]){
  int c = atoi(payload);
  Serial.println("Cambiando intensidad led");
  cambiar_intensidad_led(c);
}

void onMessage(MQTTClient *client, char topic[], char payload[], int payload_length)
{
  Serial.println(topic);
  String topicString = String(topic);

  if (topicString == INTENSITY_TOPIC)
    onIntensityMessage(payload);
}

void setupMqtt()
{
  mqttClient.begin(MQTT_SERVER, MQTT_PORT, clientMQTT);
  mqttClient.onMessageAdvanced(onMessage);
  mqttClient.setOptions(10, true, 1000);

  if (!mqttClient.connect("1", MQTT_USER, MQTT_PASSWORD, false))
  {
    Serial.println("Fallo MQTT");
    return;
  }
  Serial.println("Conectado a MQTT");

  mqttClient.subscribe(INTENSITY_TOPIC, MQTT_QOS);

}

void setup() {
  pinMode(pinBoton, INPUT);
  pinMode(pinLed, OUTPUT);

  Serial.begin(115200);
  delay(10);

  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID, WIFI_PASWORD);

  Serial.println();
  Serial.println();
  Serial.print("Esperando a conectar a la WiFi... ");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectada");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());

  clientIFTTT.setFingerprint(IFTTT_FINGERPRINT);
  setupMqtt();
}


void loop() {

  yield();
  delay(10);

  currentMillis = millis();

  if ((currentMillis - lastMillis) >= MEASURE_INTERVAL * 1000) {

    presionado = digitalRead(pinBoton);

    if (presionado)
    {
      Serial.println("Enviando trigger");
      enviar_trigger();
    }
    lastMillis = currentMillis;
  }

  mqttClient.loop();


}

void cambiar_intensidad_led(int intensidad){
      analogWrite(pinLed,intensidad);
}

void enviar_trigger()
{

    if (!clientIFTTT.connect(IFTTT_SERVER, IFTTT_PORT)) {
      Serial.println("Cliente no conectado IFTTT");
      return;
    }
    Serial.println("Conectado IFTTT");

    clientIFTTT.println("POST /trigger/" + IFTTT_EVENT + "/with/key/"+ IFTTT_KEY + " HTTP/1.1");
    clientIFTTT.println("Host: "+ IFTTT_SERVER);
    clientIFTTT.println("Connection: close");
    clientIFTTT.println();
    delay(50);

    while (clientIFTTT.connected()) {
        String line = clientIFTTT.readStringUntil('\r');
        Serial.print(line);
    }

  // Esperamos hasta que se hayan enviado todos los datos
  clientIFTTT.flush();
  // Desconectamos del cliente
  clientIFTTT.stop();


  Serial.println("Trigger enviado");
}
