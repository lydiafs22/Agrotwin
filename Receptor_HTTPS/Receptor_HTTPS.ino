#include <SPI.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <map>

// Configuración de red
const char* ssid = "xxx";
const char* pass = "xxxxxxxx";
const char* server = "hispatalent.eu";
IPAddress ip(192, 168, 87, 25);
IPAddress gateway(192, 168, 87, 1);
IPAddress subnet(255, 255, 255, 0);

// Variables NTP  para configurar y sincronizar la hora 
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// Variables para LoRa la recepción de los datos a través de LoRa
double temp;
double hum;
int waterLevel;
String lora_data_received = "";
int lora_data_rssi = 0;
Adafruit_SSD1306 oled_1(128, 64, &Wire, -1);
StaticJsonDocument<1500> json_data;
std::map<String, IPAddress> hostsMap; //Mapeo para resolver el dominio Hispatalent.

//Tiempos de medida de pantalla, envío y refresco de pantalla.
unsigned long display;
unsigned long database_send_interval = 200000;
unsigned long last_display_refresh_time = 0;
unsigned long last_database_send_time = 0;

// Clave de cifrado para el envío de los datos a través de LoRa
String lora_data_cipher_pw = "Password";

// Cliente WiFi y SSL
WiFiMulti WiFiMulti;
WiFiClientSecure sslClient;

// Función de cifrado XOR
String encryptString(String input, String key) {
    String output = "";
    for (int i = 0; i < input.length(); i++) {
        output += char(input[i] ^ key[i % key.length()]); // Operación XOR para cifrar
    }
    return output;
}

// Función de descifrado XOR
String decryptString(String input, String key) {
    String output = "";
    for (int i = 0; i < input.length(); i++) {
        output += char(input[i] ^ key[i % key.length()]); // Operación XOR para descifrar
    }
    return output;
}

