# Bare Metal HFT Engine for BIST (Cortex-A9 / FPGA)

![Language](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)
![Platform](https://img.shields.io/badge/platform-Xilinx%20Zynq%20(Cortex--A9)-orange.svg)
![Status](https://img.shields.io/badge/status-Simulation-green.svg)

## 📖 Overview

This project implements a **High-Frequency Trading (HFT)** execution engine designed for the **Borsa Istanbul (BIST)** exchange. It is optimized for **embedded environments** (specifically the ARM Cortex-A9 core found in Xilinx Zynq SoCs) following **Bare Metal** principles.

The system processes market data (**ITCH**) and generates order entries (**OUCH**) with a focus on **Zero-Copy** latency optimization and **Static Memory Allocation**.

## 🚀 Key Features

* **Zero-Copy Architecture:** * Utilizes `reinterpret_cast` to map raw binary buffers directly to C++ structs.
    * Eliminates `memcpy` and intermediate object construction in the critical path.
* **Protocol Implementation:**
    * **BIST ITCH:** Market data parsing (Add, Execute, Cancel, Delete orders).
    * **BIST OUCH:** Order entry protocol (Enter, Cancel, Replace orders).
* **Bare Metal Optimization:**
    * **No OS Dependencies:** Removed `<arpa/inet.h>` and other OS-specific headers.
    * **Custom Endianness Handling:** Implemented inline `swap16/32/64` intrinsics for Big-Endian protocol handling.
    * **Static Allocation:** Uses static arrays and pools instead of `std::vector` or `new/malloc` to avoid heap fragmentation and latency spikes.
* **Offline Simulation:** * Includes a hex-dump replay engine to test strategies against historical data without network hardware.

## 🛠️ Architecture

The system is composed of four main modules:

1.  **`HFT_ITCH` (The Parser):** Reads raw byte streams and updates the Order Book. Uses pointer casting for O(1) parsing.
2.  **`OrderBook` (The Memory):** Maintains the Limit Order Book (L2 Data) using optimized price level structures.
3.  **`Algorithm` (The Brain):** Implements the **Simple First Touch (SFT)** strategy. It reacts to snapshots and manages order states (Idle, Pending, Long).
4.  **`HFT_OUCH` (The Executor):** Formats outgoing order packets in Big-Endian format suitable for BIST gateways.

## 📂 Project Structure

```text
├── src/
│   ├── main.cpp            # Simulation entry point & hex reader loop
│   ├── algorithm.hpp       # SFT Strategy logic & state machine
│   ├── algorithm.cpp       # Strategy implementation
│   ├── HFT_ITCH.hpp        # Zero-Copy ITCH protocol definitions
│   ├── HFT_OUCH.hpp        # OUCH protocol structs & endianness helpers
│   ├── orderBook.hpp       # L2 Order Book implementation
│   └── visualizer.hpp      # CLI Order Book visualizer (for debugging)
├── data/
│   └── itch_hex_dump.txt   # Input data (ASCII Hex format)
└── output/
    └── ouch_orders.txt     # Generated order logs (Hex dump)
