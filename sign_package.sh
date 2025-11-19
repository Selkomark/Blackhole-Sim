#!/bin/bash

# One-stop script for building, signing, and packaging macOS application
set -e

# Get the project root directory (where this script is located)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${PROJECT_ROOT}"

EXPORT_DIR="export"
APP_NAME="BlackHoleSim"
APP_BUNDLE="${EXPORT_DIR}/${APP_NAME}.app"
DMG_FILE="${EXPORT_DIR}/${APP_NAME}-1.0.dmg"

echo "=========================================="
echo "  Black Hole Simulation - Release Build"
echo "=========================================="
echo ""

# Step 1: Clean previous builds
echo "[1/4] Cleaning previous builds..."
make clean > /dev/null 2>&1 || true
echo "✓ Cleaned"

# Step 2: Build the application
echo "[2/4] Building application..."
if ! make > /dev/null 2>&1; then
    echo "✗ Build failed! Showing errors:"
    make
    exit 1
fi
echo "✓ Build successful"

# Step 3: Create app bundle
echo "[3/4] Creating app bundle..."
if ! make app > /dev/null 2>&1; then
    echo "✗ App bundle creation failed! Showing errors:"
    make app
    exit 1
fi
echo "✓ App bundle created: ${APP_BUNDLE}"

# Step 4: Sign the app (optional)
echo "[4/4] Signing application..."
IDENTITY="${MACOS_SIGNING_IDENTITY:-}"
if [ -z "$IDENTITY" ]; then
    echo "  ⚠ No signing identity found. Attempting to find Developer ID..."
    IDENTITY=$(security find-identity -v -p codesigning 2>/dev/null | grep "Developer ID Application" | head -1 | sed 's/.*"\(.*\)".*/\1/' || echo "")
fi

if [ -z "$IDENTITY" ]; then
    echo "  ⚠ Skipping code signing (no certificate found)"
    echo "  To sign the app, set MACOS_SIGNING_IDENTITY environment variable:"
    echo "    export MACOS_SIGNING_IDENTITY='Developer ID Application: Your Name (TEAM_ID)'"
    SKIP_SIGN=true
else
    echo "  Using identity: ${IDENTITY}"
    if ! make sign > /dev/null 2>&1; then
        echo "  ⚠ Signing failed, but continuing..."
        SKIP_SIGN=true
    else
        echo "✓ App signed successfully"
        SKIP_SIGN=false
    fi
fi

# Step 5: Create DMG
echo "[5/5] Creating DMG..."
if ! make dmg > /dev/null 2>&1; then
    echo "✗ DMG creation failed! Showing errors:"
    make dmg
    exit 1
fi
echo "✓ DMG created: ${DMG_FILE}"

echo ""
echo "=========================================="
echo "  Release Build Complete!"
echo "=========================================="
echo ""
echo "Output files:"
echo "  • Executable: ${EXPORT_DIR}/blackhole_sim"
echo "  • App Bundle: ${APP_BUNDLE}"
echo "  • DMG File:   ${DMG_FILE}"
echo ""

if [ "$SKIP_SIGN" = true ]; then
    echo "⚠ Warning: App was not signed. For distribution, you should:"
    echo "  1. Get a Developer ID Application certificate"
    echo "  2. Set MACOS_SIGNING_IDENTITY environment variable"
    echo "  3. Run: make sign"
    echo "  4. Recreate DMG: make dmg"
    echo ""
fi

echo "To test the app bundle:"
echo "  open ${APP_BUNDLE}"
echo ""
echo "To test the DMG:"
echo "  open ${DMG_FILE}"
echo ""

