from multiprocessing import Pipe
import signal
import threading
import socket
import matplotlib.pyplot as plt
import matplotlib
import numpy as np

class FreeEEG:
    __slots__ = ('__udp', '__remote_address', '__local_address', '__electrode_pipe_in', 
                 '__accelerometer_pipe_in', '__electrode_pipe_out', '__accelerometer_pipe_out', 
                 '__filler_thread', '__active', 'N_ELECTRODE')
    def __init__(self, remote_ip = "192.168.1.1", remote_port = 69, local_ip = "", local_port = 6969) -> None:
        self.N_ELECTRODE = 8

        self.__remote_address = (remote_ip, remote_port)
        self.__local_address = (local_ip, local_port)

        self.__udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.__udp.bind(self.__local_address)

        (self.__accelerometer_pipe_in, self.__accelerometer_pipe_out) = Pipe()
        self.__electrode_pipe_in = []
        self.__electrode_pipe_out = []
        for _ in range(self.N_ELECTRODE):
            a, b = Pipe()
            self.__electrode_pipe_in.append(a)
            self.__electrode_pipe_out.append(b)

    def get_electrode_generator(self, n_electrode: int, blocking: bool = False):
        assert n_electrode < self.N_ELECTRODE
        def block():
            while True:
                yield self.__electrode_pipe_out[n_electrode].recv()

        def nonblock():
            while True:
                if self.__electrode_pipe_out[n_electrode].poll():
                    yield self.__electrode_pipe_out[n_electrode].recv()
                else:
                    yield None

        return block if blocking else nonblock



    def get_accelerometer_generator(self, blocking: bool = False):
        def block():
            while True:
                yield self.__accelerometer_pipe_out.recv()
        def nonblock():
            while True:
                if self.__accelerometer_pipe_out.poll():
                    yield self.__accelerometer_pipe_out.recv()
                else:
                    yield None

        return block if blocking else nonblock

    # Packet types:
    # - Accelerometer [type (\x10) (4 bits)][ padding (4 bits)][value x (8 bits)][value y (8 bits)][value z (8 bits)]
    # - Electrode [type (\x20) (4 bits)][electrode ID (4 bits)][value (24 bits)]
    def __stream_filler(self):
        while self.__active:
            packet = self.__udp.recv(4)
            match packet[0] & 0xF0:
                case 0x10: # Accelerometer packet
                    self.__accelerometer_pipe_in.send((int(packet[1]), int(packet[2]), int(packet[3])))
                case 0x20: # Electrode packet
                    self.__electrode_pipe_in[(packet[0] & 0x0F)].send(int.from_bytes(packet[1:], "little"))

    def start(self):
        self.__udp.sendto("start".encode(), self.__remote_address)
        self.__filler_thread = threading.Thread(target=self.__stream_filler)
        self.__active = True
        self.__filler_thread.start()

    def stop(self):
        self.__udp.sendto("stop".encode(), self.__remote_address)
        self.__active = False
        eeg.__udp.close()
        self.__filler_thread.join()


if __name__ == '__main__':

    matplotlib.use("TkAgg")
    plt.ion()
    eeg = FreeEEG() # FreeEEG("127.0.0.1", 4321)

    def signal_handler(sig, frame):
        print(sig)
        global f
        f.close()
        eeg.stop()
        exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    # freq = 1000
    # buffer_size = freq * 2  # 2 seconds

    # # fixed x axis
    # x = np.arange(buffer_size)
    # data = np.zeros(buffer_size)

    # fig, ax = plt.subplots()
    # line, = ax.plot(x, data, color='g')
    # ax.set_ylim(-20000, 20000)   # fix y limits to avoid rescale cost
    # ax.set_xlim(0, buffer_size)

    # plt.ion()
    # plt.show(block=False)
    # fig.canvas.draw()
    # background = fig.canvas.copy_from_bbox(ax.bbox)

    # # --- fast update function ---
    # def update_plot(new_val, idx):
    #     data[idx] = new_val
    #     line.set_ydata(data)
    #     fig.canvas.restore_region(background)
    #     ax.draw_artist(line)
    #     fig.canvas.blit(ax.bbox)
    #     fig.canvas.flush_events()


    import time
    last = time.time()
    
    # --- simulate incoming samples ---
    idx = 0
    electrode_stream = eeg.get_electrode_generator(0, blocking=True)

    f = open("test.csv", "w+")
    eeg.start()
    print("Starting EEG...")
    for i in electrode_stream():
        f.write(str(i)) # type: ignore
        f.write(",")

        t = time.time()
        print(f"Delta: {t-last}, value: {i}")
        last = time.time()
        
        
        # if len(data) < freq*2:
        #     data.append(i)
        # else:
        #     data.pop(0)
        #     data.append(i)

        # # update line data
        # line.set_ydata(data)
        # line.set_xdata(range(len(data)))

        # # rescale axes if needed
        # ax.relim()
        # ax.autoscale_view()

        # # restore background, draw only the line, then blit
        # fig.canvas.restore_region(background) # type: ignore
        # ax.draw_artist(line)
        # fig.canvas.blit(ax.bbox)
        # fig.canvas.flush_events()

        # update_plot(i, idx)
        # idx = (idx + 1) % buffer_size