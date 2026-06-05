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

        self.__active = False

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
        elec_data: list[tuple[int, list[int]]]
        imu_data: list[tuple[int, tuple[int,int,int], tuple[int,int,int]]]
        event_data: list[tuple[int, int]]

        def as_csv(self) -> str:
            buff = ""
            # timestamp,e1,e2,e3,e4,e5,e6,e7,e8,ax,ay,az,gx,gy,gz,event
            for et, ed in self.elec_data:
                buff += f"{self.abs_time+et/1e6}," + ",".join(str(FreeEEG.adc_to_volts(i)) for i in ed) + "\n"
                
            return buff
                
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
        (
            elec_timestamps[i],
            elec_data_flat[i * self.N_ELECTRODE:(i + 1) * self.N_ELECTRODE],
        )
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

        return self.Packet(abs_timestamp, elec_data, imu_data, event_data)

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

    def adc_to_volts(value):
        return (value if value >= 0x800000 else value - 0xFFFFFF) * FreeEEG.FSR / 0xFFFFFF
