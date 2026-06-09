import re
import time
import serial
import matplotlib.pyplot as plt

SERIAL_PORT = 'COM10'
BAUD_RATE   = 115200
TARGET_SLOT = "Slot 04"

DISTANCES    = [round(d * 0.5, 1) for d in range(9)]  # 0.0, 0.5, 1.0 ... 4.0
SAMPLES_PER  = 50

# ── Serial ────────────────────────────────────────────────────────────────────
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    print(f"Connected to {SERIAL_PORT}\n")
except Exception as e:
    print(f"Error opening {SERIAL_PORT}: {e}")
    exit()

# ── Plot setup ────────────────────────────────────────────────────────────────
plt.ion()
fig, ax = plt.subplots(figsize=(9, 5))
line_plot, = ax.plot([], [], marker='o', linestyle='-', color='tab:blue',
                     linewidth=2, markersize=7)
ax.set_title(f'Latency vs Distance — {TARGET_SLOT}')
ax.set_xlabel('Distance (m)')
ax.set_ylabel(r'Average Latency ($\mu$s)')
ax.set_xticks(DISTANCES)
ax.grid(True, linestyle='--', alpha=0.6)
plt.tight_layout()
plt.pause(0.001)

avg_latencies = []   # one value per distance point

# ── Helper: flush serial garbage before real collection ───────────────────────
def flush_serial(duration=0.5):
    deadline = time.time() + duration
    while time.time() < deadline:
        ser.read(ser.in_waiting or 1)
        time.sleep(0.05)

# ── Helper: collect N latency samples from serial ────────────────────────────
def collect_samples(n, distance):
    samples = []
    buf = ""
    print(f"  Collecting {n} samples at {distance} m ...")
    while len(samples) < n:
        if ser.in_waiting > 0:
            buf += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            while "\n" in buf:
                line, buf = buf.split("\n", 1)
                line = line.strip()
                if TARGET_SLOT in line and "Latency =" in line:
                    m = re.search(r"Latency =\s*(\d+)\s*us", line)
                    if m:
                        val = int(m.group(1))
                        samples.append(val)
                        print(f"    [{len(samples):>2}/{n}] {val} us", end='\r')
        else:
            plt.pause(0.01)
    print()   # newline after the \r progress
    return samples

# ── Main loop ─────────────────────────────────────────────────────────────────
print("=" * 55)
print(f"  Latency vs Distance — {len(DISTANCES)} points, {SAMPLES_PER} samples each")
print(f"  Distances: {DISTANCES} m")
print("=" * 55)

try:
    for i, dist in enumerate(DISTANCES):
        print(f"\n[{i+1}/{len(DISTANCES)}] Next distance: {dist} m")
        input(f"  Move device to {dist} m and press Enter when ready...")

        # Flush any stale bytes that arrived during repositioning
        flush_serial(duration=0.5)

        # Collect samples
        samples = collect_samples(SAMPLES_PER, dist)

        avg = sum(samples) / len(samples)
        avg_latencies.append(avg)
        print(f"  → Average latency at {dist} m: {avg:.1f} us")

        # Update plot
        x_so_far = DISTANCES[:len(avg_latencies)]
        line_plot.set_xdata(x_so_far)
        line_plot.set_ydata(avg_latencies)
        ax.relim()
        ax.autoscale_view()

        # Annotate each point with its value
        for txt in ax.texts:
            txt.remove()
        for x, y in zip(x_so_far, avg_latencies):
            ax.annotate(f"{y:.0f}", xy=(x, y),
                        textcoords="offset points", xytext=(0, 8),
                        ha='center', fontsize=8, color='tab:blue')

        plt.pause(0.001)

    print("\n" + "=" * 55)
    print("  All distances collected!")
    print("  Summary:")
    for d, a in zip(DISTANCES, avg_latencies):
        print(f"    {d:>4.1f} m  →  {a:.1f} us")
    print("=" * 55)

except KeyboardInterrupt:
    print("\n\nStopped early.")

finally:
    ser.close()
    plt.ioff()
    plt.show()
