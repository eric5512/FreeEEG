import pandas as pd
from datetime import datetime

def read_csv(path):
    with open(path) as f: 
        aux = next(f)
        if aux[0] == '#':
            date      = datetime.strptime(":".join(aux.split(":")[1:]).strip(), "%Y-%m-%d %H:%M:%S")
            active_ch = [i.strip() for i in next(f).split(":")[1].split(",")]
            eeg_dr    = int(next(f).split(":")[1])
            imu_dr    = int(next(f).split(":")[1])
        else:
            date      = None
            active_ch = None
            eeg_dr    = None
            imu_dr    = None
            f.seek(0)
        df = pd.read_csv(f)
        return (date, active_ch, eeg_dr, imu_dr), df

print(f"Date: {date}")
print(f"Active channels: {active_ch}")
print(f"EEG data rate: {eeg_dr}")
print(f"IMU data rate: {imu_dr}")
print(df)
