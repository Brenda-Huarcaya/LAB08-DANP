/*
 * Desarrollado por Brenda Alisson Huarcaya Zapana
 * Projecto: Laboratorio 08
 * Curso: DANP
 * Coneccion: AWS Iot Core
 * Placa: NodeMCU ESP8266
*/

#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <RTClib.h>
#include <ESP8266HTTPClient.h>

//DATOS PARA LA CONECCION CON WIFI
const char * ssid = "Pochi";
const char * password = "#en@nos2022#";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//VARIABLES PLACA
const int sensorLDR = A0;
const int led = D1;
int threshold = 500;
int ledPotencia = 0;
int httpCode = 0;

//AWS_endpoint, MQTT broker ip
const char * AWS_endpoint = "a3brjmyw4c304l-ats.iot.us-east-2.amazonaws.com";

//RECEPCION DEL MENSAJE
void callback(char * topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }

  int valor = atoi((char*) payload);
  Serial.println(valor);

  //SE MANEJA EL LED A PARTIR DEL DATO RECIBIDO
  /*
  if (valor >= 0 && valor <= 100) {
    ledPotencia = valor;
    if(valor > 0 && valor < 30){
      analogWrite(led, ledPotencia);
      }
    else if(valor >= 30 && valor < 70){
      analogWrite(led, ledPotencia);    
      } 
    else {
      analogWrite(led, ledPotencia);
      }
      Serial.println(valor);
  }
  else{
    analogWrite(led, ledPotencia);
    }
    */
}

//CONECCION AL WIFI
WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient);
HTTPClient http;

long lastMsg = 0;
char msg[50];
int value = 0;
RTC_DS3231 rtc;

void setup_wifi() {
  delay(10);
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  espClient.setX509Time(timeClient.getEpochTime());
}

//RECONECCION SI ES NECESARIA
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESPthing")) {
      Serial.println("connected");
      //client.publish("mobile/mensajes",);
      client.subscribe("sensor/command");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      char buf[256];
      espClient.getLastSSLError(buf, 256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);
      delay(5000);
    }
  }
}


void setup(){
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  
  //INICIALIZACION DE SENSOR Y LED
  pinMode(sensorLDR, INPUT);
  pinMode(led, OUTPUT);
  
  setup_wifi();
  delay(1000);

  if (!rtc.begin()) {
    Serial.println("Error al iniciar el RTC");
  }
  
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
  
  //SUBIMOS LOS CERTIFICADOS
  File cert = SPIFFS.open("/cert2.der", "r");
  if (!cert) {
    Serial.println("Failed to open cert file");
  } else
    Serial.println("Success to open cert file");
  delay(1000);
  if (espClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");
    
  //SUBIMOS LLAVE PRIVADA
  File private_key = SPIFFS.open("/private2.der", "r"); 
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  } else
    Serial.println("Success to open private cert file");
  delay(1000);
  if (espClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");
    
  //SUBIMOS CA
  File ca_dos = SPIFFS.open("/ca2.der", "r"); 
  if (!ca_dos) {
    Serial.println("Failed to open ca2 ");
  } else
    Serial.println("Success to open ca2");
  delay(1000);
  if (espClient.loadCACert(ca_dos))
    Serial.println("ca2 loaded");
  else
    Serial.println("ca2 failed");

    //ESTE CA NO FUNCIONA, NO LLEGA A HACERSE LA CONEXION
/*
  //SUBIMOS EL OTRO CA
  File ca_tres = SPIFFS.open("/ca3.der", "r"); 
  if (!ca_tres) {
    Serial.println("Failed to open ca3 ");
  } else
    Serial.println("Success to open ca3");
  delay(1000);
  if (espClient.loadCACert(ca_tres))
    Serial.println("ca3 loaded");
  else
    Serial.println("ca3 failed");
*/
  Serial.print("Heap: ");
  Serial.println(ESP.getFreeHeap());
}

//METODO PARA RECEPCION DEL MENSAJE POR MEDIO DE URL LAMBDA
void getHttp(int value) {
  //INSTANCIA HTTPClient
  HTTPClient http;
  Serial.println("Valor a enviar");
  Serial.println(value);
  
  //URL VALOR DEL MENSAJE
  char url[100];
  sprintf(url, "https://fkcmxu52vb7naznafl2ciivoei0ipuea.lambda-url.us-east-2.on.aws/command?value=%d", value);

  //SOLICITUD GET A LA URL
  http.begin(url);
  httpCode = http.GET();
  Serial.println("Código HTTP recibido: " + String(httpCode));
  
  //VERIFICACION DE SOLICITUD EXITOSA
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Mensaje enviado a través de la URL: " + payload);
    Serial.println("Valor a enviado");
    Serial.println(payload);
  } else {
    Serial.println("Error al enviar el mensaje a través de la URL. Código HTTP: " + httpCode);
  }
  
  //CIERRA LA CONEXION
  http.end();
  
  }
  
