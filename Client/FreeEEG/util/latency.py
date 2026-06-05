from FreeEEG import FreeEEG

if __name__ == '__main__':
    import time
    import signal
    import numpy as np
    eeg = FreeEEG() # FreeEEG("127.0.0.1", 4321)

    def signal_handler(sig, frame):
        # global f
        # f.close()
        global latencies
        latencies = np.array(sorted(latencies))
        diffs = (1000*(latencies[1:] - latencies[:-1])).astype(int)
        with open("data.txt", "w+") as f:
            f.write("\n".join([f"{i},{j}" for i, j in zip(diffs, latencies[1:])]))
        time_diff = time.time() - beg
        print(f"Throughput: {count} packets in {time_diff} seconds; {count/time_diff} packets/s")
        eeg.stop()
        exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    
    data_stream = eeg.get_data_generator(blocking=True)

    # f = open("test.csv", "w+")
    eeg.start()
    print("Starting EEG...")
    # f.write("timestamp,e1,e2,e3,e4,e5,e6,e7,e8,ax,ay,az,gx,gy,gz,event\n")
    latencies = []
    beg = time.time()
    count = 0
    for i in data_stream():
        # f.write(i.as_csv() + "\n")
        for j in i.elec_time:
            count += 1 
            time_ = j / 1e6 + i.abs_time
            latencies.append(time_)
