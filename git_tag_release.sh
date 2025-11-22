#!/bin/bash

# Script to tag a release and push it to the repository

set -e

# Get the version from the Makefile
VERSION="$1"

if [ -z "$VERSION" ]; then
  echo "Usage: $0 <version>"
  echo "Example: $0 1.0.1"
  exit 1
fi

TAG_NAME="prod-v$VERSION"

# Check if tag exists locally and delete it
if git rev-parse "$TAG_NAME" >/dev/null 2>&1; then
  echo "Tag '$TAG_NAME' exists locally. Deleting..."
  git tag -d "$TAG_NAME"
fi

# Check if tag exists on remote and delete it
if git ls-remote --tags origin "$TAG_NAME" | grep -q "$TAG_NAME"; then
  echo "Tag '$TAG_NAME' exists on remote. Deleting..."
  git push origin --delete "$TAG_NAME" || git push origin ":refs/tags/$TAG_NAME"
fi

# Create the tag
echo "Creating tag '$TAG_NAME'..."
git tag -a "$TAG_NAME" -m "Release $VERSION"

# Push the tag
echo "Pushing tag '$TAG_NAME' to origin..."
git push origin "$TAG_NAME"
