if __name__ == '__main__':
    import socket
    import struct
    import time
    import signal
    import numpy as np

    remote_ip = "192.168.4.1"
    remote_port = 69
    local_ip = ""
    local_port = 6969
    
    remote_address = (remote_ip, remote_port)
    local_address = (local_ip, local_port)
    
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp.bind(local_address)

    f = open("sound_recording.csv", "w+")
    def signal_handler(sig, frame):
        udp.sendto("stop".encode(), remote_address)
        print("Stopping recording...")
        udp.close()
        f.close()
        exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    n = udp.sendto("start".encode(), remote_address)
    print(f"Sent {n} bytes")
    print("Starting recording...")
    while True:
        data = udp.recv(1024*2)
        size = len(data)
        unpacked = struct.unpack(f"<{size//4}I", data)
        f.write(",".join(str(i) for i in unpacked) + ",")
