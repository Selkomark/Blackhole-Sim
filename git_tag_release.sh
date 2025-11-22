#!/bin/bash

# Script to tag a release and push it to the repository

set -e

# Get the version from the Makefile
VERSION=$(make -f Makefile print-version)

# Tag the release
if git rev-parse "prod-v$VERSION" >/dev/null 2>&1; then
  git tag -a "prod-v$VERSION" -m "Release $VERSION"
fi

git push origin "prod-v$VERSION"
