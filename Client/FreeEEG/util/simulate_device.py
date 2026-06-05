import socket
import time

    
# Electrode : [type (0b00) (2 bits)][timestamp rel (24 bits)][value (24 bits)]* <- Variable lenght depending on num of channels active
def electrode_packet(timestamp: int, data: list[int]) -> list[bytes]:
    return timestamp

udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

udp.bind(("", 4321))

freq = 10
c = 0

while True:
    data, (addr, port) = udp.recvfrom(20)
    if "start" in data.decode():
        going = True
    else:
        going = False
    while going:
        time.sleep(1/freq)
        n = c % freq
        c += 1
        udp.sendto(b"\x20\x00\x00"+n.to_bytes(), (addr, port))
