#include "config.h"

int count = 0;

// Conexión al feed de Adafruit IO
AdafruitIO_Feed *centralfeed = io.feed("Central_de_gases");

// Tiempos para debounce
unsigned long lastDebounceTime0 = 0;
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
const unsigned long debounceDelay = 50;

// Pines y estados del hardware
#define ALARM0_INPUT 15
#define ALARM1_INPUT 13
#define ALARM2_INPUT 14

int lastPinState0 = HIGH; 
int lastPinState1 = HIGH;
int lastPinState2 = HIGH;

int currentPinState0 = HIGH; 
int currentPinState1 = HIGH; 
int currentPinState2 = HIGH;

bool presionBaja = false;  
bool presionBancada = false;
bool presionLinea = false;

unsigned long alarmasState = 0; 
const long alarmasInterval = 60000; 

int cont = 0;

void verificarAlarmas() {
  
  String estado = "Baja=" + String(presionBaja ? "ER" : "OK") + ";";
  estado += "Bancada=" + String(presionBancada ? "ER" : "OK") + ";";
  estado += "Linea=" + String(presionLinea ? "ER" : "OK");


  centralfeed->save(estado);
  Serial.println("Estado enviado: " + estado);
}

void setup() {
  pinMode(ALARM0_INPUT, INPUT_PULLUP); 
  pinMode(ALARM1_INPUT, INPUT_PULLUP); 
  pinMode(ALARM2_INPUT, INPUT_PULLUP); 

  Serial.begin(115200);
  while (!Serial);

  Serial.print("Connecting to Adafruit IO");

  io.connect();

  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println(io.statusText());
}

void loop() {
  io.run();

  unsigned long currentMillis = millis();

  // Botón 1: Alarma Baja
  int rbaja = digitalRead(ALARM0_INPUT);
  if (rbaja != lastPinState0) lastDebounceTime0 = currentMillis;
  if ((currentMillis - lastDebounceTime0) > debounceDelay) {
    if (rbaja != currentPinState0) {
      currentPinState0 = rbaja;
      if (currentPinState0 == LOW) {
        Serial.println("Alarma baja");
        presionBaja = true;
      } else {
        presionBaja = false;
        Serial.println("Alarma baja apagada");
      }
    }
  }
  lastPinState0 = rbaja;

  // Botón 2: Alarma Bancada
  int rbancada = digitalRead(ALARM1_INPUT);
  if (rbancada != lastPinState1) lastDebounceTime1 = currentMillis;
  if ((currentMillis - lastDebounceTime1) > debounceDelay) {
    if (rbancada != currentPinState1) {
      currentPinState1 = rbancada;
      if (currentPinState1 == LOW) {
        Serial.println("Alarma bancada");
        presionBancada = true;
      } else {
        presionBancada = false;
        Serial.println("Alarma bancada apagada");
      }
    }
  }
  lastPinState1 = rbancada;

  //Botón 3: Alarma Línea
  int rlinea = digitalRead(ALARM2_INPUT);
  if (rlinea != lastPinState2) lastDebounceTime2 = currentMillis;
  if ((currentMillis - lastDebounceTime2) > debounceDelay) {
    if (rlinea != currentPinState2) {
      currentPinState2 = rlinea;
      if (currentPinState2 == LOW) {
        Serial.println("Alarma línea");
        presionLinea = true;
      } else {
        presionLinea = false;
        Serial.println("Alarma línea apagada");
      }
    }
  }
  lastPinState2 = rlinea;
   
  // Verificar alarmas periódicamente
  if (currentMillis - alarmasState >= alarmasInterval) {
    verificarAlarmas();
    alarmasState = currentMillis;
  }

  delay(1000);
  
}
