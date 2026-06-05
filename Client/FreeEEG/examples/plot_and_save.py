from FreeEEG import FreeEEG

if __name__ == '__main__':
    import signal
    from collections import deque

    import matplotlib.pyplot as plt
    from matplotlib.animation import FuncAnimation

    import datetime

    BUFFER_SIZE = 1000
    CHANNELS_TO_PLOT = [0]  # Add more channels later, e.g. [0, 1, 2, 3]
    
    eeg = FreeEEG()

    data_stream = eeg.get_data_generator(blocking=False)
    generator = data_stream()

    time_data = deque(maxlen=BUFFER_SIZE)
    channel_data = {
        ch: deque(maxlen=BUFFER_SIZE)
        for ch in CHANNELS_TO_PLOT
    }

    t0 = None

    def signal_handler(sig, frame):
        eeg.stop()
        csv.close()
        plt.close('all')

    signal.signal(signal.SIGINT, signal_handler)

    fig, ax = plt.subplots(figsize=(10, 5))

    lines = {}
    for ch in CHANNELS_TO_PLOT:
        line, = ax.plot([], [], label=f'EEG channel {ch + 1}')
        lines[ch] = line

    ax.set_title('FreeEEG real-time EEG data')
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Voltage (V)')
    ax.grid(True)
    ax.legend(loc='upper right')

    start = datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
    csv = open(f"recording_{start}.csv", "w+")
    
    def update(frame):
        global t0, csv

        packet = next(generator)

        if packet is None:
            return tuple(lines.values())

        csv.write(packet.as_csv())
        
        for rel_timestamp, data in packet.elec_data:
            timestamp = packet.abs_time + rel_timestamp / 1e6

            if t0 is None:
                t0 = timestamp

            time_s = (timestamp - t0)
            time_data.append(time_s)

            for ch in CHANNELS_TO_PLOT:
                adc_value = data[ch]
                volts = FreeEEG.adc_to_volts(adc_value)
                channel_data[ch].append(volts)

        for ch in CHANNELS_TO_PLOT:
            lines[ch].set_data(time_data, channel_data[ch])

        if len(time_data) >= 2:
            ax.set_xlim(time_data[0], time_data[-1])

        all_values = [
            value
            for ch in CHANNELS_TO_PLOT
            for value in channel_data[ch]
        ]

        if all_values:
            ymin = min(all_values)
            ymax = max(all_values)
            margin = 0.1 * (ymax - ymin) if ymax != ymin else 1e-6
            ax.set_ylim(ymin - margin, ymax + margin)

        return tuple(lines.values())

    eeg.start()
    print("Starting EEG...")

    try:
        animation = FuncAnimation(fig, update, interval=20, blit=False, cache_frame_data=False)
        plt.show()
    finally:
        eeg.stop()
