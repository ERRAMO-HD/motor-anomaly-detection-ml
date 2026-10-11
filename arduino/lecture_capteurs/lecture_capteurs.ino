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

// Accéléromètre
float currentAccelX = 0.0;
float currentAccelY = 0.0;
float currentAccelZ = 0.0;

// Gyroscope
float currentGyroX = 0.0;
float currentGyroY = 0.0;
float currentGyroZ = 0.0;

bool vibrationAnomalyMode = false;
bool thermalAnomalyMode = false;

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
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};

unsigned long previousMotorMillis = 0;
long motorStepDelay = 2;

// ====================================================
// SETUP
// ====================================================
void setup() {
  Serial.begin(9600);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  Wire.begin();
  dht.begin();

  if (!mpu.begin()) {
    Serial.println("ERREUR_MPU");
    while (1);
  }

  Serial.println("SYSTEM_READY");
  Serial.println("FORMAT: ax,ay,az,gx,gy,gz,temp");
}

// ====================================================
// LOOP
// ====================================================
void loop() {
  unsigned long currentMillis = millis();

  checkSerialCommand();

  // Rotation continue moteur
  if (currentMillis - previousMotorMillis >= motorStepDelay) {
    previousMotorMillis = currentMillis;
    moveOneStep(1);
  }

  // Lecture MPU6050
  if (currentMillis - previousMPUMillis >= intervalMPU) {
    previousMPUMillis = currentMillis;
    readMPU();
    newMPUData = true;
  }

  // Lecture DHT11
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
// COMMANDES SÉRIE
// ====================================================
void checkSerialCommand() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'A' || cmd == 'a') vibrationAnomalyMode = !vibrationAnomalyMode;
    if (cmd == 'B' || cmd == 'b') thermalAnomalyMode = !thermalAnomalyMode;
    while (Serial.available()) Serial.read();
  }
}

// ====================================================
// CAPTEURS
// ====================================================
void readDHT() {
  float t = dht.readTemperature();
  if (!isnan(t)) currentTemp = t;

  if (thermalAnomalyMode) {
    currentTemp += 5.0;
  }
}

void readMPU() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Accéléromètre (vibrations)
  currentAccelX = a.acceleration.x;
  currentAccelY = a.acceleration.y;
  currentAccelZ = a.acceleration.z;

  // Gyroscope (vitesse angulaire)
  currentGyroX = g.gyro.x;
  currentGyroY = g.gyro.y;
  currentGyroZ = g.gyro.z;

  // Simulation d’anomalie vibratoire
  if (vibrationAnomalyMode) {
    currentAccelX += random(-20,20) / 100.0;
    currentAccelY += random(-20,20) / 100.0;
    currentGyroZ  += random(-10,10) / 100.0;
  }
}

// ====================================================
// MOTEUR PAS À PAS
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
  Serial.print(currentAccelX, 3); Serial.print(",");
  Serial.print(currentAccelY, 3); Serial.print(",");
  Serial.print(currentAccelZ, 3); Serial.print(",");

  Serial.print(currentGyroX, 3); Serial.print(",");
  Serial.print(currentGyroY, 3); Serial.print(",");
  Serial.print(currentGyroZ, 3); Serial.print(",");

  Serial.println(currentTemp, 1);

}