//Función para recibir e interpretar los datos de LoRa, descifrarlos y almacenarlos en un Json.
void fnc_lora_onreceive(int packetSize) {
    if (packetSize == 0) return;
    
    lora_data_received = "";
    while (LoRa.available()) {
        lora_data_received += (char)LoRa.read();
    }
    lora_data_rssi = LoRa.packetRssi();

    // Descifrar datos recibidos
    String decrypted_data = decryptString(lora_data_received, lora_data_cipher_pw);

    DeserializationError error = deserializeJson(json_data, decrypted_data);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }

    temp = json_data["T"];
    hum = json_data["H"];
    waterLevel = json_data["W"];

    // Mensajes de depuración
    Serial.print("Received Temperature: ");
    Serial.println(temp);
    Serial.print("Received Humidity: ");
    Serial.println(hum);
    Serial.print("Received Water Level: ");
    Serial.println(waterLevel);

    updateOLED();
}
//Función para mostrar los datos en la pantalla OLED.
void updateOLED() {
    oled_1.clearDisplay();
    oled_1.setTextSize(2);
    oled_1.setTextColor(WHITE);
    oled_1.setCursor(0, 0);
    oled_1.print(String("TEM:"));
    oled_1.display();
    oled_1.setCursor(55, 0);
    oled_1.print(temp);
    oled_1.display();
    oled_1.setCursor(0, 20);
    oled_1.print(String("HUM:"));
    oled_1.display();
    oled_1.setCursor(55, 20);
    oled_1.print(hum);
    oled_1.display();
    oled_1.setCursor(0, 40);
    oled_1.print(String("WTR:"));
    oled_1.display(); 
    oled_1.setCursor(55, 40);
    oled_1.print(waterLevel);
    oled_1.display(); 
}
//Función para resolver el dominio de Hispatalent con la IP 192.168.87.35
IPAddress resolveHostname(String hostname) {
    if (hostsMap.find(hostname) != hostsMap.end()) {
        return hostsMap[hostname];
    } else {
        if (hostname.startsWith("https://")) {
            hostname.remove(0, 8);
        }
        IPAddress resolvedIP;
        if (WiFi.hostByName(hostname.c_str(), resolvedIP)) {
            return resolvedIP;
        } else {
            return IPAddress(0, 0, 0, 0);
        }
    }
}
//Inicialización de LoRa, Monitor Serie, WiFi, Sincronización horaria y Mapeo de Hispatalent 
void setup() {
    // Configuración LoRa
    LoRa.setPins(5, 13, 12);
    if (!LoRa.begin(868E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }

    // Configuración OLED
    oled_1.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    // Configuración Serial
    Serial.begin(115200);
    delay(10);

    // Configuración WiFi
    WiFi.mode(WIFI_STA);
    WiFi.config(ip, gateway, subnet);
    WiFiMulti.addAP(ssid, pass);

    Serial.print("Conectando a:\t");
    Serial.println(ssid);
    while (WiFiMulti.run() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("Conexión WiFi establecida");

    // Añadir mapeo de "hispatalent.eu" a "192.168.87.35"
    hostsMap["hispatalent.eu"] = IPAddress(192, 168, 87, 35);

    // Inicializar NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    Serial.println("\nStarting connection to server...");
    // Intenta la conexión sin verificar el certificado
    sslClient.setInsecure();
    IPAddress serverIP = resolveHostname(server);
    if (sslClient.connect(serverIP, 443)) {
        Serial.println("Conectado al servidor");
        sslClient.println("GET /database2.php HTTP/1.1");
        sslClient.println("Host: hispatalent.eu");
        sslClient.println("Connection: close");
        sslClient.println();
    } else {
        Serial.println("Conexión rechazada");
    }
}

void loop() {
    yield();
    //Recepción de los paquetes recibidos por LoRa.
    fnc_lora_onreceive(LoRa.parsePacket());

    unsigned long current_time = millis();
//Sincronización de pantallas OLED y envío de datos al servidor.
    if (current_time - last_display_refresh_time > database_send_interval) {
        last_display_refresh_time = current_time;
        updateOLED();
    }

    if (current_time - last_database_send_time > database_send_interval) {
        last_database_send_time = current_time;
        //Se resuelve la IP y el dominio
        IPAddress serverIP = resolveHostname(server);
        if (serverIP == IPAddress(0, 0, 0, 0)) {
            Serial.println("Error al resolver el dominio");
            return;
        }
          //Se conecta al servidor después de haberlo resuelto por el puerto 443.
        if (sslClient.connect(serverIP, 443)) {
            Serial.println("Conexión a la base de datos.");

          //Se crea un string con la URL y los datos a envíar cifrados en un Json.
            String url = "/database2.php";
            String contentType = "application/x-www-form-urlencoded";
            String encryptedTemp = encryptString(String(temp), lora_data_cipher_pw);
            String encryptedHum = encryptString(String(hum), lora_data_cipher_pw);
            String encryptedWaterLevel = encryptString(String(waterLevel), lora_data_cipher_pw);

            String postData = "temp=" + encryptedTemp + "&hum=" + encryptedHum + "&water=" + encryptedWaterLevel;

            Serial.println("Sending encrypted data:");
            Serial.println("Encrypted Temp: " + encryptedTemp);
            Serial.println("Encrypted Hum: " + encryptedHum);
            Serial.println("Encrypted Water: " + encryptedWaterLevel);

            sslClient.print(String("POST ") + url + " HTTP/1.1\r\n");
            sslClient.print(String("Host: ") + server + "\r\n");
            sslClient.print(String("Content-Type: ") + contentType + "\r\n");
            sslClient.print(String("Content-Length: ") + postData.length() + "\r\n");
            sslClient.print("Connection: close\r\n\r\n");
            sslClient.print(postData);

            delay(1000); // Espera un momento para asegurar que los datos se envíen correctamente
              //Confirmación de la recepción de las cabeceras.
            while (sslClient.connected()) {
                String line = sslClient.readStringUntil('\n');
                if (line == "\r") {
                    Serial.println("Cabeceras recibidas.");
                    break;
                }
            }
              //Respuesta del servidor para depurar fallos
            String response = sslClient.readString();
            Serial.println("Respuesta: " + response);
          //Cierre de la conexión por SSL
            sslClient.stop();
        } else {
            Serial.println("Error al conectar con la base de datos del servidor");
        }
    }
}

