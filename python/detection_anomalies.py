import os
import serial
import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler
from sklearn.svm import OneClassSVM
import time
import math

# === AJOUT GRAPHE SEULEMENT ===
import matplotlib.pyplot as plt
from collections import deque

# ==============================
# PARAMÈTRES
# ==============================
PORT = "COM5"
BAUDRATE = 9600

WINDOW_SIZE = 10
ALARM_THRESHOLD = 40
FEATURES = ["vibration_rms"]

# ==============================
# CHEMINS
# ==============================
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "..", "data")
DATA_FILE = os.path.join(DATA_DIR, "dataset_normal.csv")

# ==============================
# 1️⃣ APPRENTISSAGE NORMAL
# ==============================
print("Chargement des données normales...")

df_train = pd.read_csv(DATA_FILE)
X_train = df_train[FEATURES]

scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)

model = OneClassSVM(
    kernel="rbf",
    nu=0.1,
    gamma="scale"
)

model.fit(X_train_scaled)
print("Modèle entraîné sur moteur NORMAL")

# ==============================
# 2️⃣ INITIALISATION GRAPHE
# ==============================
plt.ion()
fig, ax = plt.subplots(figsize=(9, 4))

history = deque(maxlen=200)
history_pred = deque(maxlen=200)

# ==============================
# 3️⃣ DÉTECTION TEMPS RÉEL
# ==============================
print("\n--- DÉTECTION TEMPS RÉEL DÉMARRÉE ---")
print("CTRL + C pour arrêter\n")

ser = serial.Serial(PORT, BAUDRATE, timeout=1)
time.sleep(2)

buffer = []

try:
    while True:
        line = ser.readline().decode(errors="ignore").strip()
        if not line:
            continue

        parts = line.split(",")

        # Arduino : ax, ay, az, gx, gy, gz, temp
        if len(parts) != 7:
            continue

        try:
            ax_, ay_, az_, _, _, _, _ = map(float, parts)

            # ==============================
            # CALCUL VIBRATION RMS
            # ==============================
            vibration_rms = math.sqrt((ax_**2 + ay_**2 + az_**2) / 3)
            buffer.append([vibration_rms])

            # Ignorer phase démarrage
            if len(buffer) < WINDOW_SIZE * 2:
                continue

            df_window = pd.DataFrame(
                buffer[-WINDOW_SIZE:],
                columns=FEATURES
            )

            X_window_scaled = scaler.transform(df_window)
            preds = model.predict(X_window_scaled)

            anomaly_rate = (preds == -1).mean() * 100
            last_pred = preds[-1]

            # ==============================
            # DÉCISION + 🔊 SON
            # ==============================
            if anomaly_rate > ALARM_THRESHOLD:
                print(f"🚨 MOTEUR ANORMAL | Anomalies = {anomaly_rate:.1f}%")
                try:
                    import winsound
                    winsound.Beep(1500, 400)
                except:
                    pass
            else:
                print(f"✅ MOTEUR NORMAL | Anomalies = {anomaly_rate:.1f}%")

            # ==============================
            # GRAPHE TEMPS RÉEL
            # ==============================
            history.append(vibration_rms)
            history_pred.append(last_pred)

            ax.clear()

            idx = np.arange(len(history))
            normal_idx = [i for i, p in enumerate(history_pred) if p == 1]
            anomaly_idx = [i for i, p in enumerate(history_pred) if p == -1]

            ax.plot(idx, history, alpha=0.3, label="Vibration RMS")
            ax.scatter(
                normal_idx,
                [history[i] for i in normal_idx],
                color="green",
                s=20,
                label="Normal"
            )
            ax.scatter(
                anomaly_idx,
                [history[i] for i in anomaly_idx],
                color="red",
                s=30,
                label="Anomalie"
            )

            ax.set_title("One-Class SVM — Surveillance moteur")
            ax.set_xlabel("Temps")
            ax.set_ylabel("Vibration RMS")
            ax.legend()
            ax.grid(True)

            plt.pause(0.01)

        except ValueError:
            continue

except KeyboardInterrupt:
    print("\nArrêt de la surveillance.")

finally:
    ser.close()
    plt.ioff()
    plt.show()
    print("Port série fermé.")
