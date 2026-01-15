#!/bin/bash
# Build deck-player WebAssembly module for web interface
# Calls the canonical build script from the plugin directory

set -e

echo "Building deck-player.wasm via plugin build script..."

# Call the plugin's build script
cd ../plugins/RGDeckPlayer
./build-deck-wasm.sh

echo ""
echo "✓ Deck player WASM built successfully"
echo "  Output: web/players/deck-player.{js,wasm}"
