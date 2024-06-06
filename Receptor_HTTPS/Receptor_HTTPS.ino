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
const char* pass = "xxxxxx";
const char* server = "hispatalent.eu";
IPAddress ip(192, 168, 87, 25);
IPAddress gateway(192, 168, 87, 1);
IPAddress subnet(255, 255, 255, 0);

// Declare serverIP as a global variable
IPAddress serverIP;

// Variables NTP para configurar y sincronizar la hora 
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

// Tiempos de medida de pantalla, envío y refresco de pantalla.
unsigned long display;
unsigned long database_send_interval = 20000;
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

// Función para recibir e interpretar los datos de LoRa, descifrarlos y almacenarlos en un Json.
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
    Serial.print("Temperatura recibida: ");
    Serial.println(temp);
    Serial.print("Humedad recibida: ");
    Serial.println(hum);
    Serial.print("Humeda del suelo recibida: ");
    Serial.println(waterLevel);

    updateOLED();
}

// Definir el intervalo de tiempo para cambiar entre datos
unsigned long display_interval = 5000; // 5 segundos por pantalla
unsigned long last_display_time = 0;
int display_state = 0;

// Función para mostrar los datos en la pantalla OLED.
void updateOLED() {
    oled_1.clearDisplay();
    // Encabezado centrado
    oled_1.setTextSize(1);
    oled_1.setTextColor(WHITE);
    oled_1.setCursor(15, 0);
    oled_1.print("SENSOR AMBIENTAL");

    // Línea de separación
    oled_1.drawLine(0, 10, 128, 10, WHITE);

    oled_1.setTextSize(2);

    switch (display_state) {
        case 0: // Mostrar temperatura
            oled_1.setCursor(25, 20);
            oled_1.print("TIEMPO:");
            oled_1.setCursor(30, 40);
            oled_1.print(temp);
            oled_1.print(" C");
            break;
        case 1: // Mostrar humedad
            oled_1.setCursor(20, 20);
            oled_1.print("HUMEDAD:");
            oled_1.setCursor(30, 40);
            oled_1.print(hum);
            oled_1.print(" %");
            break;
        case 2: // Mostrar nivel de agua
            oled_1.setCursor(20, 20);
            oled_1.print("TIERRA:");
            oled_1.setCursor(40, 40);
            oled_1.print(waterLevel);
            oled_1.print(" %");
            break;
    }
    oled_1.display(); 
}

// Función para resolver el dominio de Hispatalent con la IP 192.168.87.35
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

// Inicialización de LoRa, Monitor Serie, WiFi, Sincronización horaria y Mapeo de Hispatalent 
void setup() {
    // Configuración LoRa
    LoRa.setPins(5, 13, 12);
    if (!LoRa.begin(868E6)) {
        Serial.println("Conexión con LoRa fallida!");
        while (1);
    }

    // Configuración OLED
    oled_1.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    updateOLED();

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

    Serial.println("\nConectando con el servidor...");
    // Intenta la conexión sin verificar el certificado
    sslClient.setInsecure();
    serverIP = resolveHostname(server);
    if (serverIP == IPAddress(0, 0, 0, 0)) {
        Serial.println("Error al resolver el dominio");
        return;
    }
    Serial.print("IP resuelta del servidor: ");
    Serial.println(serverIP);

    if (sslClient.connect(serverIP, 443)) {
        Serial.println("Conectado al servidor");
    } else {
        Serial.println("Conexión rechazada");
    }
}

void loop() {
    yield();
    // Recepción de los paquetes recibidos por LoRa.
    fnc_lora_onreceive(LoRa.parsePacket());

    unsigned long current_time2 = millis();
    if (current_time2 - last_display_time > display_interval) {
        last_display_time = current_time2;
        display_state = (display_state + 1) % 3;
        updateOLED();
    }

    unsigned long current_time = millis();
    if (current_time - last_database_send_time >= database_send_interval) {
        last_database_send_time = current_time;

        // Enviar datos a través de WiFi
        envioweb(temp, hum, waterLevel);
    }
}

void envioweb(double t1, double h1, int w1) { // Función para el envío por HTTP a la BBDD
    // Se conecta al servidor después de haberlo resuelto por el puerto 443.
    if (sslClient.connect(serverIP, 443)) {
        Serial.println("Conexión a la base de datos.");

        // Se crea un string con la URL y los datos a enviar cifrados en un Json.
        String url = "/database2.php";
        String contentType = "application/x-www-form-urlencoded";
        String encryptedTemp = encryptString(String(t1), lora_data_cipher_pw);
        String encryptedHum = encryptString(String(h1), lora_data_cipher_pw);
        String encryptedWaterLevel = encryptString(String(w1), lora_data_cipher_pw);

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
        // Confirmación de la recepción de las cabeceras.
        while (sslClient.connected()) {
            String line = sslClient.readStringUntil('\n');
            if (line == "\r") {
                Serial.println("Cabeceras recibidas.");
                break;
            }
        }
        // Respuesta del servidor para depurar fallos
        String response = sslClient.readString();
        Serial.println("Respuesta: " + response);
        // Cierre de la conexión por SSL
        sslClient.stop();
    } else {
        Serial.println("Error al conectar con la base de datos del servidor");
    }
}
