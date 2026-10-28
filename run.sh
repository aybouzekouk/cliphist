#!/bin/bash
# Launch Cliphist in the background, detached from the terminal.
# Run with:  ~/cliphist/run.sh
cd "$(dirname "$0")"
mkdir -p "$HOME/.local/share/cliphist"
nohup ./build/cliphist > "$HOME/.local/share/cliphist/cliphist.log" 2>&1 &
echo "Cliphist started (pid $!)."
echo "Log: $HOME/.local/share/cliphist/cliphist.log"
echo "Press Ctrl+Shift+V anywhere to open the popup."
