#!/bin/bash

# Script to code sign macOS app bundle with interactive identity selection
set -e

EXPORT_DIR="export"
APP_NAME="BlackHoleSim"
APP_BUNDLE="${EXPORT_DIR}/${APP_NAME}.app"

if [ ! -d "${APP_BUNDLE}" ]; then
    echo "Error: App bundle not found. Run 'make app' first."
    exit 1
fi

# Function to select signing identity interactively
select_signing_identity() {
    echo ""
    echo "Available code signing identities:"
    echo "-----------------------------------"
    
    # Get all available identities (including hash and full identity string)
    local temp_file=$(mktemp)
    security find-identity -v -p codesigning 2>/dev/null | grep -E "^\s+[0-9]+\)" > "$temp_file" || true
    
    if [ ! -s "$temp_file" ]; then
        rm -f "$temp_file"
        echo "No code signing identities found."
        echo ""
        echo "To create a signing identity:"
        echo "  1. Go to https://developer.apple.com/account"
        echo "  2. Create a Developer ID Application certificate (for distribution)"
        echo "     or Apple Development certificate (for testing)"
        echo "  3. Download and install it in Keychain Access"
        return 1
    fi
    
    # Parse and display identities
    local count=1
    local identity_map=()
    while IFS= read -r line; do
        # Extract the identity string (everything between quotes)
        local identity=$(echo "$line" | sed -n 's/.*"\(.*\)".*/\1/p')
        if [ -n "$identity" ]; then
            echo "  [$count] $identity"
            identity_map+=("$identity")
            ((count++))
        fi
    done < "$temp_file"
    rm -f "$temp_file"
    
    if [ ${#identity_map[@]} -eq 0 ]; then
        echo "No valid identities found."
        return 1
    fi
    
    echo ""
    echo -n "Select identity number (1-${#identity_map[@]}): "
    read -r selection
    
    # Validate selection
    if ! [[ "$selection" =~ ^[0-9]+$ ]] || [ "$selection" -lt 1 ] || [ "$selection" -gt ${#identity_map[@]} ]; then
        echo "Invalid selection."
        return 1
    fi
    
    # Get selected identity (array is 0-indexed)
    local selected_index=$((selection - 1))
    echo "${identity_map[$selected_index]}"
}

# Check for signing identity
IDENTITY="${MACOS_SIGNING_IDENTITY:-}"

# If not set via environment variable, interactively select
if [ -z "$IDENTITY" ]; then
    IDENTITY=$(select_signing_identity)
    if [ -z "$IDENTITY" ]; then
        echo ""
        echo "⚠ Warning: No signing identity selected. App will not be signed."
        echo ""
        echo "To sign manually, set MACOS_SIGNING_IDENTITY environment variable:"
        echo "  export MACOS_SIGNING_IDENTITY='Developer ID Application: Your Name (TEAM_ID)'"
        echo "  make sign"
        exit 0
    fi
fi

echo ""
echo "Signing app with identity: ${IDENTITY}"
echo ""

# Sign the executable
echo "Signing application..."
if ! codesign --force --deep --sign "${IDENTITY}" \
    --options runtime \
    --entitlements scripts/entitlements.plist \
    "${APP_BUNDLE}" 2>&1; then
    echo ""
    echo "❌ Signing failed!"
    exit 1
fi

echo ""
echo "Verifying signature..."
echo "-----------------------------------"

# Verify signature with detailed output
VERIFY_OUTPUT=$(codesign --verify --verbose "${APP_BUNDLE}" 2>&1)
VERIFY_EXIT=$?

if [ $VERIFY_EXIT -ne 0 ]; then
    echo "❌ Signature verification failed!"
    echo "$VERIFY_OUTPUT"
    exit 1
fi

echo "✓ Code signature verified"
echo ""

# Check signature details
echo "Signature details:"
echo "-----------------------------------"
codesign -dvv "${APP_BUNDLE}" 2>&1 | grep -E "Authority|Identifier|Format|Signed|TeamIdentifier" || true
echo ""

# Verify with spctl (Gatekeeper assessment)
echo "Gatekeeper assessment:"
echo "-----------------------------------"
SPCTL_OUTPUT=$(spctl --assess --verbose "${APP_BUNDLE}" 2>&1)
SPCTL_EXIT=$?

if [ $SPCTL_EXIT -eq 0 ]; then
    echo "✓ Gatekeeper assessment passed"
else
    echo "⚠ Gatekeeper assessment: $SPCTL_OUTPUT"
    echo "  (This is normal for apps not notarized yet)"
fi

echo ""
echo "=========================================="
echo "✅ App signed successfully!"
echo "=========================================="
echo ""
echo "Signed app: ${APP_BUNDLE}"
echo "Identity: ${IDENTITY}"
echo ""
