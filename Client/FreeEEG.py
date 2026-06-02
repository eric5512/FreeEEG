from multiprocessing import Pipe
import threading
import socket
import struct
from dataclasses import dataclass

class FreeEEG:
    __slots__ = ('__udp', '__remote_address', '__local_address',
                 '__pipe_in', '__pipe_out', 
                 '__filler_thread', '__active',
                 '__STRUCT_FMT', '__STRUCT_SIZE',
                 '__ELEC_BUFF_SIZE', '__IMU_BUFF_SIZE', '__EVENT_BUFF_SIZE',
                 '__unpack')

    FSR = 5.0
    N_ELECTRODE = 8

    def __init__(self, remote_ip = "192.168.4.1", remote_port = 69, local_ip = "", local_port = 6969) -> None:    
        self.__ELEC_BUFF_SIZE =  30
        self.__IMU_BUFF_SIZE =   5
        self.__EVENT_BUFF_SIZE = 2
        self.__STRUCT_FMT = (
            "<"
            "I"      # abs_timestamp
            "B"      # num_elec
            f"{self.__ELEC_BUFF_SIZE}I"
            f"{self.__ELEC_BUFF_SIZE * 8}I"
            "B"      # num_imu
            f"{self.__IMU_BUFF_SIZE}I"
            f"{self.__IMU_BUFF_SIZE * 3}H"
            f"{self.__IMU_BUFF_SIZE * 3}H"
            "B"      # num_events
            f"{self.__EVENT_BUFF_SIZE}I"
            f"{self.__EVENT_BUFF_SIZE}B"
        )

        self.__STRUCT_SIZE = struct.calcsize(self.__STRUCT_FMT)
        self.__unpack = struct.Struct(self.__STRUCT_FMT).unpack_from

        self.__remote_address = (remote_ip, remote_port)
        self.__local_address = (local_ip, local_port)

        self.__udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.__udp.bind(self.__local_address)

        (self.__pipe_in, self.__pipe_out) = Pipe()

    @dataclass
    class Packet:
        abs_time: int
        elec_time: list[int]
        elec_data: list[list[int]]
        imu_data: list[list[int]]
        event_time: list[int]


    def __parse_packet(self, data):
        offset = 0
        
        abs_timestamp = data[offset]
        offset += 1

        n_elec = data[offset]
        offset += 1

        elec_timestamps = data[offset:offset + self.__ELEC_BUFF_SIZE]
        offset += self.__ELEC_BUFF_SIZE

        elec_data_flat = data[offset:offset + self.__ELEC_BUFF_SIZE * self.N_ELECTRODE]
        offset += self.__ELEC_BUFF_SIZE * self.N_ELECTRODE

        n_imu = data[offset]
        offset += 1

        imu_timestamps = data[offset:offset + self.__IMU_BUFF_SIZE]
        offset += self.__IMU_BUFF_SIZE

        accel_data_flat = data[offset:offset + self.__IMU_BUFF_SIZE * 3]
        offset += self.__IMU_BUFF_SIZE * 3
        
        gyro_data_flat = data[offset:offset + self.__IMU_BUFF_SIZE * 3]
        offset += self.__IMU_BUFF_SIZE * 3
        
        n_events = data[offset]
        offset += 1
        
        event_timestamps = data[offset:offset + self.__EVENT_BUFF_SIZE]
        offset += self.__EVENT_BUFF_SIZE
        
        events = data[offset:offset + self.__EVENT_BUFF_SIZE]
        
        elec_data = [
            elec_data_flat[i * self.N_ELECTRODE:(i + 1) * self.N_ELECTRODE]
            for i in range(n_elec)
        ]

        imu_data = [
        (
            imu_timestamps[i],
            accel_data_flat[i * 3:(i + 1) * 3],
            gyro_data_flat[i * 3:(i + 1) * 3],
        ) for i in range(n_imu)]
        
        event_data = [
            (event_timestamps[i], events[i])
            for i in range(n_events)
        ]

        return self.Packet(abs_timestamp, elec_timestamps[:n_elec], elec_data, imu_data, event_data)

    def get_data_generator(self, blocking: bool = False):
        def block():
            while True:
                yield self.__pipe_out.recv()

        def nonblock():
            while True:
                if self.__pipe_out.poll():
                    yield self.__pipe_out.recv()
                else:
                    yield None

        return block if blocking else nonblock

    def __stream_filler(self):
        while self.__active:
            packet = self.__udp.recv(self.__STRUCT_SIZE)
            self.__pipe_in.send(self.__parse_packet(self.__unpack(packet)))

    def start(self):
        self.__udp.sendto("start".encode(), self.__remote_address)
        self.__filler_thread = threading.Thread(target=self.__stream_filler)
        self.__active = True
        self.__filler_thread.start()

    def stop(self):
        self.__active = False
        self.__udp.sendto("stop".encode(), self.__remote_address)
        eeg.__udp.close()
        self.__filler_thread.join()

    def adc_to_volts(value):
        return (value if value >= 0x800000 else value - 0xFFFFFF) * FreeEEG.FSR / 0xFFFFFF

