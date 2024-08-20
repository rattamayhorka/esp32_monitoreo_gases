#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include <WiFi.h>
#include "esp_camera.h"

const char* ssid     = "Megacable543";   //your network SSID
const char* password = "Sotan.2023";   //your network password
const char* myDomain = "script.google.com";

String AlarmScript = "/macros/s/AKfycbwVd9Y-Cu9EBoADnzoBRbSnEyz2t8SZ3YQkOOdMzOMwhDYZS0O76oxVBsC5y-m2-gwj/exec";    //recibe fotos y manda correo
String CounterScript = "/macros/s/AKfycbwPEzQ1T2D7w_9Y-TxoemEJT9N3M1oeLthccOTvZMu257t4OvTaXbdOQdkaEmlGt_Gl-Q/exec";    //recibe fotos y agrega en drive

String myFilename = "filename=GASES.jpg";
String mimeType = "&mimetype=image/jpeg";
String myImage = "&data=";

int waitingResponse = 60000; //Wait 30 seconds to google response.

long EsperaEnvio = 5 * 60; // minuto 
 
int counter = EsperaEnvio;

//definiciones de puerto camara
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

bool PulseAlarm = false;

//boolean enviar = true;

// definicion led
#define LED_FLASH 4 
#define LED_OK 12
#define LED_FAIL 15

static void IRAM_ATTR FuncPulseAlarm(void * arg){
  PulseAlarm = true;
}

void setup(){
  pinMode(LED_FLASH, OUTPUT);
  pinMode(LED_OK, OUTPUT);
  pinMode(LED_FAIL, OUTPUT);

  digitalWrite(LED_FLASH, LOW); 
  digitalWrite(LED_FAIL, LOW); 
  digitalWrite(LED_OK, LOW); 
  

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  delay(10);
  
  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  


  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
    
  Serial.println("");

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
  config.frame_size = FRAMESIZE_VGA;  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
  config.jpeg_quality = 10;
  config.fb_count = 1;
  
  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    digitalWrite(LED_FAIL, HIGH);
    digitalWrite(LED_OK, LOW); 
    Serial.printf("error-1 antes de timer 40000");
    delay(40000);
    Serial.printf("error-1 despues de timer 40000");
    Serial.printf("error-1");
    ESP.restart();
  }
  err = gpio_isr_handler_add(GPIO_NUM_13, &FuncPulseAlarm, (void *) 13);  
  if (err != ESP_OK){
     Serial.printf("hubo un error 0x%x \r\n", err); 
  }
  
  err = gpio_set_intr_type(GPIO_NUM_13, GPIO_INTR_POSEDGE);
  if (err != ESP_OK){
    Serial.printf("hubo un error 0x%x \r\n", err);
  }
}

String urlencode(String str){
  String encodedString="";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i =0; i < str.length(); i++){
    c=str.charAt(i);
    if (c == ' '){
      encodedString+= '+';
    } else if (isalnum(c)){
      encodedString+=c;
    } else{
      code1=(c & 0xf)+'0';
      if ((c & 0xf) >9){
          code1=(c & 0xf) - 10 + 'A';
      }
      c=(c>>4)&0xf;
      code0=c+'0';
      if (c > 9){
          code0=c - 10 + 'A';
      }
      code2='\0';
      encodedString+='%';
      encodedString+=code0;
      encodedString+=code1;
    }
    yield();
  }
  return encodedString;
}

