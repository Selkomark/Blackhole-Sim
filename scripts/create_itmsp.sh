#!/bin/bash

# Script to create .itmsp package for App Store Connect upload
# .itmsp (iTunes Metadata Package) is the required format for Transporter

set -e

APP_NAME="Blackhole Sim"
APP_BUNDLE="export/${APP_NAME}.app"
BUNDLE_ID="com.blackhole.simulation"
VERSION="${APP_VERSION:-1.0}"  # Use APP_VERSION from env if set (for CI/CD)
ITMSP_NAME="${APP_NAME}.itmsp"
ITMSP_PATH="export/${ITMSP_NAME}"

echo "=========================================="
echo "Creating .itmsp package for App Store"
echo "=========================================="
echo ""

# Check if app bundle exists
if [ ! -d "$APP_BUNDLE" ]; then
    echo "❌ App bundle not found: $APP_BUNDLE"
    exit 1
fi

# Remove old .itmsp if exists
if [ -d "$ITMSP_PATH" ]; then
    echo "Removing old .itmsp package..."
    rm -rf "$ITMSP_PATH"
fi

# Create .itmsp directory
mkdir -p "$ITMSP_PATH"

# Create .pkg installer from app bundle (required for App Store uploads)
echo "Creating .pkg installer..."
PKG_FILE="${APP_NAME}.pkg"
PKG_PATH="$ITMSP_PATH/$PKG_FILE"

# Build the .pkg installer
# The app will be installed to /Applications
productbuild --component "$APP_BUNDLE" /Applications "$PKG_PATH"

if [ ! -f "$PKG_PATH" ]; then
    echo "❌ Failed to create .pkg installer"
    exit 1
fi

echo "✅ Created .pkg installer"

# Sign the .pkg with "3rd Party Mac Developer Installer" certificate if available
echo "Checking for installer certificate to sign .pkg..."
INSTALLER_LINE=$(security find-identity -v -p macappstore 2>/dev/null | grep "3rd Party Mac Developer Installer" | head -1 || true)

if [ -n "$INSTALLER_LINE" ]; then
    INSTALLER_HASH=$(echo "$INSTALLER_LINE" | awk '{print $2}')
    INSTALLER_NAME=$(echo "$INSTALLER_LINE" | sed 's/.*"\(.*\)"/\1/')
    
    echo "Found installer certificate: $INSTALLER_NAME"
    echo "Signing .pkg installer..."
    
    if productsign --sign "$INSTALLER_HASH" "$PKG_PATH" "${PKG_PATH}.signed" 2>&1; then
        if [ -f "${PKG_PATH}.signed" ]; then
            mv "${PKG_PATH}.signed" "$PKG_PATH"
            echo "✅ Signed .pkg installer"
            
            # Verify the signature
            if pkgutil --check-signature "$PKG_PATH" &>/dev/null; then
                echo "✅ Package signature verified"
            fi
        fi
    else
        echo "⚠️  Failed to sign .pkg (will be unsigned)"
    fi
else
    echo "⚠️  No installer certificate found - .pkg will be unsigned"
    echo "   For App Store uploads, the .pkg should be signed"
fi

# Create metadata.xml
# For macOS apps, reference the .pkg file (not the .app bundle)
echo "Creating metadata.xml..."
cat > "$ITMSP_PATH/metadata.xml" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<package version="software5.4" xmlns="http://apple.com/itunes/importer">
<software_assets>
<asset type="bundle">
<data_file>
<file_name>${PKG_FILE}</file_name>
</data_file>
</asset>
</software_assets>
</package>
EOF

echo "✅ Created .itmsp package: $ITMSP_PATH"
echo ""
echo "Contents:"
ls -lh "$ITMSP_PATH"
echo ""
echo "=========================================="
echo "Next Steps:"
echo "=========================================="
echo ""
echo "1. Open Transporter.app:"
echo "   open -a Transporter"
echo ""
echo "2. Drag and drop this folder into Transporter:"
echo "   $(pwd)/$ITMSP_PATH"
echo ""
echo "3. Click 'Deliver'"
echo ""
echo "Or open Transporter with the .itmsp directly:"
echo "   open -a Transporter \"$(pwd)/$ITMSP_PATH\""
echo ""

