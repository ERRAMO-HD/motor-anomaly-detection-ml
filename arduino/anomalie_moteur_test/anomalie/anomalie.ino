// ====================================================
// LIBRAIRIES
// ====================================================
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>

// ====================================================
// VARIABLES GLOBALES
// ====================================================
float currentTemp = 25.0;
float currentAccelX = 0.0;
float currentAccelY = 0.0;
float currentAccelZ = 0.0;

// Modes d'anomalie
bool vibrationAnomalyMode = true; // Activé
bool thermalAnomalyMode = true;   // Activé
bool motorStopMode = false;

// ====================================================
// DHT11
// ====================================================
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
const unsigned long intervalDHT = 3000;
unsigned long previousDHTMillis = 0;
bool newDHTData = false;

// ====================================================
// MPU6050
// ====================================================
Adafruit_MPU6050 mpu;
const unsigned long intervalMPU = 100;
unsigned long previousMPUMillis = 0;
bool newMPUData = false;

// ====================================================
// MOTEUR PAS À PAS (ULN2003)
// ====================================================
#define IN1 8
#define IN2 9
#define IN3 10
#define IN4 11

int stepIndex = 0;
int steps[8][4] = {
  {1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},
  {0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}
};

unsigned long previousMotorMillis = 0;
long motorStepDelay = 1;   // ⚠️ très rapide pour anomalie

// ====================================================
// SETUP
// ====================================================
void setup() {
  Serial.begin(9600);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  dht.begin();
  Wire.begin();

  if (!mpu.begin()) {
    Serial.println("ERREUR_MPU");
    while (1);
  }

  Serial.println("SYSTEM_READY_ANORMAL");
  Serial.println("FORMAT: accel_x,accel_y,accel_z,temperature");
}

// ====================================================
// LOOP
// ====================================================
void loop() {
  unsigned long currentMillis = millis();

  // 🔁 MOTEUR : rotation avec anomalie
  if (!motorStopMode && currentMillis - previousMotorMillis >= motorStepDelay) {
    previousMotorMillis = currentMillis;
    moveOneStep((random(2) == 0) ? 1 : -1);  // direction aléatoire
  }

  // MPU6050
  if (currentMillis - previousMPUMillis >= intervalMPU) {
    previousMPUMillis = currentMillis;
    readMPU();
    newMPUData = true;
  }

  // DHT11
  if (currentMillis - previousDHTMillis >= intervalDHT) {
    previousDHTMillis = currentMillis;
    readDHT();
    newDHTData = true;
  }

  // Envoi CSV synchronisé
  if (newMPUData && newDHTData) {
    sendCSV();
    newMPUData = false;
    newDHTData = false;
  }
}

// ====================================================
// CAPTEURS
// ====================================================
void readDHT() {
  float t = dht.readTemperature();
  if (!isnan(t)) currentTemp = t;

  if (thermalAnomalyMode) {
    currentTemp += random(5, 15); // température artificiellement haute
  }
}

void readMPU() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  currentAccelX = a.acceleration.x;
  currentAccelY = a.acceleration.y;
  currentAccelZ = a.acceleration.z;

  if (vibrationAnomalyMode) {
    // ajouter variations aléatoires importantes pour simuler anomalie
    currentAccelX += random(-50,50) / 10.0;
    currentAccelY += random(-50,50) / 10.0;
    currentAccelZ += random(-50,50) / 10.0;
  }
}

// ====================================================
// MOTEUR
// ====================================================
void moveOneStep(int direction) {
  digitalWrite(IN1, steps[stepIndex][0]);
  digitalWrite(IN2, steps[stepIndex][1]);
  digitalWrite(IN3, steps[stepIndex][2]);
  digitalWrite(IN4, steps[stepIndex][3]);

  stepIndex += direction;
  if (stepIndex > 7) stepIndex = 0;
  if (stepIndex < 0) stepIndex = 7;
}

// ====================================================
// SORTIE CSV
// ====================================================
void sendCSV() {
  Serial.print(currentAccelX, 3);
  Serial.print(",");
  Serial.print(currentAccelY, 3);
  Serial.print(",");
  Serial.print(currentAccelZ, 3);
  Serial.print(",");
  Serial.println(currentTemp, 1);
}

