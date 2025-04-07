#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "config.h"

  AdafruitIO_Feed *centralfeed = io.feed("Central_de_gases");
  String BOTtoken = "7256551752:AAF1SIjvFdvclianBJ5MmxRAoz18BbAnpkA";  
  String CHAT_ID1 = "528825472";  //RCE
  String CHAT_ID2 = "1899462806"; //JUGA
  String CHAT_ID3 = "0000000000"; //JUGA
  String EngineerChatID = CHAT_ID1;
  String baja;
  String linea;
  String bancada;
  String Pbancada;
  String Plinea;
  String Pbaja;

  bool sendPhoto = false;
  bool StatusAlarm = 0 ;
  bool presionBaja = false; 
  bool flashState = LOW;
  bool alarmInicio = true;
    
  // Variables para el debounce
  unsigned long alarmasState = 0; // Variable para almacenar el último tiempo de envío
  unsigned long lastDebounceTime = 0; // Última vez que se detectó un cambio
  unsigned long lastTimeBotRan;
  unsigned long previousMillis = 0;
  unsigned long lastUpdateTime = 0;  // Guarda el tiempo de la última actualización en milisegundos
  unsigned long timeout = 90000; // 1 minuto (60,000 ms)

  const long interval = 1000;        // Intervalo de tiempo (1000 ms = 1 segundo)
  const long alarmasInterval = 60000; // Intervalo de 1 minuto (60,000 ms)

  const unsigned long debounceDelay = 50; // Tiempo de debounce en milisegundos

  int lastPinState = 0; // Estado anterior del pin
  int currentPinState = HIGH; // Estado actual del pin
  int botRequestDelay = 1000;
  int counter = 0;
  int todasAlarmscounter = 0;
  int dewarBancadaCounter = 0;
  int dewarCounter = 0;
  int bancadaLineaCounter = 0;
  int bancadaCounter = 0;
  int dewarLineaCounter = 0;
  int lineaCounter = 0;
  int alarmaChicaCounter = 0;
  int alarmaChicaSilenciadaCounter = 0;

  //conexion de internet
  WiFiClientSecure clientTCP;
  UniversalTelegramBot bot(BOTtoken, clientTCP);

  void(* resetFunc) (void) = 0;
  
  //pines
  #define FLASH_LED_PIN 2   
  #define LED_OK 12
  #define LED_FAIL 15
  #define OutputSignal_Alarm 14

  #define ALARM0_INPUT 13

  //CAMERA_MODEL_AI_THINKER
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27

  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22

// funciones 
 
// Función para procesar datos con punteros
void procesarDatos(int *ptr) {
    if (ptr != NULL) {
        Serial.println("Puntero válido, procesando datos...");
        *ptr += 10; // Operar con el puntero
    } else {
        Serial.println("Error: puntero inválido.");
    }
}

void statusLEDs() {
  if (io.status() == AIO_CONNECTED) {
    digitalWrite(LED_OK, HIGH);  // Enciende el LED OK
    digitalWrite(LED_FAIL, LOW); // Apaga el LED FAIL
  } else {
    digitalWrite(LED_OK, LOW);   // Apaga el LED OK
    digitalWrite(LED_FAIL, HIGH); // Enciende el LED FAIL
    resetFunc(); 
  }
}

String getAlias(String chatID) {
  if (chatID == CHAT_ID1) return "RCE";
  if (chatID == CHAT_ID2) return "JUGA";
  if (chatID == CHAT_ID3) return "GQV";
  return "Desconocido";
}

void configInitCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;

  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Inicio de cámara falló. Error: 0x%x", err);
    delay(1000);
    ESP.restart();
  }
  if (err != ESP_OK){
    Serial.printf("Error al cambiar tipo de interrupción. Error: 0x%x \r\n", err);
  }
}

