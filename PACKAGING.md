# macOS Packaging & Distribution Guide

Complete guide for building, signing, and distributing the Black Hole Simulation application for macOS.

## Quick Start

### One-Stop Script (Recommended)
```bash
# Build, sign, and package everything automatically
./sign_package.sh

# Or using Makefile:
make package
```

This script will:
1. Clean previous builds
2. Build the application
3. Create app bundle
4. Attempt to sign (if certificate available)
5. Create DMG file

### Step-by-Step Commands
```bash
# Build everything: clean, build, create app bundle, sign, and create DMG
make release

# Or individual steps:
make          # Build the application
make app      # Create app bundle
make sign     # Sign the app (requires certificate)
make dmg      # Create DMG file
```

## Prerequisites

1. **Xcode Command Line Tools** (for `codesign`, `hdiutil`, etc.)
   ```bash
   xcode-select --install
   ```

2. **Code Signing Certificate** (optional but recommended)
   - For distribution: Developer ID Application certificate
   - For testing: Development certificate
   - Get from: [Apple Developer Portal](https://developer.apple.com/account/)

## Build Outputs

All build outputs are placed in the `export/` folder:
- `export/blackhole_sim` - Compiled executable
- `export/BlackHoleSim.app` - App bundle
- `export/BlackHoleSim-1.0.dmg` - DMG file

## Building the App Bundle

### Step 1: Build the Application
```bash
make clean
make
```

This creates `export/blackhole_sim`.

### Step 2: Create App Bundle
```bash
make app
```

This creates `export/BlackHoleSim.app` with:
- Executable in `Contents/MacOS/`
- Assets in `Contents/Resources/`
- Info.plist with app metadata
- App icon (if available)

## Code Signing

### Find Your Signing Identity
```bash
security find-identity -v -p codesigning
```

### Sign the App

**Option 1: Using Environment Variable**
```bash
export MACOS_SIGNING_IDENTITY="Developer ID Application: Your Name (TEAM_ID)"
make sign
```

**Option 2: Automatic Detection**
The `sign_package.sh` script will automatically attempt to find a Developer ID certificate if `MACOS_SIGNING_IDENTITY` is not set.

**Option 3: Skip Signing**
If you don't have a certificate, the app will still work but macOS may show warnings. You can skip signing by not running `make sign`.

**Note**: For distribution outside the Mac App Store, you need a "Developer ID Application" certificate from Apple Developer.

## Creating DMG for Distribution

### Create DMG File
```bash
make dmg
```

This creates `export/BlackHoleSim-1.0.dmg` with:
- App bundle
- Applications folder symlink
- README (if available)
- Proper window layout

### Manual DMG Customization

The DMG script creates a basic DMG. To customize:

1. Edit `scripts/create_dmg.sh`
2. Add background image to `assets/dmg_background.png` (optional)
3. Customize window layout in the AppleScript section

## Notarization (Required for Gatekeeper)

After signing, you should notarize the app for distribution:

```bash
# 1. Create zip for notarization
cd export
ditto -c -k --keepParent BlackHoleSim.app BlackHoleSim.zip

# 2. Submit for notarization
xcrun notarytool submit BlackHoleSim.zip \
    --apple-id "your@email.com" \
    --team-id "TEAM_ID" \
    --password "app-specific-password" \
    --wait

# 3. Staple the ticket
xcrun stapler staple BlackHoleSim.app

# 4. Recreate DMG
cd ..
make dmg
```

## Testing

### Test App Bundle
```bash
open export/BlackHoleSim.app
```

### Test DMG
```bash
open export/BlackHoleSim-1.0.dmg
# Drag app to Applications folder
```

### Verify Signature
```bash
codesign --verify --verbose export/BlackHoleSim.app
spctl --assess --verbose export/BlackHoleSim.app
```

## File Structure

```
export/
├── blackhole_sim                    # Compiled executable
├── BlackHoleSim.app/               # App bundle
│   └── Contents/
│       ├── Info.plist              # App metadata
│       ├── MacOS/
│       │   └── BlackHoleSim         # Executable (copied from export/)
│       └── Resources/
│           ├── assets/              # Game assets
│           └── AppIcon.icns         # App icon
└── BlackHoleSim-1.0.dmg            # Distribution DMG
```

## Troubleshooting

### "App is damaged" Error
- Ensure app is properly signed
- Check code signature: `codesign --verify --verbose export/BlackHoleSim.app`
- May need notarization for distribution

### Icon Not Showing
- Ensure `assets/export/iOS-Default-1024x1024@1x.png` exists
- Check that `iconutil` is available (part of Xcode)

### Signing Fails
- Verify certificate is installed: `security find-identity -v -p codesigning`
- Check certificate hasn't expired
- Ensure you're using the correct identity format

### Build Errors
- Ensure all dependencies are installed via vcpkg
- Check that Xcode Command Line Tools are installed
- Verify Metal shader compilation succeeds

## Environment Variables

- `MACOS_SIGNING_IDENTITY`: Code signing identity (e.g., "Developer ID Application: Name (TEAM_ID)")

## Distribution Checklist

- [ ] Application builds successfully
- [ ] App bundle created and tested
- [ ] Application signed with Developer ID certificate
- [ ] Application notarized (for Gatekeeper)
- [ ] DMG created and tested
- [ ] DMG tested on clean macOS system
- [ ] Icon displays correctly
- [ ] All assets included in bundle

## Scripts Reference

- `sign_package.sh` - One-stop build, sign, and package script
- `scripts/create_app_bundle.sh` - App bundle creation script
- `scripts/sign_app.sh` - Code signing script
- `scripts/create_dmg.sh` - DMG creation script
- `scripts/entitlements.plist` - Code signing entitlements