void envioAlarma(){
  Serial.println("Conectado a " + String(myDomain));
  WiFiClientSecure client;
  client.setInsecure(); 
  if (client.connect(myDomain, 443)){
    Serial.println("Conexion establecida");
    digitalWrite(LED_OK, HIGH);  // turn the LED on (HIGH is the voltage level)
    digitalWrite(LED_FAIL, LOW);
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();  
    if(!fb){
      Serial.println("Camera capture failed");
      delay(1000);
      digitalWrite(LED_FAIL, HIGH);
      delay(40000);
      ESP.restart();
      return;
    }
  
    char *input = (char *)fb->buf;
    char output[base64_enc_len(3)];
    String imageFile = "";
    for (int i=0;i<fb->len;i++){
      base64_encode(output, (input++), 3);
      if (i%3==0) imageFile += urlencode(String(output));
    }
    String Data = myFilename+mimeType+myImage;
    
    esp_camera_fb_return(fb);

    Serial.println("enviando foto a Google Drive.");
    client.println("POST " + AlarmScript + " HTTP/1.1");
    client.println("Host: " + String(myDomain));
    client.println("Content-Length: " + String(Data.length()+imageFile.length()));
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println();
    
    client.print(Data);
    int Index;
    for (Index = 0; Index < imageFile.length(); Index = Index+1000){
      client.print(imageFile.substring(Index, Index+1000));
    }
    
    Serial.println("fotoalarma: esperando respuesta.");
    long int StartTime=millis();
    while (!client.available()){
      Serial.print(".");
      delay(100);
      if ((StartTime+waitingResponse) < millis()){
        Serial.println();
        Serial.println("Sin respuesta.");
        //If you have no response, maybe need a greater value of waitingResponse
        break;
      }
    }
    Serial.println();
    Serial.println("google: foto recibida");//respuesta de google   
    /*while (client.available()){
      //Serial.print(char(client.read()));
    }*/
  }
  else{         
    Serial.println("Conectado a " + String(myDomain) + " failed.");
    digitalWrite(LED_FAIL, HIGH); 
    digitalWrite(LED_OK, LOW); 
    WiFi.begin(ssid, password); 
  }
  client.stop();
}

void envioFotoContador(){
  
  Serial.println("Conectado a " + String(myDomain));
  WiFiClientSecure client;
  client.setInsecure(); 
  if (client.connect(myDomain, 443)){
    Serial.println("Conexion establecida");
    digitalWrite(LED_OK, HIGH);  // turn the LED on (HIGH is the voltage level)
    digitalWrite(LED_FAIL, LOW);
  
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();  
    if(!fb){
      Serial.println("Camera capture failed");
      delay(1000);
      digitalWrite(LED_FAIL, HIGH);
      delay(40000);
      ESP.restart();
      return;
    }
  
    char *input = (char *)fb->buf;
    char output[base64_enc_len(3)];
    String imageFile = "";
    for (int i=0;i<fb->len;i++){
      base64_encode(output, (input++), 3);
      if (i%3==0) imageFile += urlencode(String(output));
    }
    String Data = myFilename+mimeType+myImage;
    
    esp_camera_fb_return(fb);
    
    Serial.println("Enviando foto a Google Drive.");
    
    client.println("POST " + CounterScript + " HTTP/1.1");
    client.println("Host: " + String(myDomain));
    client.println("Content-Length: " + String(Data.length()+imageFile.length()));
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println();
    
    client.print(Data);
    int Index;
    for (Index = 0; Index < imageFile.length(); Index = Index+1000) {
      client.print(imageFile.substring(Index, Index+1000));
    }
    
    Serial.println("fotocontador: Esperando respuesta.");
    long int StartTime=millis();
    while (!client.available()){
      Serial.print(".");
      delay(100);
      if ((StartTime+waitingResponse) < millis()){
        Serial.println();
        Serial.println("Sin respuesta.");
        //If you have no response, maybe need a greater value of waitingResponse
        break;
      }
    }
    Serial.println();
    Serial.println("google: foto recibida");//respuesta de google
    //Serial.println();//respuesta de google   
    /*while (client.available()){
      Serial.print(char(client.read()));
    }*/  
  }
  else{         
    Serial.println("Conectado a " + String(myDomain) + " failed.");
    digitalWrite(LED_FAIL, HIGH); 
    digitalWrite(LED_OK, LOW); 
    WiFi.begin(ssid, password); 
  }
  client.stop();
}

void loop(){
  esp_camera_return_all();
  if (PulseAlarm) {
    Serial.println("Alarma activada!!!");
    digitalWrite(LED_FLASH, HIGH); // Enciende el LED de flash
    envioAlarma();
    digitalWrite(LED_FLASH, LOW); // Apaga el LED de flash
    PulseAlarm = false; // Restablece la bandera
  }
  if(counter == EsperaEnvio ){
    Serial.println("envio foto...");
    digitalWrite(LED_FLASH, HIGH); // Enciende el LED de flash
    envioFotoContador();    
    digitalWrite(LED_FLASH, LOW); // Enciende el LED de flash
    counter = 0;
  }
  counter++;
  Serial.printf("contador %d \r\n", counter);
  delay(1000); 
}