if __name__ == '__main__':
    import signal
    from collections import deque

    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation


    BUFFER_SIZE = 1000
    CHANNELS_TO_PLOT = [0]  # Add more channels later, e.g. [0, 1, 2, 3]
    
    eeg = FreeEEG()

    data_stream = eeg.get_data_generator(blocking=False)
    generator = data_stream()

    time_data = deque(maxlen=BUFFER_SIZE)
    channel_data = {
        ch: deque(maxlen=BUFFER_SIZE)
        for ch in CHANNELS_TO_PLOT
    }

    t0 = None

    def signal_handler(sig, frame):
        eeg.stop()
        plt.close('all')
        exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    fig, ax = plt.subplots(figsize=(10, 5))

    lines = {}
    for ch in CHANNELS_TO_PLOT:
        line, = ax.plot([], [], label=f'EEG channel {ch + 1}')
        lines[ch] = line

    ax.set_title('FreeEEG real-time EEG data')
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Voltage (V)')
    ax.grid(True)
    ax.legend(loc='upper right')

    def update(frame):
        global t0

        packet = next(generator)

        if packet is None:
            return tuple(lines.values())

        for sample_idx, rel_timestamp in enumerate(packet.elec_time):
            timestamp = packet.abs_time + rel_timestamp / 1e6

            if t0 is None:
                t0 = timestamp

            time_s = (timestamp - t0)
            time_data.append(time_s)

            for ch in CHANNELS_TO_PLOT:
                adc_value = packet.elec_data[sample_idx][ch]
                volts = FreeEEG.adc_to_volts(adc_value)
                channel_data[ch].append(volts)

        for ch in CHANNELS_TO_PLOT:
            lines[ch].set_data(time_data, channel_data[ch])

        if len(time_data) >= 2:
            ax.set_xlim(time_data[0], time_data[-1])

        all_values = [
            value
            for ch in CHANNELS_TO_PLOT
            for value in channel_data[ch]
        ]

        if all_values:
            ymin = min(all_values)
            ymax = max(all_values)
            margin = 0.1 * (ymax - ymin) if ymax != ymin else 1e-6
            ax.set_ylim(ymin - margin, ymax + margin)

        return tuple(lines.values())

    eeg.start()
    print("Starting EEG...")

    animation = FuncAnimation(fig, update, interval=20, blit=False)
    plt.show()

    eeg.stop()

# if __name__ == '__main__':
#     import time
#     import signal
#     import numpy as np
#     eeg = FreeEEG() # FreeEEG("127.0.0.1", 4321)

#     def signal_handler(sig, frame):
#         # global f
#         # f.close()
#         global latencies
#         latencies = np.array(sorted(latencies))
#         diffs = (1000*(latencies[1:] - latencies[:-1])).astype(int)
#         with open("data.txt", "w+") as f:
#             f.write("\n".join([f"{i},{j}" for i, j in zip(diffs, latencies[1:])]))
#         time_diff = time.time() - beg
#         print(f"Throughput: {count} packets in {time_diff} seconds; {count/time_diff} packets/s")
#         eeg.stop()
#         exit(0)

#     signal.signal(signal.SIGINT, signal_handler)
    
#     data_stream = eeg.get_data_generator(blocking=True)

#     # f = open("test.csv", "w+")
#     eeg.start()
#     print("Starting EEG...")
#     # f.write("timestamp,e1,e2,e3,e4,e5,e6,e7,e8,ax,ay,az,gx,gy,gz,event\n")
#     latencies = []
#     beg = time.time()
#     count = 0
#     for i in data_stream():
#         # f.write(i.as_csv() + "\n")
#         for j in i.elec_time:
#             count += 1 
#             time_ = j / 1e6 + i.abs_time
#             latencies.append(time_)
