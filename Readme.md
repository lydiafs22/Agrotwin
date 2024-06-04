#                                                          Documentación del Proyecto Agrotwin - Arduino
# Índice
    1. Introducción
        1.1 Comunicación LoRa
    2. Descripción del Proyecto
    3. Componentes y Herramientas Utilizadas
    4. Configuración del Emisor
    5. Configuración del Receptor
    6. Configuración del Servidor PHP
    7. Conclusión

# 1. Introducción
En este proyecto, se han utilizado dos placas ESP32 Plus STEAMaker para implementar un sistema de monitoreo ambiental. El sistema consta de un emisor y un receptor, los cuales se comunican utilizando la tecnología de radiofrecuencia LoRa (Long Range). Esta tecnología permite la transmisión de datos a largas distancias con bajo consumo de energía, lo que la hace ideal para aplicaciones de Internet de las Cosas (IoT).

El emisor está equipado con un sensor DHT11, que mide la humedad y la temperatura del ambiente, y un sensor de humedad de la tierra. Estos datos se envían al receptor mediante comunicación LoRa. Además, el emisor cuenta con una pantalla OLED donde se muestran los datos de los sensores en tiempo real. Para garantizar la seguridad de la información transmitida, los datos se cifran utilizando una operación XOR antes de ser enviados.

El receptor, a su vez, recibe los datos cifrados, los descifra y los muestra en su propia pantalla OLED. Después, estos datos se vuelven a cifrar usando XOR y se envían a una base de datos PostgreSQL en un servidor a través de una conexión HTTPS, aprovechando la conectividad WiFi del receptor. Esto permite el almacenamiento seguro y remoto de los datos para su posterior análisis y monitoreo.

# 1.1. Comunicación LoRa
LoRa es una tecnología de modulación de radiofrecuencia diseñada para la transmisión de datos a larga distancia con un bajo consumo de energía. A diferencia de las    comunicaciones de corto alcance como Bluetooth o WiFi, LoRa puede cubrir distancias de varios kilómetros, dependiendo de las condiciones del entorno. Esta característica la   convierte en una opción ideal para aplicaciones IoT, especialmente en áreas rurales o en situaciones donde no es viable el uso de redes convencionales.

La implementación de la comunicación LoRa en este proyecto permite que los datos de los sensores se transmitan de manera eficiente  entre el emisor y el receptor, incluso en  entornos con obstáculos físicos. La elección de esta tecnología asegura que el sistema pueda funcionar de manera autónoma y con un bajo mantenimiento, proporcionando una      solución robusta y escalable para el monitoreo ambiental.

# 2. Descripción del Proyecto
Este proyecto se divide en tres componentes principales:

    * Emisor: Recoge datos de temperatura, humedad del aire y humedad del suelo, los muestra en una pantalla OLED y los envía cifrados con XOR a través de LoRa.
    * Receptor: Recibe los datos enviados por el emisor, los descifra, los muestra en una pantalla OLED y los envía a un servidor PHP cifrados con XOR y por HTTPS.
    * Servidor PHP: Recibe los datos del receptor, los descifra y los almacena en una base de datos PostgreSQL.

# 3. Componentes y Herramientas Utilizadas
Hardware:
    Sensores de temperatura y humedad DHT11
    Sensor de humedad del suelo
    Módulos LoRa
    Pantalla OLED (Adafruit SSD1306)
    Microcontroladores compatibles con Arduino (por ejemplo, ESP32)

Software:
    Arduino IDE

