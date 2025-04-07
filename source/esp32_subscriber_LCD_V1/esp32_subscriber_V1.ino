/************************** Configuration ***********************************/
// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"
#include <LiquidCrystal.h>


/************************ Example Starts Here *******************************/
unsigned long lastUpdateTime = 0;  // Guarda el tiempo de la última actualización en milisegundos
unsigned long timeout = 90000; // 1 minuto (60,000 ms)
bool alarmState = true;
bool estadoDeBotton = false;
const byte pinBuzzer = 19; // Pin conectado al buzzer
int temporizadorDeSilencio = 0;

#define bottonSilence 27
#define NOTE_B3  200 // Notas (frecuencias en Hz)
String Pbancada;//variables para alamcenar e imprimir
String Plinea;
String Pbaja;
String baja;//variables para guardar estados
String linea;
String bancada;
LiquidCrystal lcd(23, 22, 18, 5, 17, 16); // Pines del LCD


// Configuración del feed de Adafruit IO
AdafruitIO_Feed *counter = io.feed("Central_de_gases");

//caracter de señal de wifi 
byte wifi[8] = {
  B00000,
  B00000,
  B00000,
  B11111,
  B00000,
  B01110,
  B00000,
  B00100
};
byte wifinot[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00101,
  B00010,
  B00101
};
byte base[8] = {
  B00000,
  B00000,
  B00000,
  B11111,
  B00000,
  B11111,
  B00000,
  B11111
};

void verificarConexion() {
    // Verificar si ha pasado un minuto sin recibir actualizaciones
    if (millis() - lastUpdateTime > timeout) {
        // Si pasó más de un minuto sin datos, mostrar un mensaje de error
        lcd.clear();
        lcd.setCursor(14, 1);
        lcd.write(byte(2));
         lcd.setCursor(15, 1);
        lcd.write(byte(1));
        alarmState = false; // Apagar el buzzer cuando "baja" cambie

        
    }
}

//funcion de mostrar estatus de wifi

void mostrarEstadoWifi() {
    lcd.setCursor(15, 1); // Posición en la esquina inferior derecha
    if (io.mqttStatus() == AIO_CONNECTED) {
        lcd.write(byte(0)); // Mostrar símbolo Wi-Fi conectado
    } else {
      lcd.setCursor(0, 1); // Posicionar el cursor al inicio de la fila 1
      lcd.print("                "); // Escribir 16 espacios para limpiar
      lcd.setCursor(0, 1); // Devolver el cursor al inicio de la fila 1
      lcd.print("conectando ...");
      lcd.write(byte(1)); // Mostrar símbolo Wi-Fi desconectado
    }
}

//funcion DE IMPRIMIR
void variablesprint(){
  
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BAJA BANC. LINEA"); // Línea 1
    lcd.setCursor(0, 1);
    if (baja == "ER" || bancada == "ER"|| linea == "ER"){
      alarmState = true; // prender el buzzer cuando "baja" cambie
    }
    else {
      alarmState = false; // Apagar el buzzer cuando "baja" cambie
    }
  
    if (baja == "ER"){
    lcd.setCursor(0, 1);
    lcd.print("ACT!");
    }
    else{
     lcd.setCursor(0,1);
     lcd.print("OK");
    }
    if (bancada == "ER"){
     lcd.setCursor(6, 1);
     lcd.print("ACT!");
    }
    else{
     lcd.setCursor(6,1);
     lcd.print("OK");
   }
    if (linea == "ER"){
     lcd.setCursor(11, 1);
     lcd.print("ACT!");
    }
    else{
     lcd.setCursor(11,1);    
     lcd.print("OK");
    }     
}

// Función para procesar datos con punteros
void procesarDatos(int *ptr) {
    if (ptr != NULL) {
        Serial.println("Puntero válido, procesando datos...");
        *ptr += 10; // Operar con el puntero
    } else {
        Serial.println("Error: puntero inválido.");
    }
}

//FUNCION RESET 

void(* resetFunc) (void) = 0; // Declarar un puntero a la dirección 0

//funcion de buzzer
void buzzer(){
  if (alarmState) {
    tone(pinBuzzer,NOTE_B3, 400);
    delay(600);//la diferencia entre el tono y el delay son los ms apagada
  }
  else {
    noTone(pinBuzzer); // Detener el buzzer
  }
}

void setup() {
  //creaciòn de caracteres
  lcd.createChar(0, wifi);
  lcd.createChar(1, wifinot); 
  lcd.createChar(2, base); 


    //salida de buzze
    pinMode(pinBuzzer, OUTPUT);
    pinMode(bottonSilence, INPUT_PULLDOWN);
    // Iniciar la comunicación serial
    Serial.begin(115200);
    lcd.begin(16, 2);

    // Esperar a que el monitor serial esté listo
    while (!Serial);

    Serial.print("Connecting to Adafruit IO");
    lcd.print("Conectando a MQTT");

    // Conexión a Adafruit IO
    io.connect();

    // Asignar la función handleMessage al feed
    counter->onMessage(handleMessage);

    // Esperar la conexión MQTT
    unsigned long startAttemptTime = millis();
    unsigned long connectionTimeout = 10000; // 10 segundos de espera
    while (io.mqttStatus() < AIO_CONNECTED) {
        Serial.print(".");
        delay(500);

        if (millis() - startAttemptTime > connectionTimeout) {
            Serial.println("Error: No se pudo conectar a Adafruit IO.");
            lcd.clear();
            lcd.print("Error: MQTT");
            resetFunc();
            return; // Salir del setup si no se conecta
        }
    }

    Serial.println("\nConectado a Adafruit IO");
    lcd.clear();
    lcd.setCursor(0, 0);
    Serial.print("7");
    lcd.print("MQTT Conectado");

    // Solicitar el último valor del feed
    counter->get();

    // Ejemplo de punteros y memoria dinámica
    int *datos = NULL; // Inicializar el puntero
    datos = (int *)malloc(sizeof(int)); // Asignar memoria
    if (datos != NULL) {
        *datos = 20;
        procesarDatos(datos); // Pasar el puntero
        Serial.println(*datos);
        free(datos); // Liberar memoria
        datos = NULL; // Evitar punteros colgantes
    } else {
        Serial.println("Error: no se pudo asignar memoria.");
    }
}

void loop() {
  // Mantener la conexión con Adafruit IO
  io.run();
  buzzer();
  variablesprint();
  mostrarEstadoWifi();
  // Verificar si hay un timeout de conexión
  verificarConexion();  // Esta línea se agregó
  //silenciar alarma 2 min 
  int botonsilencio = digitalRead(bottonSilence);
  //Serial.println(botonsilencio);
 if (botonsilencio == HIGH){
    estadoDeBotton= true;  
  }
  if(estadoDeBotton==true){
    if(temporizadorDeSilencio<=4500){
       Serial.println("inicio temp");
       temporizadorDeSilencio++;
       alarmState = false;
    }
    else{
     estadoDeBotton=false;
     temporizadorDeSilencio=0;
     Serial.println("fin temp");
    } 
  } 
  


}

// Función llamada al recibir un mensaje del feed
void handleMessage(AdafruitIO_Data *data) {

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