// Función llamada al recibir un mensaje del feed
void leerdatos(AdafruitIO_Data *data) {

    lastUpdateTime = millis(); // Reiniciar el temporizador
      
    // Obtener el mensaje recibido
    String message = data->value();
    
    if (message == "") {
        Serial.println("Error: Mensaje vacío recibido.");
        return;
    }

    Serial.print("Comando recibido: ");
    Serial.println(message);

    // Índices de las claves
    int bajaIndex = message.indexOf("Baja=");
    int bancadaIndex = message.indexOf("Bancada=");
    int lineaIndex = message.indexOf("Linea=");

    // Extraer valores si están presentes
    if (bajaIndex != -1 && message.indexOf(";", bajaIndex) != -1) {
        baja = message.substring(bajaIndex + 5, message.indexOf(";", bajaIndex));
    }
    if (bancadaIndex != -1 && message.indexOf(";", bancadaIndex) != -1) {
        bancada = message.substring(bancadaIndex + 8, message.indexOf(";", bancadaIndex));
    }
    if (lineaIndex != -1) {
        linea = message.substring(lineaIndex + 6);
    }

    // Mostrar valores en el monitor serial
    Serial.print("Baja: ");
    Serial.println(baja);
    Serial.print("Bancada: ");
    Serial.println(bancada);
    Serial.print("Linea: ");
    Serial.println(linea);  

    
}
void handleNewMessages(int numNewMessages) {
  Serial.print("Comandos recibidos: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    EngineerChatID = chat_id; // Guarda el chat_id del remitente en EngineerChatID

    if (chat_id != CHAT_ID1 && chat_id != CHAT_ID2 && chat_id != CHAT_ID3) {
      bot.sendMessage(chat_id, "Usuario no autorizado", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);
    
    String from_name = bot.messages[i].from_name;
    if (text == "/inicio") {
      String welcome = "Bienvenido, " + from_name + "\n";
      welcome += "Usa los siguientes comandos para conocer el status de la central de gases.\n";
      welcome += "/presiones : toma una foto de los manómetros\n";
      welcome += "/apagar : apagar alarma\n";
     
      bot.sendMessage(EngineerChatID, welcome, "");
    }
    if (text == "/apagar") {
      //StatusAlarm = false;
      digitalWrite(OutputSignal_Alarm, HIGH);
      delay(3000);
      digitalWrite(OutputSignal_Alarm, LOW);   
      bot.sendMessage(EngineerChatID, "señal de apagado enviada.", "");
      presionBaja = false;
      Serial.println(presionBaja ? "true" : "false");
      

    }

    
    
    if (text == "/presiones") {
      sendPhoto = true;
      Serial.println("comando de presiones recibido.");
      bot.sendMessage(EngineerChatID, "Preparando foto", "");
      digitalWrite(FLASH_LED_PIN, HIGH);
    }
  }
}

String sendPhotoTelegram() {
  //digitalWrite(LED_OK, LOW);

  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  //Dispose first picture because of bad quality
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  esp_camera_fb_return(fb); // dispose the buffered image
  
  // Take a new photo
  fb = NULL;  
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Error al tomar foto");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }
  digitalWrite(FLASH_LED_PIN, LOW); //apago FLASH_LED
  Serial.println("Conectando a " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Conexion establecida");
    Serial.println("enviando foto...");
    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + EngineerChatID + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    size_t imageLen = fb->len;
    size_t extraLen = head.length() + tail.length();
    size_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  }
  else {
    getBody="conectando a api.telegram.org failed.";
    Serial.println("Falló la conexión a la api.telegram.org.");
  }
  bot.sendMessage(EngineerChatID, "Foto enviada", ""); 
  Serial.print("Foto enviada");
  
  String alias = getAlias(EngineerChatID);
  String mqttMessage = "Foto enviada a " + alias;


  return getBody;
}

void setup() {

  // start the serial connection
  Serial.begin(115200);

  // wait for serial monitor to open
  while(! Serial);

  Serial.print("Connecting to Adafruit IO");

  // connect to io.adafruit.com
  io.connect();

  centralfeed->onMessage(leerdatos);
  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());

  configInitCamera();   // Config and init the camera
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  //Serial.begin(115200); // Init Serial Monitor

  pinMode(FLASH_LED_PIN, OUTPUT);
  pinMode(LED_OK, OUTPUT);
  pinMode(LED_FAIL, OUTPUT);
  pinMode(OutputSignal_Alarm, OUTPUT);
  pinMode(ALARM0_INPUT, INPUT); // Configurar el pin como entrada con resistencia pull-up interna
  
  
  
  digitalWrite(LED_FAIL, HIGH); 
  digitalWrite(LED_OK, LOW); 
  digitalWrite(FLASH_LED_PIN, flashState);

  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Conectado a telegram...");
  Serial.println();
  Serial.print("IP de ESP32-CAM: ");
  Serial.println(WiFi.localIP());

  Serial.print("bot en linea...");
  //centralfeed->save("bot en linea"); // Publicar en MQTT

  // Send a welcome message to both chat IDs
  String welcomeMessage = "El bot está en línea.\r\nPara mas información escriba '/inicio'.";
  bot.sendMessage(CHAT_ID1, welcomeMessage, "");
  bot.sendMessage(CHAT_ID2, welcomeMessage, "");
  //
  centralfeed->get();

  // Ejemplo de punteros y memoria dinámica
    int *datos = NULL; // Inicializar el puntero
    datos = (int *)malloc(sizeof(int)); // Asignar memoria
    if (datos != NULL) {
        *datos = 20;
        procesarDatos(datos); // Pasar el puntero
        //Serial.println(*datos);
        free(datos); // Liberar memoria
        datos = NULL; // Evitar punteros colgantes
    } else {
        Serial.println("Error: no se pudo asignar memoria.");
    }
}

