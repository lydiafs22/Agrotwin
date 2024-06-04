#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ABlocks_DHT.h>

//Pin 35 analógico para el sensor de tierra.
const int sensorPin = 35; 
//Variables que almacenan los datos de Water: Humedad del suelo Temp: Temperatura del DHT11 y Hum: Humedad recogida por el DHT11
int waterLevel; 
double temp; 
double hum; 

String s_datos_json; // Cadena de datos json
String lora_data_received = ""; // Cadena de datos recibidos por LoRa
int lora_data_rssi = 0; // Cantidad de datos recibidos por LoRa
Adafruit_SSD1306 oled_1(128, 64, &Wire, -1); // Declaración pines pantalla OLED
bool oled_1_autoshow = true; // Se crea una variable booleana verdadera
DHT dht25(25, DHT11); // Se declara el PIN de donde irá el medidor de humedad y temperatura
String lora_data_cipher_pw = "Password"; // Contraseña para el cifrado de datos

unsigned long last_task_time_ms = 0; // Variable para medir el tiempo
unsigned long last_receive_time = 0; // Variable para el tiempo del último paquete recibido

// Función para enviar datos cifrados a través de LoRa
void fnc_lora_send(String _data){
    LoRa.beginPacket(); 
    LoRa.print(encryptString(_data, lora_data_cipher_pw)); // Cifra el dato antes de enviarlo
    LoRa.endPacket(); 
}

// Función para cifrar una cadena de texto
String encryptString(String input, String key) {
    String output = "";
    for (int i = 0; i < input.length(); i++) {
        output += char(input[i] ^ key[i % key.length()]);
    }
    return output;
}

void setup() {
  //Se inicializa el Pin 35 como entrada, el dht11, la comunicación LoRa, la pantalla OLED y el Serial.
    pinMode(sensorPin, INPUT); 

    dht25.begin();
    LoRa.setPins(5, 13, 12);
    LoRa.begin(868E6); // Inicializa LoRa en 868MHz
    oled_1.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    Serial.begin(115200);
}

void loop() {
    unsigned long current_time = millis(); // Tiempo actual

    // Actualizar datos solo si ha pasado el tiempo de refresco
    if (current_time - last_task_time_ms >= 20000) { // Actualizar cada 20 segundos
        last_task_time_ms = current_time; // Actualiza el tiempo del último ciclo

        //Se almacenan los datos recogidos en sus respectivas variables.
        temp = dht25.readTemperature();
        hum = dht25.readHumidity(); 
        waterLevel = analogRead(sensorPin); 

  // Mapear el valor de waterLevel de 4095 a 0 y de 0 a 100
        waterLevel = map(waterLevel, 0, 4095, 100, 0);
        waterLevel = constrain(waterLevel, 0, 100);
        
        // Mensajes de depuración
        Serial.print("Temp: ");
        Serial.println(temp);
        Serial.print("Hum: ");
        Serial.println(hum);
        Serial.print("Water Level: ");
        Serial.println(waterLevel);

        // Muestra los datos en la pantalla OLED
        oled_1.clearDisplay();
        oled_1.setTextSize(2);
        oled_1.setTextColor(WHITE);
        oled_1.setCursor(0, 0);
        oled_1.print(String("TEM:"));
        oled_1.display();
        oled_1.setTextSize(2);
        oled_1.setTextColor(WHITE);
        oled_1.setCursor(55, 0);
        oled_1.print(temp);
        oled_1.display();
        oled_1.setTextSize(2);
        oled_1.setTextColor(WHITE);
        oled_1.setCursor(0, 20);
        oled_1.print(String("HUM:"));
        oled_1.display();
        oled_1.setTextSize(2);
        oled_1.setTextColor(WHITE);
        oled_1.setCursor(55, 20);
        oled_1.print(hum);
        oled_1.display();
        oled_1.setTextSize(2);
        oled_1.setTextColor(WHITE);
        oled_1.setCursor(0, 40);
        oled_1.print(String("WTR:"));
        oled_1.display();
        oled_1.setTextSize(2);
        oled_1.setTextColor(WHITE);
        oled_1.setCursor(55, 40);
        oled_1.print(String(waterLevel));
        oled_1.display();

        // Crea y envía datos JSON cifrados a través de LoRa
        s_datos_json = String("{") +
            String((String("\"") + String("T") + String("\"") + String(":") + String(temp))) +
            String(",") +
            String((String("\"") + String("H") + String("\"") + String(":") + String(hum))) +
            String(",") +
            String((String("\"") + String("W") + String("\"") + String(":") + String(waterLevel))) +
            String("}");

        fnc_lora_send(s_datos_json); // Envía los datos JSON a través de LoRa

        delay(1000); // Espera un tiempo antes de la próxima transmisión
    }

    // Verifica si hay datos recibidos por LoRa
    if (LoRa.available()) {
        last_receive_time = current_time; // Actualiza el tiempo del último paquete recibido
    }
}
