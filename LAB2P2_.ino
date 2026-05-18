#include <Arduino.h>

// a b c d e f g
const int segPins[7] = {13, 12, 14, 27, 26, 25, 33};

// Displays ánodo común
const int digitPins[3] = {18, 19, 21};

// Tabla de números
byte numbers[10][7] = {

  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}  // 9
};

// BOTONES 

const int btnEntrada = 4;
const int btnSalida  = 5;

const int swEntrada = 15;
const int swSalida  = 2;

//  SERVO
const int servoPin = 22;

// PWM SERVO
const int servoClose = 20;
const int servoOpen  = 110;

// TIMER 
hw_timer_t *timer = NULL;

//  VARIABLES 

// Espacios disponibles
volatile int espacios = 250;

// Barrido displays
volatile int currentDigit = 0;

// Dígitos
volatile int centenas = 2;
volatile int decenas  = 5;
volatile int unidades = 0;

//  FLAGS ISR 
volatile bool flagEntrada = false;
volatile bool flagSalida  = false;

// Anti rebote ISR
volatile unsigned long lastInterruptEntrada = 0;
volatile unsigned long lastInterruptSalida  = 0;

const int debounce = 250;

// CONTROL SERVO 

bool servoActivo = false;

unsigned long servoStart = 0;

const unsigned long servoTiempoAbierto = 3000;

//  DEBUG 

unsigned long lastDebug = 0;

// ACTUALIZAR DISPLAY 

void actualizarDigitos() {

  centenas = espacios / 100;

  decenas = (espacios / 10) % 10;

  unidades = espacios % 10;
}

//  ABRIR SERVO
void abrirServo() {

  ledcWrite(servoPin, servoOpen);

  servoActivo = true;

  servoStart = millis();

  Serial.println("Servo ABIERTO");
}

//  CERRAR SERVO 

void cerrarServo() {

  ledcWrite(servoPin, servoClose);

  servoActivo = false;

  Serial.println("Servo CERRADO");
}

// ISR ENTRADA 

void IRAM_ATTR isrEntrada() {

  unsigned long tiempo = millis();

  if(tiempo - lastInterruptEntrada > debounce) {

    flagEntrada = true;

    lastInterruptEntrada = tiempo;
  }
}

//ISR SALIDA 

void IRAM_ATTR isrSalida() {

  unsigned long tiempo = millis();

  if(tiempo - lastInterruptSalida > debounce) {

    flagSalida = true;

    lastInterruptSalida = tiempo;
  }
}

// TIMER ISR 


void IRAM_ATTR onTimer() {

  // APAGAR DISPLAYS
  // EVITA GHOSTING

  digitalWrite(digitPins[0], HIGH);
  digitalWrite(digitPins[1], HIGH);
  digitalWrite(digitPins[2], HIGH);

  // SELECCIONAR VALOR

  int valor;

  if(currentDigit == 0) {

    valor = centenas;
  }

  else if(currentDigit == 1) {

    valor = decenas;
  }

  else {

    valor = unidades;
  }

  // CARGAR SEGMENTOS

  for(int i = 0; i < 7; i++) {

    digitalWrite(segPins[i], numbers[valor][i]);
  }

  // ACTIVAR DISPLAY

  digitalWrite(digitPins[currentDigit], LOW);

  // SIGUIENTE DISPLAY

  currentDigit++;

  if(currentDigit > 2) {

    currentDigit = 0;
  }
}


void setup() {

  Serial.begin(115200);

  Serial.println(" SISTEMA PARQUEADERO INICIADO ");
  Serial.println(" TIMER + ISR + MULTIPLEXADO ");

  // SEGMENTOS

  for(int i = 0; i < 7; i++) {

    pinMode(segPins[i], OUTPUT);

    digitalWrite(segPins[i], LOW);
  }

  // DISPLAYS

  for(int i = 0; i < 3; i++) {

    pinMode(digitPins[i], OUTPUT);

    digitalWrite(digitPins[i], HIGH);
  }

  // BOTONES

  pinMode(btnEntrada, INPUT_PULLUP);
  pinMode(btnSalida, INPUT_PULLUP);

  pinMode(swEntrada, INPUT_PULLUP);
  pinMode(swSalida, INPUT_PULLUP);

  // INTERRUPCIONES BOTONES

  attachInterrupt(digitalPinToInterrupt(btnEntrada),
                  isrEntrada,
                  FALLING);

  attachInterrupt(digitalPinToInterrupt(btnSalida),
                  isrSalida,
                  FALLING);

  // PWM SERVO

  ledcAttach(servoPin, 50, 8);

  ledcWrite(servoPin, servoClose);

  // TIMER HARDWARE

  timer = timerBegin(1000000);

  timerAttachInterrupt(timer, &onTimer);

  timerAlarm(timer, 1000, true, 0);

  actualizarDigitos();

  Serial.println("Timer ISR iniciado");
  Serial.println("Refresh = 1 ms");
  Serial.println("Frecuencia total = 333 Hz");
}


void loop() {

  // DEBUG

  if(millis() - lastDebug > 3000) {

    lastDebug = millis();

    Serial.println("Barrido activo por TIMER ISR");
    Serial.print("Espacios disponibles: ");
    Serial.println(espacios);
  }

  // LEER SWITCHES

  bool entradaHabilitada = digitalRead(swEntrada);

  bool salidaHabilitada = digitalRead(swSalida);

  // EVENTO ENTRADA

  if(flagEntrada) {

    flagEntrada = false;

    Serial.println("INTERRUPCION ENTRADA");

    if(entradaHabilitada == HIGH) {

      Serial.println("Entrada habilitada");

      if(espacios > 0) {

        espacios--;

        actualizarDigitos();

        Serial.print("Vehiculo INGRESO -> ");
        Serial.println(espacios);

        abrirServo();
      }

      else {

        Serial.println("PARQUEADERO LLENO");
      }
    }

    else {

      Serial.println("Entrada BLOQUEADA");
    }
  }
  // EVENTO SALIDA

  if(flagSalida) {

    flagSalida = false;

    Serial.println("INTERRUPCION SALIDA");

    if(salidaHabilitada == HIGH) {

      Serial.println("Salida habilitada");

      if(espacios < 250) {

        espacios++;

        actualizarDigitos();

        Serial.print("Vehiculo SALIO -> ");
        Serial.println(espacios);

        abrirServo();
      }

      else {

        Serial.println("Parqueadero Disponible");
      }
    }

    else {

      Serial.println("Salida BLOQUEADA");
    }
  }
  // CONTROL SERVO NO BLOQUEANTE
  if(servoActivo) {

    if(millis() - servoStart >= servoTiempoAbierto) {

      cerrarServo();
    }
  }
}