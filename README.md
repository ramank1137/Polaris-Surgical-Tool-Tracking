
# Polaris Surgical Tool Tracking

GStreamer-based pipeline to track surgical tools using an NDI Polaris optical tracking system and the vendor’s **Combined API Sample** demo application.

This repository provides:

- A small Python wrapper (`ndiTrack.py`) to launch the NDI *CombinedAPISample* AR demo (`run.sh`) from Python.
- Convenience scripts to install required system dependencies (GStreamer, GTK) and to configure the correct path to the NDI Combined API sample.

> ⚠️ **Note:** The actual NDI *CombinedAPISample* code and binaries are **not** included here. You must obtain them from your Polaris / Aurora CD or NDI support and copy the `CombinedAPISample` folder onto your host machine, as described in the official manuals.

---

## Repository Structure

```text
Polaris-Surgical-Tool-Tracking/
├── Polaris-Surgical-Tool-Tracking/  # (optional vendor/sample code – see below)
├── ndiTrack.py                      # Python wrapper around NDI AR demo shell script
├── setup.py                         # Python packaging for ndiTrack
├── install.sh                       # Installs GStreamer + GTK system dependencies
├── setup.sh                         # Wires ndiTrack to your local CombinedAPISample path and installs the module
└── .DS_Store                        # macOS metadata (can be ignored)
```

### Key Components

- **`ndiTrack.py`**

  Defines a single class:

  ```python
  class Ardemo:
      def __init__(self, ip: str,
                   tool_location: str,
                   tool_file: str,
                   rtsp_location: str,
                   video_port: str,
                   capture_path: str):
          ...
      def run(self):
          ...
  ```

  Internally, `Ardemo.run()` calls:

  ```bash
  bash <ARDEMO_PATH>/run.sh run <ip> <tool_directory> <tool_file> <rtsp_location> <video_port> <capture_path>
  ```

- **`setup.py`** — Installs `ndiTrack` module.

- **`install.sh`** — Installs GStreamer + GTK dependencies.

- **`setup.sh`** — Rewrites ARDEMO path, installs, restores original file.

---

## Prerequisites

### Hardware & Vendor Software

- NDI Polaris tracker (Vega, Lyra, Vicra etc.)
- NDI Combined API Sample (must be copied manually into repo)

### Host System

- Ubuntu/Debian system
- Python 3.x, pip, setuptools

---

## Setup

```bash
git clone https://github.com/ramank1137/Polaris-Surgical-Tool-Tracking.git
cd Polaris-Surgical-Tool-Tracking
```

Copy `CombinedAPISample` folder manually.

Install dependencies:

```bash
sudo bash install.sh
```

Configure & install:

```bash
sudo bash setup.sh
```

---

## Usage Example

```python
from ndiTrack import Ardemo

demo = Ardemo(
    ip="192.168.0.100",
    tool_location="/path/tools/",
    tool_file="xyz.rom",
    rtsp_location="rtsp://192.168.0.100:554/stream",
    video_port="5000",
    capture_path="/path/capture"
)

demo.run()
```

---

## Troubleshooting

| Issue | Fix |
|------|------|
| `CombinedAPISample` missing | Copy sample folder inside repo |
| GTK/GStreamer missing | Re-run `install.sh` |
| Permission errors | Use `sudo` or install to venv |

---

