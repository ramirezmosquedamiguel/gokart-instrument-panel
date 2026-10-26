#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
// =====================================================
// CONFIGURACION GENERAL
// =====================================================
// ---------------- SENSOR HALL DELANTERO ----------------
const byte HALL_PIN = 2;          // Pin con interrupcion
const int IMANES = 4;             // 4 imanes en la rueda
volatile unsigned long pulsosHall = 0;
// ---------------- BOTON RESET ----------------
const byte RESET_PIN = 4;

// ---------------- SERVOMOTOR TACOMETRO ----------------
const byte SERVO_PIN = 9;
Servo tacometro;
// ---------------- LCD I2C ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
// =====================================================
// DATOS DE LA RUEDA
// =====================================================
// Diametro aproximado rueda delantera = 22 cm
// Circunferencia aproximada = 0.693 m
const float CIRCUNFERENCIA_RUEDA = 0.693;
// Cada vuelta produce 4 pulsos
const float DISTANCIA_POR_PULSO =
  CIRCUNFERENCIA_RUEDA / IMANES;
// =====================================================
// RELACION MOTOR / RUEDA
// =====================================================
// Relacion aproximada utilizada para estimar RPM motor
const float RELACION_MOTOR_RUEDA = 71.79;
// =====================================================
// TACOMETRO
// =====================================================
// Ajustar posteriormente segun la caratula fisica
const float RPM_MOTOR_MAX = 25000.0;
const int ANGULO_MIN = 0;
const int ANGULO_MAX = 180;
// =====================================================
// VARIABLES
// =====================================================
unsigned long tiempoAnterior = 0;
unsigned long pulsosAnteriores = 0;
float rpmRueda = 0;
float rpmMotor = 0;
float distanciaMetros = 0;
bool botonAnterior = HIGH;
// =====================================================
// INTERRUPCION DEL SENSOR HALL
// =====================================================
void contarPulsoHall() {
  pulsosHall++;
}
void setup() {
  Serial.begin(115200);
  // ---------------- HALL ----------------
  pinMode(HALL_PIN, INPUT_PULLUP);
  attachInterrupt(
    digitalPinToInterrupt(HALL_PIN),
    contarPulsoHall,
    FALLING
  );
  // ---------------- BOTON RESET ----------------
  pinMode(RESET_PIN, INPUT_PULLUP);

  // ---------------- SERVO ----------------
  tacometro.attach(SERVO_PIN);
  tacometro.write(ANGULO_MIN);
  // ---------------- LCD ----------------
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Panel Go-Kart");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(1500);
  lcd.clear();
  tiempoAnterior = millis();
  Serial.println("Sistema iniciado");
}
void loop() {
  unsigned long tiempoActual = millis();
  // BOTON DE RESET
  bool estadoBoton = digitalRead(RESET_PIN);
  // Detectar cuando se presiona
  if (botonAnterior == HIGH &&
      estadoBoton == LOW) {
    noInterrupts();
    pulsosHall = 0;
    pulsosAnteriores = 0;
    interrupts();
    distanciaMetros = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Distancia");
    lcd.setCursor(0, 1);
    lcd.print("Reiniciada");
    delay(500);
  }
  botonAnterior = estadoBoton;
  // CALCULO CADA 1 SEGUNDO
  if (tiempoActual - tiempoAnterior >= 1000) {
    float deltaTiempo =
      (tiempoActual - tiempoAnterior) / 1000.0;
    tiempoAnterior = tiempoActual;
    // COPIAR PULSOS
    noInterrupts();
    unsigned long pulsosActuales = pulsosHall;
    interrupts();
    // Pulsos generados durante el ultimo intervalo
    unsigned long pulsosPeriodo =
      pulsosActuales - pulsosAnteriores;
    pulsosAnteriores = pulsosActuales;
    // RPM DE LA RUEDA
    rpmRueda =
      (pulsosPeriodo * 60.0) /
      (IMANES * deltaTiempo);
    // RPM APROXIMADAS DEL MOTOR
    rpmMotor =
      rpmRueda * RELACION_MOTOR_RUEDA;
    // DISTANCIA TOTAL
    distanciaMetros =
      pulsosActuales * DISTANCIA_POR_PULSO;
    // SERVOMOTOR TACOMETRO
    int anguloServo =
      map(
        (long)rpmMotor,
        0,
        (long)RPM_MOTOR_MAX,
        ANGULO_MIN,
        ANGULO_MAX
      );
    anguloServo =
      constrain(
        anguloServo,
        ANGULO_MIN,
        ANGULO_MAX
      );
    tacometro.write(anguloServo);
    // LCD
    lcd.setCursor(0, 0);
    lcd.print("Distancia:      ");
    lcd.setCursor(0, 1);
    lcd.print(distanciaMetros, 2);
    lcd.print(" m        ");
    // MONITOR SERIE
    Serial.print("Pulsos: ");
    Serial.print(pulsosActuales);
    Serial.print(" | RPM rueda: ");
    Serial.print(rpmRueda, 1);
    Serial.print(" | RPM motor aprox: ");
    Serial.print(rpmMotor, 0);
    Serial.print(" | Distancia: ");
    Serial.print(distanciaMetros, 2);
    Serial.println(" m");
  }
}