Librerías:
   - SPI.h: Proporciona soporte para la comunicación SPI (Serial Peripheral Interface).

   - LoRa.h: Facilita la comunicación inalámbrica utilizando la tecnología LoRa.

   - Wire.h: Proporciona soporte para la comunicación I2C (Inter-Integrated Circuit), un protocolo de comunicación síncrona que permite que múltiples    dispositivos se comuniquen con el microcontrolador a través de dos líneas.

   - Adafruit_GFX.h: Es una librería gráfica para manejar displays, proporcionando funciones para dibujar formas, texto e imágenes.

   - Adafruit_SSD1306.h: Específicamente diseñada para controlar pantallas OLED basadas en el controlador SSD1306, utilizando funciones de la librería   Adafruit_GFX.

   - ABlocks_DHT.h: Facilita la interacción con sensores DHT (como el DHT11) para obtener lecturas de temperatura y humedad.

   - WiFi.h: Proporciona funcionalidades para conectar el microcontrolador a redes WiFi.

   - WiFiMulti.h: Extiende la funcionalidad de WiFi.h permitiendo gestionar conexiones a múltiples redes WiFi, intentando conectarse a la más fuerte     disponible.

   - HTTPClient.h: Permite realizar solicitudes HTTP (GET, POST, etc.) a servidores web, facilitando la interacción con APIs y servicios web.

   - WiFiClientSecure.h: Proporciona una versión segura (SSL/TLS) del cliente WiFi, permitiendo conexiones HTTPS seguras.

   - time.h: Proporciona funciones para gestionar y manipular el tiempo y las fechas.

   - ArduinoJson.h: Facilita la creación y análisis de datos en formato JSON (JavaScript Object Notation), utilizado para intercambiar datos entre sistemas y APIs.

   - PostgreSQL

   - PHP

   - Servidor web con soporte para PHP y PostgreSQL

# 4. Configuración del Emisor
El código del emisor se encarga de recopilar los datos que recibe a través de los sensores (DHT11 y Sensor de humeda de la tierra), estos datos los interpreta y los muestra en la pantalla OLED. Una vez que muestra los datos, los cifra con XOR y los envía a través de la comunicación LoRa.
La comunicación LoRa se usa en 868MHz (permitido en Europa), enviando los datos al receptor.

        Código del Emisor:
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


# 5. Configuración del Receptor
El código del Receptor se encarga de recibir los datos enviados por LoRa, descifra los datos encriptados en XOR para interpretarlos y mostrarlos en la pantalla OLED de forma legible. Una vez mostrados, los vuelve a cifrar con XOR y los envía utilizando el método POST de HTTPS. Utiliza una cliente inseguro, ya que, no verifica el certificado del servidor.

        Código del Receptor:
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


# 6. Configuración del Servidor PHP
El servidor PHP recibe los datos del receptor por el método POST, los descifra con la clave que ambas ESP32 tienen y envía los datos a la base de PosgreSQL que se encuentra en el propio servidor.
        Código PHP del Servidor:
                                        <?php
                                        
                                        $host = 'localhost';
                                        $db = 'temphumid';
                                        $user = 'postgres';
                                        $pass = 'password';
                                        
                                        $conn = pg_connect("host=$host dbname=$db user=$user password=$pass"); //Conexión a la base de datos
                                        
                                        if (!$conn) {
                                            echo "Ha ocurrido un error.\n";
                                            exit;
                                        }
                                        
                                        function decryptString($input, $key) { //Función para el descifrado de los datos recibidos por POST con HTTPS.
                                            $output = '';
                                            for ($i = 0; $i < strlen($input); $i++) {
                                                $output .= chr(ord($input[$i]) ^ ord($key[$i % strlen($key)]));
                                            }
                                            return $output;
                                        }
                                        
                                        $lora_data_cipher_pw = "Password"; //Clave del cifrado XOR.
                                        
                                        $input = file_get_contents('php://input');
                                        $data = json_decode($input, true);
                                        
                                        $temp = $data['T']; 
                                        $hum = $data['H'];
                                        $waterLevel = $data['W'];
                                        
                                        $result = pg_query_params($conn, 'INSERT INTO measurements (temperature, humidity, water_level) VALUES ($1, $2, $3)', array($temp, $hum,            $waterLevel)); //Función para insertar los datos en PostgreSQL.
                                        
                                        if (!$result) {
                                            echo "Ha ocurrido un error.\n";
                                            exit;
                                        }
                                        
                                        echo "Datos enviados con éxito.";
            
                                        ?>

# 7. Conclusión
Este proyecto demuestra cómo utilizar sensores y módulos de comunicación LoRa para crear un sistema de monitoreo ambiental. 
Los datos se recolectan, cifran, transmiten de forma inalámbrica con LoRa, se descifran y almacenan en una base de datos, proporcionando una solución completa para la monitorización remota de condiciciones ambientales.




