from multiprocessing import Pipe
import signal
import threading
import socket
import matplotlib.pyplot as plt
import matplotlib
import numpy as np

class FreeEEG:
    __slots__ = ('__udp', '__remote_address', '__local_address',
                 '__pipe_in', '__pipe_out', 
                 '__filler_thread', '__active', 'N_ELECTRODE')
    def __init__(self, remote_ip = "192.168.4.1", remote_port = 69, local_ip = "", local_port = 6969) -> None:
        self.N_ELECTRODE = 8

        self.__remote_address = (remote_ip, remote_port)
        self.__local_address = (local_ip, local_port)

        self.__udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.__udp.bind(self.__local_address)

        (self.__pipe_in, self.__pipe_out) = Pipe()

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


    # Packet structure:
    # [absolute timestamp (40 bits)][# of EEG measurements (8 bits)]([relative timestamp (16 bits)][eeg data for each channel (24 bits)]*)*[# of IMU measurements (8 bits)]([relative timestamp (16 bits)][IMU data (96 bits)])*[# of events (8 bits)]([relative timestamp (16 bits)][event ID (8 bits)])*
    def __parse_packet(self, packet):
        raise RuntimeError("__parse_electrode_packet: Not implemented")

    def __stream_filler(self):
        while self.__active:
            packet = self.__udp.recv(4)
            self.__pipe_in.send(int.from_bytes(packet, byteorder='little'))

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


if __name__ == '__main__':
    import signal
    from collections import deque

    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation
    BUFFER_SIZE = 1000
    eeg = FreeEEG()  # FreeEEG("127.0.0.1", 4321)

    data_stream = eeg.get_data_generator(blocking=False)
    generator = data_stream()

    samples = deque(maxlen=BUFFER_SIZE)

    def signal_handler(sig, frame):
        eeg.stop()
        plt.close('all')
        exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    fig, ax = plt.subplots(figsize=(10, 5))
    line, = ax.plot([], [], label='EEG channel')

    ax.set_title('FreeEEG real-time EEG data')
    ax.set_xlabel('Samples')
    ax.set_ylabel('ADC value')
    ax.grid(True)
    ax.legend(loc='upper right')

    def update(frame):
        try:
            value = next(generator)
        except StopIteration:
            return line,
        except Exception:
            return line,

        if value < 1e9:
            return line,

        samples.append(value)

        x = range(len(samples))
        line.set_data(x, samples)

        ax.set_xlim(0, BUFFER_SIZE)

        if samples:
            ymin = min(samples)
            ymax = max(samples)
            margin = 0.1 * (ymax - ymin) if ymax != ymin else 1
            ax.set_ylim(ymin - margin, ymax + margin)

        return line,

    eeg.start()
    print("Starting EEG...")

    animation = FuncAnimation(fig, update, interval=20, blit=False)
    plt.show()

    eeg.stop()

# if __name__ == '__main__':
#     eeg = FreeEEG() # FreeEEG("127.0.0.1", 4321)

#     def signal_handler(sig, frame):
#         # global f
#         # f.close()
#         eeg.stop()
#         exit(0)

#     signal.signal(signal.SIGINT, signal_handler)
    
#     data_stream = eeg.get_data_generator(blocking=True)

#     # f = open("test.csv", "w+")
#     eeg.start()
#     print("Starting EEG...")
#     # f.write("timestamp,e1,e2,e3,e4,e5,e6,e7,e8,ax,ay,az,gx,gy,gz,event\n")
#     for i in data_stream():
#         # f.write(i.as_csv() + "\n")

#         print(i)
