#!/bin/bash
# RG52MINI TV Player - EmuELEC 4.7 Launcher
# Supports live TV streams via libmpv

# CA certificate fix
if [ ! -f /etc/ssl/certs/ca-certificates.crt ]; then
    mount -t tmpfs tmpfs /etc/ssl 2>/dev/null
    mkdir -p /etc/ssl/certs 2>/dev/null
    ln -sf /run/libreelec/cacert.pem /etc/ssl/certs/ca-certificates.crt 2>/dev/null
fi

# Paths
PORT_DIR=/storage/roms/ports/rg52mini-tv
TV_DIR=/storage/roms/TV
LOGFILE=$PORT_DIR/debug.log

# Create TV directory if not exists
mkdir -p $TV_DIR

# Logging
echo "=== RG52MINI TV Player Start: $(date) ===" > "$LOGFILE"
echo "PORT_DIR=$PORT_DIR" >> "$LOGFILE"
echo "TV_DIR=$TV_DIR" >> "$LOGFILE"

# Environment
export LD_LIBRARY_PATH=$PORT_DIR/libs
export SDL_VIDEODRIVER=kmsdrm
export SDL_AUDIODRIVER=alsa
export HOME=$PORT_DIR

# Binary info
echo "Binary info:" >> "$LOGFILE"
file $PORT_DIR/rg52mini-tv >> "$LOGFILE" 2>&1

# Run
cd $PORT_DIR
echo "Launching TV player..." >> "$LOGFILE"
./rg52mini-tv -d "$TV_DIR" >> "$LOGFILE" 2>&1

echo "Exit code: $?" >> "$LOGFILE"
