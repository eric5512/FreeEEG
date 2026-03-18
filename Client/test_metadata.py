import pandas as pd
from datetime import datetime

with open("test_metadata.csv") as f:
    date      = datetime.strptime(":".join(next(f).split(":")[1:]).strip(), "%Y-%m-%d %H:%M:%S")
    active_ch = [i.strip() for i in next(f).split(":")[1].split(",")]
    eeg_dr    = int(next(f).split(":")[1])
    imu_dr    = int(next(f).split(":")[1])
    df = pd.read_csv(f)

print(f"Date: {date}")
print(f"Active channels: {active_ch}")
print(f"EEG data rate: {eeg_dr}")
print(f"IMU data rate: {imu_dr}")
print(df)
