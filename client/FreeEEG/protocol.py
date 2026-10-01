import struct
from dataclasses import dataclass


N_ELECTRODE = 8

class MalformedPacket(Exception):
    def __init__(self, msg):
        super().__init__(msg)

def malformed_packet(msg, x):
    raise MalformedPacket(f"{msg} is out of bounds: {x}")

@dataclass
class Packet:
    abs_time: int
    elec_data: list[tuple[int, list[int]]]
    imu_data: list[tuple[int, tuple[int,int,int], tuple[int,int,int]]]
    event_data: list[tuple[int, int]]

    CSV_HEADER = "timestamp,e1,e2,e3,e4,e5,e6,e7,e8,ax,ay,az,gx,gy,gz,event\n"

    def parse_packet(data):
        global N_ELECTRODE
        __ELEC_BUFF_SIZE =  30
        __IMU_BUFF_SIZE =   5
        __EVENT_BUFF_SIZE = 2

        check_bound = lambda x, y, msg: x if x <= y else malformed_packet(msg, x)

        offset = 4

        n_elec = check_bound(data[offset], N_ELECTRODE, "# electrodes")
        offset += 1

        n_elec_samp = check_bound(data[offset], __ELEC_BUFF_SIZE, "# electrode samples")
        offset += 4*(n_elec_samp*(1+n_elec))+1

        n_imu = check_bound(data[offset], __IMU_BUFF_SIZE, "# imu samples")
        offset += 16*n_imu+1

        n_events = check_bound(data[offset], __EVENT_BUFF_SIZE, "# of events")

        struct_fmt = (
            "<"
            "I"      # abs_timestamp
            "B"      # num_elec_
            "B"      # num_elec_samp
            f"{n_elec_samp}I"
            f"{n_elec_samp * n_elec}i"
            "B"      # num_imu_samp
            f"{n_imu}I"
            f"{n_imu * 3}H"
            f"{n_imu * 3}H"
            "B"      # num_events_samp
            f"{n_events}I"
            f"{n_events}B"
        )

        unpacked_data = struct.unpack_from(struct_fmt, data)

        offset = 0

        abs_timestamp = unpacked_data[offset]
        offset += 3

        elec_timestamps = unpacked_data[offset:offset+n_elec_samp]
        offset += n_elec_samp
        elec_data_flat = unpacked_data[offset:offset+n_elec_samp*n_elec]
        offset += n_elec_samp*n_elec+1

        imu_timestamps = unpacked_data[offset:offset+n_imu]
        offset += n_imu

        accel_data_flat = unpacked_data[offset:offset+n_imu*3]
        offset += n_imu*3

        gyro_data_flat = unpacked_data[offset:offset+n_imu*3]
        offset += n_imu*3+1

        event_timestamps = unpacked_data[offset:offset+n_events]
        offset += n_events

        events = unpacked_data[offset:offset+n_events]
        
        elec_data = [
            (
                elec_timestamps[i],
                elec_data_flat[i * n_elec:(i + 1) * n_elec],
            )
            for i in range(n_elec_samp)
        ]
    
        imu_data = [
            (
                imu_timestamps[i],
                accel_data_flat[i * 3:(i + 1) * 3],
                gyro_data_flat[i * 3:(i + 1) * 3],
            ) for i in range(n_imu)]
    
        event_data = list(zip(event_timestamps, events))
        return Packet(abs_timestamp, elec_data, imu_data, event_data)        
        
    def as_csv(self, gain=None) -> str:
        buff = ""
        if gain == None:
            for et, ed in self.elec_data:
                buff += f"{self.abs_time+et/1e6}," + \
                    ",".join(str(i) for i in ed) + \
                    "".join("," for i in range(6)) + \
                    ",\n"
        else:
            for et, ed in self.elec_data:
                buff += f"{self.abs_time+et/1e6}," + \
                    ",".join(str(FreeEEG.adc_to_volts(i, gain)) for i in ed) + \
                    "".join("," for i in range(6)) + \
                    ",\n"
                
                for it, ida in self.imu_data:
                    ad, gd = ida
                (ax, ay, az) = ad
                (gx, gy, gz) = gd
                buff += f"{self.abs_time+it/1e6}," + "".join("," for i in range(FreeEEG.N_ELECTRODES)) + f"{ax},{ay},{az},{gx},{gy},{gz},\n"

            return buff

