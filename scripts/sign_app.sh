#!/bin/bash

# Script to code sign macOS app bundle
set -e

EXPORT_DIR="export"
APP_NAME="BlackHoleSim"
APP_BUNDLE="${EXPORT_DIR}/${APP_NAME}.app"

if [ ! -d "${APP_BUNDLE}" ]; then
    echo "Error: App bundle not found. Run 'make app' first."
    exit 1
fi

# Check for signing identity
IDENTITY="${MACOS_SIGNING_IDENTITY:-}"
if [ -z "$IDENTITY" ]; then
    echo "Warning: MACOS_SIGNING_IDENTITY not set. Attempting to find Developer ID..."
    IDENTITY=$(security find-identity -v -p codesigning | grep "Developer ID Application" | head -1 | sed 's/.*"\(.*\)".*/\1/' || echo "")
fi

if [ -z "$IDENTITY" ]; then
    echo "Warning: No code signing identity found. App will not be signed."
    echo "To sign the app, set MACOS_SIGNING_IDENTITY environment variable:"
    echo "  export MACOS_SIGNING_IDENTITY='Developer ID Application: Your Name (TEAM_ID)'"
    echo ""
    echo "Or run:"
    echo "  security find-identity -v -p codesigning"
    echo "to list available identities."
    exit 0
fi

echo "Signing app with identity: ${IDENTITY}"

# Sign the executable
codesign --force --deep --sign "${IDENTITY}" \
    --options runtime \
    --entitlements scripts/entitlements.plist \
    "${APP_BUNDLE}"

# Verify signature
codesign --verify --verbose "${APP_BUNDLE}"
spctl --assess --verbose "${APP_BUNDLE}"

echo "App signed successfully!"

