import re
import time
import serial
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

SERIAL_PORT = 'COM10'
BAUD_RATE   = 115200
TARGET_SLOT = "Slot 04"

DISTANCES   = [round(d * 0.5, 1) for d in range(9)]  # 0.0 .. 4.0 m
SAMPLES_PER = 50

# RTT threshold that separates clean packets from retransmitted ones.
RETRANSMIT_THRESHOLD_US = 550

# ── Serial ────────────────────────────────────────────────────────────────────
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    print(f"Connected to {SERIAL_PORT}\n")
except Exception as e:
    print(f"Error opening {SERIAL_PORT}: {e}")
    exit()

# ── Plot layout (Light Theme) ─────────────────────────────────────────────────
plt.ion()
fig = plt.figure(figsize=(11, 7))
fig.patch.set_facecolor('#f8f9fa')  # Light page background

gs = gridspec.GridSpec(2, 1, hspace=0.45, top=0.88, bottom=0.10,
                        left=0.09, right=0.97)

ax_rtt  = fig.add_subplot(gs[0])   # top:    Mean RTT
ax_retr = fig.add_subplot(gs[1])   # bottom: Retransmit rate %

PANEL_BG  = '#ffffff'  # Crisp white panel backgrounds
ACCENT    = '#0066cc'  # Clear professional blue
WARN      = '#d9383a'  # Vivid soft red for warnings
GRID_COL  = '#e1e4e8'  # Subtle light grey grids
TEXT_COL  = '#24292e'  # Dark slate for primary text
MUTED_TXT = '#586069'  # Secondary grey for sub-labels

for ax in (ax_rtt, ax_retr):
    ax.set_facecolor(PANEL_BG)
    ax.tick_params(colors=TEXT_COL, labelsize=8)
    ax.xaxis.label.set_color(TEXT_COL)
    ax.yaxis.label.set_color(TEXT_COL)
    ax.spines[:].set_color(GRID_COL)
    ax.grid(True, color=GRID_COL, linewidth=0.6, linestyle='--')
    ax.set_xticks(DISTANCES)

# Top subplot
ax_rtt.set_title(f'Mean RTT  —  {TARGET_SLOT}',
                 color=TEXT_COL, fontsize=10, pad=6, fontweight='semibold')
ax_rtt.set_ylabel('Mean RTT (µs)', color=TEXT_COL)
ax_rtt.set_xlabel('Distance (m)', color=TEXT_COL)

mean_line, = ax_rtt.plot([], [], marker='o', color=ACCENT,
                          linewidth=2, markersize=6, zorder=3)

# Bottom subplot
ax_retr.set_title(f'Retransmit Rate  —  {TARGET_SLOT}',
                  color=TEXT_COL, fontsize=10, pad=6, fontweight='semibold')
ax_retr.set_ylabel('Retransmit rate (%)', color=TEXT_COL)
ax_retr.set_xlabel('Distance (m)', color=TEXT_COL)
ax_retr.set_ylim(-5, 105)

retr_line, = ax_retr.plot([], [], marker='s', color=WARN,
                           linewidth=2, markersize=6, zorder=3)
retr_fill = None

fig.suptitle('nRF ESB TDMA  ·  RTT vs Distance', color=TEXT_COL,
             fontsize=13, fontweight='bold', y=0.96)

plt.pause(0.001)

# ── Storage ───────────────────────────────────────────────────────────────────
all_means = []
all_retr  = []

# ── Helpers ───────────────────────────────────────────────────────────────────
def flush_serial(duration=0.5):
    deadline = time.time() + duration
    while time.time() < deadline:
        ser.read(ser.in_waiting or 1)
        time.sleep(0.05)

def collect_samples(n, distance):
    samples = []
    buf = ""
    print(f"  Collecting {n} samples at {distance} m ...")
    while len(samples) < n:
        if ser.in_waiting > 0:
            buf += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            while "\n" in buf and len(samples) < n:
                line, buf = buf.split("\n", 1)
                line = line.strip()
                if TARGET_SLOT in line and "Latency =" in line:
                    m = re.search(r"Latency =\s*(\d+)\s*us", line)
                    if m:
                        val = int(m.group(1))
                        samples.append(val)
                        print(f"    [{len(samples):>2}/{n}]  {val} µs", end='\r')
        else:
            plt.pause(0.01)
    print()
    return samples

def update_plots():
    global retr_fill

    x = DISTANCES[:len(all_means)]

    # ── Top: mean line ────────────────────────────────────────────────────────
    mean_line.set_xdata(x)
    mean_line.set_ydata(all_means)

    for txt in ax_rtt.texts:
        txt.remove()
    for xi, yi in zip(x, all_means):
        ax_rtt.annotate(f"{yi:.0f}", xy=(xi, yi),
                        textcoords="offset points", xytext=(0, 8),
                        ha='center', fontsize=7.5, color=ACCENT, fontweight='bold')

    ax_rtt.relim()
    ax_rtt.autoscale_view()

    # ── Bottom: retransmit rate ───────────────────────────────────────────────
    retr_line.set_xdata(x)
    retr_line.set_ydata(all_retr)

    if retr_fill is not None:
        retr_fill.remove()
    retr_fill = ax_retr.fill_between(x, 0, all_retr,
                                     alpha=0.15, color=WARN, zorder=2)

    for txt in ax_retr.texts:
        txt.remove()
    for xi, yi in zip(x, all_retr):
        ax_retr.annotate(f"{yi:.0f}%", xy=(xi, yi),
                         textcoords="offset points", xytext=(0, 8),
                         ha='center', fontsize=7.5, color=WARN, fontweight='bold')

    plt.pause(0.001)

# ── Main loop ─────────────────────────────────────────────────────────────────
print("=" * 55)
print(f"  RTT vs Distance — {len(DISTANCES)} points, {SAMPLES_PER} samples each")
print(f"  Retransmit threshold: {RETRANSMIT_THRESHOLD_US} µs")
print(f"  Distances: {DISTANCES} m")
print("=" * 55)

try:
    for i, dist in enumerate(DISTANCES):
        print(f"\n[{i+1}/{len(DISTANCES)}]  Target: {dist} m")
        input(f"  Move device to {dist} m and press Enter when ready...")

        flush_serial(duration=0.5)
        samples = collect_samples(SAMPLES_PER, dist)
        arr = np.array(samples)

        mean_val = float(np.mean(arr))
        retr     = float(np.mean(arr > RETRANSMIT_THRESHOLD_US) * 100)

        all_means.append(mean_val)
        all_retr.append(retr)

        print(f"  → Mean {mean_val:.0f} µs  |  Retransmit {retr:.1f}%")
        update_plots()

    print("\n" + "=" * 55)
    print("  All distances collected. Summary:")
    print(f"  {'Dist':>5}  {'Mean':>8}  {'Retr%':>7}")
    print("  " + "-" * 25)
    for d, m, rt in zip(DISTANCES, all_means, all_retr):
        print(f"  {d:>5.1f}  {m:>8.0f}  {rt:>6.1f}%")
    print("=" * 55)

except KeyboardInterrupt:
    print("\n\nStopped early.")

finally:
    ser.close()
    plt.ioff()
    plt.show()
