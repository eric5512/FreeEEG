from FreeEEG import FreeEEG

if __name__ == '__main__':
    import time
    import signal
    import numpy as np
    eeg = FreeEEG() # FreeEEG("127.0.0.1", 4321)

    def signal_handler(sig, frame):
        eeg.stop()
        exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    
    data_stream = eeg.get_data_generator(blocking=True)

    eeg.start()
    print("Starting EEG...")
    beg = time.time()
    count = 0
    for i in data_stream():
        print(i)
