#!/bin/bash
# RG52MINI TV Player - EmuELEC 4.7 Launcher

# CA certificate fix
if [ ! -f /etc/ssl/certs/ca-certificates.crt ]; then
    mount -t tmpfs tmpfs /etc/ssl 2>/dev/null
    mkdir -p /etc/ssl/certs 2>/dev/null
    ln -sf /run/libreelec/cacert.pem /etc/ssl/certs/ca-certificates.crt 2>/dev/null
fi

# Paths
PORT_DIR=/roms/ports/rg52mini-tv
TV_DIR=/roms/TV
LOGFILE=$PORT_DIR/debug.log

# Create directories
mkdir -p $TV_DIR
mkdir -p $PORT_DIR

# Logging
echo "=== RG52MINI TV Player Start: $(date) ===" > "$LOGFILE"
echo "PORT_DIR=$PORT_DIR" >> "$LOGFILE"
echo "TV_DIR=$TV_DIR" >> "$LOGFILE"

# Check files
echo "=== Files check ===" >> "$LOGFILE"
ls -la $PORT_DIR/rg52mini-tv >> "$LOGFILE" 2>&1
ls -la $PORT_DIR/assets/fonts/ >> "$LOGFILE" 2>&1
ls $PORT_DIR/libs/ | head -5 >> "$LOGFILE" 2>&1

# Ensure executable
chmod +x $PORT_DIR/rg52mini-tv 2>/dev/null

# Environment
export LD_LIBRARY_PATH=$PORT_DIR/libs
export SDL_VIDEODRIVER=kmsdrm
export SDL_AUDIODRIVER=alsa
export HOME=$PORT_DIR
export XDG_RUNTIME_DIR=/tmp/runtime-root
mkdir -p $XDG_RUNTIME_DIR

echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >> "$LOGFILE"
echo "SDL_VIDEODRIVER=$SDL_VIDEODRIVER" >> "$LOGFILE"

# Run
cd $PORT_DIR
echo "Launching TV player..." >> "$LOGFILE"
./rg52mini-tv -d "$TV_DIR" >> "$LOGFILE" 2>&1
EXIT_CODE=$?
echo "Exit code: $EXIT_CODE" >> "$LOGFILE"
echo "=== End: $(date) ===" >> "$LOGFILE"
