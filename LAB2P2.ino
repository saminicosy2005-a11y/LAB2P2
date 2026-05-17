

// SEGMENTOS

// a b c d e f g
const int segPins[7] = {13, 12, 14, 27, 26, 25, 33}; // PINES DE SEGMENTOS COMPARTIDOS

// Displays ánodo común
const int digitPins[3] = {18, 19, 21}; // CONTROL DE CENTENAS, DECENAS Y UNIDADES

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

//  BOTONES 

const int btnEntrada = 4;
const int btnSalida  = 5;

const int swEntrada = 15;
const int swSalida  = 2;

//  SERVO 
const int servoPin = 22;

// PWM servo
const int servoClose = 20;  // DUTY PWM PARA 0°
const int servoOpen  = 110; // DUTY PWM PARA 90°

// TIMER


hw_timer_t *timer = NULL; // TIMER DE HARDWARE DEL ESP32

//  VARIABLES


// Espacios disponibles
volatile int espacios = 250; // VARIABLE USADA EN ISR

// Barrido
volatile int currentDigit = 0; // DISPLAY ACTUAL DEL BARRIDO

// Dígitos
volatile int centenas = 2;
volatile int decenas  = 5;
volatile int unidades = 0;

// Anti rebote
unsigned long lastEntrada = 0;
unsigned long lastSalida  = 0;

const int debounce = 250; // TIEMPO ANTI REBOTE

// Servo no bloqueante
bool servoActivo = false;

unsigned long servoStart = 0;

const unsigned long servoTiempoAbierto = 3000; // TIEMPO DE APERTURA DEL SERVO

// Debug timer barrido
unsigned long lastDebug = 0;


//  ACTUALIZAR DÍGITOS DISPLAY 


void actualizarDigitos() {

  centenas = espacios / 100;       // OBTENER CENTENAS

  decenas = (espacios / 10) % 10;  // OBTENER DECENAS

  unidades = espacios % 10;        // OBTENER UNIDADES
}


//  ABRIR SERVO 


void abrirServo() {

  ledcWrite(servoPin, servoOpen); // ENVIAR PWM DE APERTURA

  servoActivo = true;

  servoStart = millis(); // INICIAR TEMPORIZADOR

  Serial.println("Servo ABIERTO");
}

// CERRAR SERVO 


void cerrarServo() {

  ledcWrite(servoPin, servoClose); // ENVIAR PWM DE CIERRE

  servoActivo = false;

  Serial.println("Servo CERRADO");
}

//  TIMER ISR 

void IRAM_ATTR onTimer() {

  // APAGAR TODOS LOS DISPLAYS
  // EVITA GHOSTING ENTRE TRANSICIONES

  digitalWrite(digitPins[0], HIGH);
  digitalWrite(digitPins[1], HIGH);
  digitalWrite(digitPins[2], HIGH);

  // SELECCIONAR DÍGITO A MOSTRAR

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

  // ACTUALIZAR SEGMENTOS

  for(int i = 0; i < 7; i++) {

    digitalWrite(segPins[i], numbers[valor][i]);
  }

  // ACTIVAR DISPLAY CORRESPONDIENTE

  digitalWrite(digitPins[currentDigit], LOW);

  // CAMBIAR AL SIGUIENTE DISPLAY

  currentDigit++;

  if(currentDigit > 2) {

    currentDigit = 0;
  }
}



void setup() {

  Serial.begin(115200); // INICIALIZAR SERIAL

  Serial.println(" SISTEMA PARQUEADERO INICIADO ");
  Serial.println(" BARRIDO POR TIMER ISR ");

  // CONFIGURAR SEGMENTOS COMO SALIDAS

  for(int i = 0; i < 7; i++) {

    pinMode(segPins[i], OUTPUT);

    digitalWrite(segPins[i], LOW);
  }

  // CONFIGURAR DISPLAYS COMO SALIDAS

  for(int i = 0; i < 3; i++) {

    pinMode(digitPins[i], OUTPUT);

    digitalWrite(digitPins[i], HIGH); // DISPLAY APAGADO
  }

  // CONFIGURAR BOTONES Y SWITCHES

  pinMode(btnEntrada, INPUT_PULLUP);
  pinMode(btnSalida, INPUT_PULLUP);

  pinMode(swEntrada, INPUT_PULLUP);
  pinMode(swSalida, INPUT_PULLUP);

  // CONFIGURAR PWM PARA SERVO

  ledcAttach(servoPin, 50, 8); // 50 Hz Y RESOLUCIÓN 8 BITS

  ledcWrite(servoPin, servoClose);

  // CONFIGURAR TIMER A 1 MHz

  timer = timerBegin(1000000);

  timerAttachInterrupt(timer, &onTimer); // ASOCIAR ISR

  timerAlarm(timer, 1000, true, 0); // INTERRUPCIÓN CADA 1 ms

  actualizarDigitos();

  Serial.println("Timer ISR iniciado");
  Serial.println("Refresh display = 1 ms por digito");
  Serial.println("Frecuencia total aproximada = 333 Hz");
}



void loop() {

  // DEBUG DEL SISTEMA

  if(millis() - lastDebug > 3000) {

    lastDebug = millis();

    Serial.println("Barrido activo mediante TIMER ISR");
    Serial.print("Espacios disponibles: ");
    Serial.println(espacios);
  }

  // LEER ESTADO DE SWITCHES

  bool entradaHabilitada = digitalRead(swEntrada);

  bool salidaHabilitada = digitalRead(swSalida);

  //  ENTRADA 
  

  if(digitalRead(btnEntrada) == LOW) {

    if(millis() - lastEntrada > debounce) { // ANTI REBOTE

      lastEntrada = millis();

      Serial.println("Boton ENTRADA detectado");

      if(entradaHabilitada == HIGH) {

        Serial.println("Entrada habilitada");

        if(espacios > 0) {

          espacios--;

          actualizarDigitos(); // ACTUALIZAR DISPLAY

          Serial.print("Vehiculo INGRESO -> Espacios: ");
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
  }

  // SALIDA 
 

  if(digitalRead(btnSalida) == LOW) {

    if(millis() - lastSalida > debounce) { // ANTI REBOTE

      lastSalida = millis();

      Serial.println("Boton SALIDA detectado");

      if(salidaHabilitada == HIGH) {

        Serial.println("Salida habilitada");

        if(espacios < 250) {

          espacios++;

          actualizarDigitos(); // ACTUALIZAR DISPLAY

          Serial.print("Vehiculo SALIO -> Espacios: ");
          Serial.println(espacios);

          abrirServo();
        }

        else {

          Serial.println("El parqueadero ya esta vacio");
        }
      }

      else {

        Serial.println("Salida BLOQUEADA");
      }
    }
  }

  // CONTROL NO BLOQUEANTE DEL SERVO

  if(servoActivo) {

    if(millis() - servoStart >= servoTiempoAbierto) {

      cerrarServo();
    }
  }
}