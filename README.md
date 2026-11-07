# Happy Accident 🎸⚡

A high-performance, ultra-low-latency native digital guitar effects processor (pedalboard) written in pure **C** for Windows. Optimized specifically for use with the **Arturia MiniFuse** audio interface via the **Windows Audio Session API (WASAPI)** in Shared Mode.

What started as an exploration into guitar tone processing evolved into a clean, minimal, zero-latency implementation of a classic analog **Fuzz Face** topology combined with additive EQ stages—all running natively in a flat sample-processing loop.

---

## Features

* **Ultra-Low Latency:** Bypasses heavy commercial plugin bloat by communicating directly with Windows Core Audio at 32-bit float, 44.1kHz.
* **Analog Fuzz Modeling:** Utilizes `tanhf` for warm, tube-like soft-clipping behavior without digital artifacts or signal choking.
* **Additive EQ & Presence Boost:** Shaped spectral enhancement designed to keep signals thick and articulate.
* **Master Make-up Gain:** Calibrated output boost ensuring mass volume and punch without breaking buffer stability.
* **Flat Buffer Architecture:** Completely avoids complex conditional nesting or circular delay buffers, preventing audio dropouts or freezing.

---

## Requirements

* **OS:** Windows 10 / 11
* **Compiler:** GCC (MinGW-w64)
* **Hardware:** Audio interface (optimized and tested with Arturia MiniFuse series)

---

## Compilation & Usage

Make sure you have GCC (MinGW) installed, open your terminal in the project directory, and run the following commands:

```powershell
gcc main.c -o guitar_rig.exe -lole32 -luuid -lwinmm -lksuser -lm

To run it:
.\guitar_rig.exe

To Exit: Press ESC, q, or Q while the console window is focused.

Architecture Overview
The core processing loop iterates directly over the uncompressed sample stream retrieved through WASAPI capture and render clients:

for (UINT32 i = 0; i < totalSamples; i++) {
    float sample = src[i];

    // 1. Additive Bass Boost
    sample *= bass_boost;

    // 2. Analog Fuzz / Soft-clipping (tanhf)
    sample = tanhf(sample * overdrive_gain);

    // 3. Post-distortion Presence Boost
    sample *= presence_boost;

    // 4. Final Master Volume / Make-up Gain
    sample *= output_boost;

    dst[i] = sample;
}
