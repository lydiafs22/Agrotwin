#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ABlocks_DHT.h>

const int sensorPin = 35;
int waterLevel;
double temp;
double hum;
String s_datos_json;

Adafruit_SSD1306 oled_1(128, 64, &Wire, -1);
bool oled_1_autoshow = true;

DHT dht25(25, DHT11);

String lora_data_cipher_pw = "Password";

unsigned long last_task_time_ms = 0;

void fnc_lora_send(String _data){
    LoRa.beginPacket(); 
    LoRa.print(encryptString(_data, lora_data_cipher_pw)); 
    LoRa.endPacket(); 
}

String encryptString(String input, String key) {
    String output = "";
    for (int i = 0; i < input.length(); i++) {
        output += char(input[i] ^ key[i % key.length()]);
    }
    return output;
}

void setup() {
    pinMode(sensorPin, INPUT);
    dht25.begin();
    LoRa.setPins(5, 13, 12);
    LoRa.begin(868E6);
    oled_1.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    Serial.begin(115200);
}
// Definir el intervalo de tiempo para cambiar entre datos
unsigned long display_interval = 5000; // 5 segundos por pantalla
unsigned long last_display_time = 0;
int display_state = 0;

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

void loop() {
    unsigned long current_time = millis();
    if (current_time - last_display_time > display_interval) {
        last_display_time = current_time;
        display_state = (display_state + 1) % 3;
        updateOLED();
    }
    if (current_time - last_task_time_ms >= 20000) {
        last_task_time_ms = current_time;
        
        temp = dht25.readTemperature();
        hum = dht25.readHumidity(); 
        waterLevel = analogRead(sensorPin);
        waterLevel = map(waterLevel, 0, 4095, 100, 0);
        waterLevel = constrain(waterLevel, 0, 100);

        Serial.print("Temp: "); Serial.println(temp);
        Serial.print("Hum: "); Serial.println(hum);
        Serial.print("Water Level: "); Serial.println(waterLevel);

        s_datos_json = String("{") +
            String((String("\"") + String("T") + String("\"") + String(":") + String(temp))) +
            String(",") +
            String((String("\"") + String("H") + String("\"") + String(":") + String(hum))) +
            String(",") +
            String((String("\"") + String("W") + String("\"") + String(":") + String(waterLevel))) +
            String("}");

        fnc_lora_send(s_datos_json);
        delay(1000);
    }
}
