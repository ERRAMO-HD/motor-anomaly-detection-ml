import os
import serial
import csv
import time
import math

# ==============================
# PARAMÈTRES
# ==============================
PORT = "COM5"
BAUDRATE = 9600

# ==============================
# CHEMINS
# ==============================
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "..", "data")
os.makedirs(DATA_DIR, exist_ok=True)

CSV_FILE = os.path.join(DATA_DIR, "dataset_normal.csv")

# ==============================
# OUVERTURE SÉRIE
# ==============================
ser = serial.Serial(PORT, BAUDRATE, timeout=1)
time.sleep(2)
print("Connexion Arduino OK")

# ==============================
# ACQUISITION
# ==============================
try:
    with open(CSV_FILE, mode="w", newline="") as file:
        writer = csv.writer(file)

        # EN-TÊTE ML
        writer.writerow(["vibration_rms", "rotation_rms", "temperature"])

        print("Acquisition moteur NORMAL (CTRL+C pour arrêter)")

        while True:
            line = ser.readline().decode(errors="ignore").strip()
            if not line:
                continue

            # Arduino : ax, ay, az, gx, gy, gz, temp
            parts = line.split(",")
            if len(parts) != 7:
                continue

            try:
                ax, ay, az, gx, gy, gz, temp = map(float, parts)

                # ==============================
                # CALCUL RMS
                # ==============================
                vibration_rms = math.sqrt((ax**2 + ay**2 + az**2) / 3)
                rotation_rms  = math.sqrt((gx**2 + gy**2 + gz**2) / 3)

                # ==============================
                # ÉCRITURE CSV
                # ==============================
                writer.writerow([
                    round(vibration_rms, 3),
                    round(rotation_rms, 4),
                    round(temp, 2)
                ])
                file.flush()

                print(
                    f"Vib RMS={vibration_rms:.3f} | "
                    f"Rot RMS={rotation_rms:.4f} | "
                    f"Temp={temp:.1f}"
                )

            except ValueError:
                continue

except KeyboardInterrupt:
    print("\nArrêt acquisition.")

finally:
    ser.close()
    print("Port série fermé.")
