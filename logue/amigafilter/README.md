# Amiga Filter - NTS-3 Background Effect

Amiga Paula hardware RC filter emulation for Korg NTS-3 kaoss pad.


## Features

- Background Effect  
  Runs continuously without requiring HOLD button
- 4 Filter Types:
  - A500 (4.9kHz)  
    Classic Amiga 500 low-pass RC filter
  - A500+LED (3.3kHz)  
    Amiga 500 with power LED loading
  - A1200 (32kHz)  
    Amiga 1200 high-frequency RC filter
  - A1200+LED (3.3kHz)  
    Amiga 1200 with power LED loading
- Mix Control: 0-100% wet/dry mix


## Background Operation

This effect uses the `get_raw_input()` API to run as a background effect, meaning:

- Always processes audio (even when pad is not touched)
- No HOLD button required
- Ideal for filters that should be always-on


## Building

### Prerequisites

- logue-sdk at `~/Projects/logue-sdk`
- Docker/Podman with logue-sdk build image

https://github.com/gbraad-logue/logue-sdk/pkgs/container/logue-sdk%2Fdev-env


### Build Command

```bash
cd ~/Projects/rfx/logue/amigafilter
./build.sh
```


## License

ISC License - Copyright (C) 2026
