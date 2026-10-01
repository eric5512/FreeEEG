from multiprocessing import Pipe
import threading
import socket
import struct
from dataclasses import dataclass

from .protocol import *

class FreeEEG:
    __slots__ = ('__udp', '__remote_address', '__local_address',
                 '__pipe_in', '__pipe_out', 
                 '__filler_thread', '__active')

    FSR = 5.0
    __BUFF_SIZE = 1024*10
    N_ELECTRODE = N_ELECTRODE
    
    def __init__(self, remote_ip = "192.168.4.1", remote_port = 69, local_ip = "", local_port = 6969) -> None:    
        self.__active = False

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

    def __stream_filler(self):
        while self.__active:
            packet = self.__udp.recv(self.__BUFF_SIZE)
            try:
                self.__pipe_in.send(Packet.parse_packet(packet))
            except Exception as e:
                print(e)
            

    def start(self):
        if self.__active:
            return
        
        self.__udp.sendto("start".encode(), self.__remote_address)
        self.__filler_thread = threading.Thread(target=self.__stream_filler, daemon=True)
        self.__active = True
        self.__filler_thread.start()

    def stop(self):
        if not self.__active:
            return
        
        self.__active = False
        self.__udp.sendto("stop".encode(), self.__remote_address)
        self.__udp.close()

    def adc_to_volts(value, gain):
        return value * FreeEEG.FSR / (gain*8388607.0)