void loop() {
  io.run(); 
  statusLEDs(); //muestra con los leds verde y rojo, el status de conectividad al broker (io.adafruit)
  //Serial.print(AIO_CONNECTED); 
   
  unsigned long currentMillis = millis(); // Obtiene el tiempo actual

  // Enviar foto si se solicita
  if (sendPhoto) {
    Serial.println("Preparando foto");
    sendPhotoTelegram(); 
    sendPhoto = false; 
  }

  // esperando mensajes desde el bot de telegram
  if (currentMillis > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("Comando recibido");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = currentMillis;
  }

  // Leer estado del pin de alarma con debounce
  lastPinState = digitalRead(ALARM0_INPUT); //cambio de estado en pin de alarma (entrada)
    
  if (lastPinState == LOW) { // Alarma activada
    while(alarmaChicaCounter<1){
      alarmaChicaCounter++;
      Serial.println("Alarma chica encendida");
      presionBaja = true;

      Serial.println(presionBaja ? "true" : "false");
      bot.sendMessage(CHAT_ID2, "ALARMA CHICA ENCENDIDA", "");
      bot.sendMessage(CHAT_ID1, "ALARMA CHICA ENCENDIDA", "");
      alarmaChicaSilenciadaCounter=0;
      alarmInicio = false;
    } 
  } else { // Alarma desactivada
    while(alarmaChicaSilenciadaCounter<1 && alarmInicio != true ){
      alarmaChicaSilenciadaCounter++;
      Serial.println("Alarma silenciada");
      bot.sendMessage(CHAT_ID2, "ALARMA CHICA SILENCIADA", "");
      bot.sendMessage(CHAT_ID1, "ALARMA CHICA SILENCIADA", "");
      alarmaChicaCounter=0; //reinicio de contador alarma chica (para no enviar mensajes duplicados)
      }
  }

  //verificaciòn de estado de alarmas
  if(baja == "ER"|| bancada == "ER" || linea == "ER"){ //si cualquiera de las presiones es ER verifica cual se activo 
    //Alarma dewar,bancada y linea
    if(baja == "ER" && bancada == "ER" && linea == "ER"){
      while(todasAlarmscounter<1){
        todasAlarmscounter++;
        bot.sendMessage(CHAT_ID1, "DEWAR:ER; BANCADA:ER; LINEA:ER", "");
        bot.sendMessage(CHAT_ID2, "DEWAR:ER; BANCADA:ER; LINEA:ER", "");
      }
    }
    else{
      todasAlarmscounter=0;
    }
    //Alarma dewar,bancada
    if(baja == "ER" && bancada == "ER" && linea == "OK"){
      while(dewarBancadaCounter<1){
        dewarBancadaCounter++;
        bot.sendMessage(CHAT_ID1, "DEWAR:ER; BANCADA:ER; LINEA:OK", "");
        bot.sendMessage(CHAT_ID2, "DEWAR:ER; BANCADA:ER; LINEA:OK", "");
      }
    }
    else{
      dewarBancadaCounter=0;
    }
    //Alarma dewar
    if(baja == "ER" && bancada == "OK" && linea == "OK"){
      while(dewarCounter<1){
        dewarCounter++;
        bot.sendMessage(CHAT_ID1, "DEWAR:ER; BANCADA:OK; LINEA:OK", "");
        bot.sendMessage(CHAT_ID2, "DEWAR:ER; BANCADA:OK; LINEA:OK", "");
      }
    }
    else{
      dewarCounter=0;
    }
    //Alarma bancada y linea
    if(baja == "OK" && bancada == "ER" && linea == "ER"){
      while(bancadaLineaCounter<1){
        bancadaLineaCounter++;
        bot.sendMessage(CHAT_ID1, "DEWAR:OK; BANCADA:ER; LINEA:ER", "");
        bot.sendMessage(CHAT_ID2, "DEWAR:OK; BANCADA:ER; LINEA:ER", "");
      }
    }
    else{
      bancadaLineaCounter=0;
    }
    //Alarma bancada
    if(baja == "OK" && bancada == "ER" && linea == "OK"){
      while(bancadaCounter<1){
        bancadaCounter++;
        bot.sendMessage(CHAT_ID1, "DEWAR:OK; BANCADA:ER; LINEA:OK", "");
        bot.sendMessage(CHAT_ID2, "DEWAR:OK; BANCADA:ER; LINEA:OK", "");
      }
    }
    else{
      bancadaCounter=0;
    }
    // Alarma dewar y linea
    if(baja == "ER" && bancada == "OK" && linea == "ER"){
      while(dewarLineaCounter<1){
        dewarLineaCounter++;
        bot.sendMessage(CHAT_ID1, "DEWAR:ER; BANCADA:OK; LINEA:ER", "");
        bot.sendMessage(CHAT_ID2, "DEWAR:ER; BANCADA:OK; LINEA:ER", "");
      }
    }
    else{
      dewarLineaCounter=0;
    }
    //Alarma linea
    if(baja == "OK" && bancada == "OK" && linea == "ER"){
      while(lineaCounter<1){
        lineaCounter++;
        bot.sendMessage(CHAT_ID1, "DEWAR:OK; BANCADA:OK; LINEA:ER", "");
        bot.sendMessage(CHAT_ID2, "DEWAR:OK; BANCADA:OK; LINEA:ER", "");
      }
    }
    else{
      lineaCounter=0;
    }
  } 
  
  // Verificar si se mantiene la conexion al broker    
  if (io.mqttStatus() == AIO_CONNECTED) {
        digitalWrite(LED_OK,HIGH);
        delay(2000);
        digitalWrite(LED_OK,LOW);
        digitalWrite(LED_FAIL,LOW);
  } else {
      digitalWrite(LED_OK,LOW);
      digitalWrite(LED_FAIL,HIGH);
      resetFunc();
  }
     
}
