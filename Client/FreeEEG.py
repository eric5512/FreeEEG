import socket
import matplotlib.pyplot as plt

class FreeEEG:
    __slots__ = ('__udp', '__remote_address', '__local_address')
    def __init__(self, remote_ip = "192.168.1.1", remote_port = 69, local_ip = "127.0.0.1", local_port = 6969) -> None:
        self.__remote_address = (remote_ip, remote_port)
        self.__local_address = (local_ip, local_port)

        self.__udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.__udp.bind(self.__local_address)

    def stream(self):
        while True:
            yield self.__udp.recv(8)

    def start(self):
        self.__udp.sendto("start".encode(), self.__remote_address)

    def stop(self):
        self.__udp.sendto("stop".encode(), self.__remote_address)


if __name__ == '__main__':
    plt.ion()
    eeg = FreeEEG("127.0.0.1", 4321)
    freq = 10
    # data = [0 for _ in range(freq*2)]
    plot = plt.plot([0])[0]
    data = []
    c = 0
    eeg.start()
    for i in eeg.stream():
        d = int.from_bytes(i)
        if len(data) < freq*2:
            data.append(d)
        else:
            data.pop(0)
            data.append(d)
        plot.remove()
        plot = plt.plot(data, color='g')[0]
        plt.pause(0.01)