//METODO DE ENVIO DE LOS DATOS
void sendToCloud(DateTime timestamp, int value, const char* unit, const char* notes) {

  //OBJETO JSON
  StaticJsonDocument<200> doc;
  
  //FECHA Y HORA FORMATO LEGIBLE "DD/MM/AA HH:mm:ss"
  char formattedTime[20];
  sprintf(formattedTime, "%02d/%02d/%02d %02d:%02d:%02d",
          timestamp.day(), timestamp.month(), timestamp.year() % 100,
          timestamp.hour(), timestamp.minute(), timestamp.second());
          
  //AGREGAMOS DATOS AL JSON
  doc["Timestamp"] = formattedTime;
  doc["Value"] = value;
  doc["Unit"] = unit;
  doc["Notes"] = notes;

  //DE OBJETO A CADENA JSON
  char mensaje[200];
  serializeJson(doc, mensaje);
  
  //TOPIC
  if (client.publish("mobile/mensajes", mensaje)) {
    Serial.println("Mensaje enviado al cloud.");
    Serial.println(mensaje);
  } else {
    Serial.println("Error al enviar el mensaje al cloud.");
  }
}

//VARIABLES PARA EL ENVIO DE MENSAJES CADA 30 SEGUNDOS
unsigned long before = 0;
unsigned long segundosTreinta = 30000;

void loop(){
  if (!client.connected()) {
    reconnect();
  }
  
  //LECTURA DL SENSOR
  int rawData = analogRead(sensorLDR);
  client.loop();

  //EL FOCO PRENDE DEPENDIENDO DEL VALOR DEL SENSOR
  Serial.println(rawData);
    if(rawData > threshold){
      analogWrite(led, 0);
      }
    else if(rawData >= 300 && rawData < threshold){
      analogWrite(led, 25);    
      } 
    else {
      analogWrite(led, 100);
      }
     
  delay(1000);

  //COMUNICACION PERIODICA
   if (millis() - before >= segundosTreinta) {
    before = millis();

    //PARA ENVIO DE MENSAJE
      //OBTIENE TIMESTAMP ACTUAL
      long timestamp = timeClient.getEpochTime();
      //SE PUBLICA EL MENSAJE
      sendToCloud(timestamp, rawData, "Lux(unidad de iluminancia)", "TEST");
     
    //CODIGO PARA LA PETICION GET
      //getHttp(ledPotencia);
      //MIENTRAS NO HAYA CONEXION NO SIGUE ENVIANDO VALORES
      /*while (httpCode != HTTP_CODE_OK) {
        httpCode = http.GET();
        delay(100);
      }*/
      //ENVIA VALORES DE 10 EN 10 PARA VER EL CAMBIO EN EL LED
      /*ledPotencia += 10;
      if (ledPotencia > 100) {
         ledPotencia = 0;
       }*/
   }
 
  //RECEPCION DEL MENSAJE
  //char recivedMsg = client.subscribe("sensor/command",1);
  //Serial.println(valor);
  delay(1000);
}
