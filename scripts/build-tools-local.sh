#!/bin/bash
set -e

# ============================================================
# Local Build Script for STC Arduino Core Tool Archives
# ============================================================
# This script downloads SDCC and builds stcgal standalone
# binaries, then packages them as release assets.
#
# Usage:
#   ./scripts/build-tools-local.sh [OPTIONS]
#
# Options:
#   --sdcc-only       Only build SDCC archives
#   --stcgal-only     Only build stcgal archives
#   --skip-windows    Skip Windows stcgal (requires Windows)
#   --skip-linux      Skip Linux stcgal (requires Docker)
#   --help            Show this help
#
# Requirements:
#   - curl, tar, zip/unzip
#   - Python 3 + pip (for stcgal)
#   - Docker (for Linux stcgal binary, optional)
#   - 7z (for extracting Windows SDCC installer, via: brew install p7zip)
# ============================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build-tools"
OUTPUT_DIR="${PROJECT_DIR}/release-assets"

# Versions (match package_stc8051_index.json)
SDCC_VERSION="4.5.0"
SDCC_LABEL_VERSION="4.5.13"
STCGAL_LABEL_VERSION="1.0.0"

# Parse arguments
BUILD_SDCC=true
BUILD_STCGAL=true
BUILD_WINDOWS_STCGAL=true
BUILD_LINUX_STCGAL=true

for arg in "$@"; do
  case $arg in
    --sdcc-only)     BUILD_STCGAL=false ;;
    --stcgal-only)   BUILD_SDCC=false ;;
    --skip-windows)  BUILD_WINDOWS_STCGAL=false ;;
    --skip-linux)    BUILD_LINUX_STCGAL=false ;;
    --help)
      head -25 "$0" | tail -20
      exit 0
      ;;
    *)
      echo "Unknown option: $arg"
      exit 1
      ;;
  esac
done

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

info()  { echo -e "${BLUE}[INFO]${NC} $*"; }
ok()    { echo -e "${GREEN}[OK]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; }

# Setup directories
mkdir -p "$BUILD_DIR" "$OUTPUT_DIR"

# ──────────────────────────────────────────────────────────────
# Helper: compute checksum (works on macOS and Linux)
# ──────────────────────────────────────────────────────────────
compute_sha256() {
  if command -v sha256sum &>/dev/null; then
    sha256sum "$1" | awk '{print $1}'
  else
    shasum -a 256 "$1" | awk '{print $1}'
  fi
}

file_size() {
  if [[ "$(uname)" == "Darwin" ]]; then
    stat -f%z "$1"
  else
    stat -c%s "$1"
  fi
}

print_archive_info() {
  local file="$1"
  local sha=$(compute_sha256 "$file")
  local size=$(file_size "$file")
  echo ""
  ok "Archive: $(basename "$file")"
  echo "   SHA-256: ${sha}"
  echo "   Size:    ${size}"
}

# ──────────────────────────────────────────────────────────────
# SDCC for Linux
# ──────────────────────────────────────────────────────────────
build_sdcc_linux() {
  info "Building SDCC Linux archive..."
  local WORK="${BUILD_DIR}/sdcc-linux"
  rm -rf "$WORK" && mkdir -p "$WORK"

  # Download
  local URL="https://sourceforge.net/projects/sdcc/files/sdcc-linux-amd64/${SDCC_VERSION}/sdcc-${SDCC_VERSION}-amd64-unknown-linux2.5.tar.bz2/download"
  local DOWNLOAD="${WORK}/sdcc-linux.tar.bz2"

  if [ ! -f "$DOWNLOAD" ]; then
    info "Downloading SDCC ${SDCC_VERSION} for Linux..."
    curl -L -o "$DOWNLOAD" "$URL"
  else
    info "Using cached download: $DOWNLOAD"
  fi

  # Extract
  info "Extracting..."
  tar -xjf "$DOWNLOAD" -C "$WORK"
  local SDCC_DIR=$(find "$WORK" -maxdepth 1 -type d -name "sdcc*" ! -name "sdcc-linux" | head -1)
  info "Found: $SDCC_DIR"

  # Repackage with expected directory structure
  local PKG="${WORK}/repackaged"
  mkdir -p "$PKG/bin" "$PKG/share/sdcc/include" "$PKG/share/sdcc/lib"

  # Copy binaries
  for bin in sdcc sdar sdas8051 sdld packihx sdnm sdobjcopy; do
    [ -f "$SDCC_DIR/bin/$bin" ] && cp "$SDCC_DIR/bin/$bin" "$PKG/bin/"
  done
  chmod +x "$PKG/bin/"* 2>/dev/null || true

  # Copy includes and libraries
  [ -d "$SDCC_DIR/share/sdcc/include" ] && cp -r "$SDCC_DIR/share/sdcc/include/"* "$PKG/share/sdcc/include/"
  [ -d "$SDCC_DIR/share/sdcc/lib" ]     && cp -r "$SDCC_DIR/share/sdcc/lib/"* "$PKG/share/sdcc/lib/"
  [ -d "$SDCC_DIR/share/sdcc/non-free" ] && cp -r "$SDCC_DIR/share/sdcc/non-free" "$PKG/share/sdcc/"

  # Create archive
  local ARCHIVE="${OUTPUT_DIR}/sdcc-${SDCC_LABEL_VERSION}-linux-x86_64.tar.gz"
  (cd "$PKG" && tar -czf "$ARCHIVE" .)
  print_archive_info "$ARCHIVE"
}

# ──────────────────────────────────────────────────────────────
# SDCC for Windows
# ──────────────────────────────────────────────────────────────
build_sdcc_windows() {
  info "Building SDCC Windows archive..."
  local WORK="${BUILD_DIR}/sdcc-windows"
  rm -rf "$WORK" && mkdir -p "$WORK"

  # Download the Windows installer
  local URL="https://sourceforge.net/projects/sdcc/files/sdcc-win64/${SDCC_VERSION}/sdcc-${SDCC_VERSION}-x64-setup.exe/download"
  local DOWNLOAD="${WORK}/sdcc-setup.exe"

  if [ ! -f "$DOWNLOAD" ]; then
    info "Downloading SDCC ${SDCC_VERSION} for Windows..."
    curl -L -o "$DOWNLOAD" "$URL"
  else
    info "Using cached download: $DOWNLOAD"
  fi

  # Extract using 7z (NSIS installer can be extracted with 7z)
  if ! command -v 7z &>/dev/null; then
    error "7z not found! Install with: brew install p7zip"
    error "Skipping Windows SDCC build."
    return 1
  fi

  info "Extracting Windows installer with 7z..."
  local EXTRACT="${WORK}/extracted"
  mkdir -p "$EXTRACT"
  7z x -o"$EXTRACT" "$DOWNLOAD" -y > /dev/null

  # The NSIS installer extracts with a specific structure
  # Find where the files landed
  info "Extracted contents:"
  ls "$EXTRACT/"

  # Repackage with expected directory structure (Windows: no share/sdcc/ prefix)
  local PKG="${WORK}/repackaged"
  mkdir -p "$PKG/bin" "$PKG/include" "$PKG/lib"

  # Copy binaries
  for bin in sdcc.exe sdar.exe sdas8051.exe sdld.exe packihx.exe sdnm.exe sdobjcopy.exe; do
    local found=$(find "$EXTRACT" -name "$bin" -type f 2>/dev/null | head -1)
    if [ -n "$found" ]; then
      cp "$found" "$PKG/bin/"
      info "  Copied: $bin"
    else
      warn "  Not found: $bin"
    fi
  done

  # Copy includes
  local INC_DIR=$(find "$EXTRACT" -type d -name "include" | head -1)
  if [ -n "$INC_DIR" ]; then
    cp -r "$INC_DIR/"* "$PKG/include/" 2>/dev/null || true
  fi

  # Copy libraries
  local LIB_DIR=$(find "$EXTRACT" -type d -name "lib" | head -1)
  if [ -n "$LIB_DIR" ]; then
    cp -r "$LIB_DIR/"* "$PKG/lib/" 2>/dev/null || true
  fi

  # Copy non-free if present
  local NONFREE_DIR=$(find "$EXTRACT" -type d -name "non-free" | head -1)
  if [ -n "$NONFREE_DIR" ]; then
    cp -r "$NONFREE_DIR" "$PKG/"
  fi

  # Create zip archive
  local ARCHIVE="${OUTPUT_DIR}/sdcc-${SDCC_LABEL_VERSION}-windows-x86_64.zip"
  (cd "$PKG" && zip -r "$ARCHIVE" . -q)
  print_archive_info "$ARCHIVE"
}

# ──────────────────────────────────────────────────────────────
# stcgal for Linux (via Docker)
# ──────────────────────────────────────────────────────────────
build_stcgal_linux() {
  info "Building stcgal Linux binary via Docker..."

  if ! command -v docker &>/dev/null; then
    error "Docker not found! Install Docker Desktop for Mac."
    error "Skipping Linux stcgal build."
    return 1
  fi

  local WORK="${BUILD_DIR}/stcgal-linux"
  rm -rf "$WORK" && mkdir -p "$WORK"

  # Create a Dockerfile for building stcgal
  cat > "$WORK/Dockerfile" << 'DOCKERFILE_EOF'
FROM python:3.11-slim

RUN pip install --no-cache-dir stcgal pyinstaller pyserial

RUN STCGAL_SCRIPT=$(which stcgal) && \
    pyinstaller \
      --onefile \
      --name stcgal \
      --hidden-import serial \
      --hidden-import serial.tools \
      --hidden-import serial.tools.list_ports \
      --hidden-import stcgal.protocols \
      --hidden-import stcgal.models \
      --hidden-import stcgal.utils \
      --collect-all stcgal \
      "$STCGAL_SCRIPT"

RUN mkdir -p /output && \
    cp /dist/stcgal /output/ && \
    chmod +x /output/stcgal
DOCKERFILE_EOF

  # Build Docker image
  info "Building Docker image (this may take a few minutes)..."
  docker build -t stcgal-builder "$WORK"

  # Extract the binary
  info "Extracting stcgal binary from Docker..."
  docker create --name stcgal-extract stcgal-builder /bin/true
  docker cp stcgal-extract:/output/stcgal "$WORK/stcgal"
  docker rm stcgal-extract
  docker rmi stcgal-builder 2>/dev/null || true

  # Create wrapper script
  mkdir -p "$WORK/package"
  cp "$WORK/stcgal" "$WORK/package/"
  chmod +x "$WORK/package/stcgal"

  echo '#!/bin/bash' > "$WORK/package/stcgal.sh"
  echo '# stcgal wrapper for Arduino IDE' >> "$WORK/package/stcgal.sh"
  echo 'SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"' >> "$WORK/package/stcgal.sh"
  echo '"${SCRIPT_DIR}/stcgal" "$@"' >> "$WORK/package/stcgal.sh"
  chmod +x "$WORK/package/stcgal.sh"

  # Create archive
  local ARCHIVE="${OUTPUT_DIR}/stcgal-${STCGAL_LABEL_VERSION}-linux-x86_64.tar.gz"
  (cd "$WORK/package" && tar -czf "$ARCHIVE" .)
  print_archive_info "$ARCHIVE"
}

# ──────────────────────────────────────────────────────────────
# stcgal for Windows (via Docker + Wine)
# ──────────────────────────────────────────────────────────────
build_stcgal_windows() {
  info "Building stcgal Windows binary via Docker + Wine..."

  if ! command -v docker &>/dev/null; then
    error "Docker not found! Install Docker Desktop for Mac."
    error "Skipping Windows stcgal build."
    return 1
  fi

  local WORK="${BUILD_DIR}/stcgal-windows"
  rm -rf "$WORK" && mkdir -p "$WORK"

  # Use a Docker image with Wine + Python to cross-compile for Windows
  cat > "$WORK/Dockerfile" << 'DOCKERFILE_EOF'
FROM tobix/pywine:3.11

# Install stcgal and PyInstaller in the Wine Python environment
RUN wine pip install stcgal pyinstaller pyserial 2>/dev/null; \
    wineserver -w

# Build the Windows executable
RUN STCGAL_SCRIPT=$(wine python -c "import shutil; print(shutil.which('stcgal'))" 2>/dev/null | tr -d '\r') && \
    wine pyinstaller \
      --onefile \
      --name stcgal \
      --hidden-import serial \
      --hidden-import serial.tools \
      --hidden-import serial.tools.list_ports \
      --hidden-import stcgal.protocols \
      --hidden-import stcgal.models \
      --hidden-import stcgal.utils \
      --collect-all stcgal \
      "$STCGAL_SCRIPT" 2>/dev/null; \
    wineserver -w

RUN mkdir -p /output && \
    cp /dist/stcgal.exe /output/ 2>/dev/null || \
    cp /home/user/dist/stcgal.exe /output/ 2>/dev/null || \
    find / -name "stcgal.exe" -exec cp {} /output/ \; 2>/dev/null
DOCKERFILE_EOF

  # Build Docker image
  info "Building Docker image with Wine (this may take several minutes)..."
  if docker build -t stcgal-win-builder "$WORK" 2>&1; then
    # Extract the exe
    info "Extracting stcgal.exe from Docker..."
    docker create --name stcgal-win-extract stcgal-win-builder /bin/true
    docker cp stcgal-win-extract:/output/stcgal.exe "$WORK/stcgal.exe"
    docker rm stcgal-win-extract
    docker rmi stcgal-win-builder 2>/dev/null || true

    # Create wrapper batch file
    mkdir -p "$WORK/package"
    cp "$WORK/stcgal.exe" "$WORK/package/"

    printf '@echo off\r\nREM stcgal wrapper for Arduino IDE\r\n"%%~dp0stcgal.exe" %%*\r\n' > "$WORK/package/stcgal.bat"

    # Create zip archive
    local ARCHIVE="${OUTPUT_DIR}/stcgal-${STCGAL_LABEL_VERSION}-windows-x86_64.zip"
    (cd "$WORK/package" && zip -r "$ARCHIVE" . -q)
    print_archive_info "$ARCHIVE"
  else
    warn "Docker Wine build failed. For Windows stcgal, consider:"
    warn "  1. Use the GitHub Actions workflow instead (just for stcgal-windows)"
    warn "  2. Build on a Windows machine with: pip install stcgal pyinstaller && pyinstaller --onefile stcgal"
    return 1
  fi
}

# ──────────────────────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────────────────────
echo ""
echo "=========================================="
echo "  STC Arduino Core - Tool Archive Builder"
echo "=========================================="
echo "  SDCC version:    ${SDCC_VERSION} (labeled ${SDCC_LABEL_VERSION})"
echo "  stcgal version:  ${STCGAL_LABEL_VERSION}"
echo "  Output dir:      ${OUTPUT_DIR}"
echo "=========================================="
echo ""

FAILED=()

if [ "$BUILD_SDCC" = true ]; then
  build_sdcc_linux  || FAILED+=("sdcc-linux")
  build_sdcc_windows || FAILED+=("sdcc-windows")
fi

if [ "$BUILD_STCGAL" = true ]; then
  if [ "$BUILD_LINUX_STCGAL" = true ]; then
    build_stcgal_linux || FAILED+=("stcgal-linux")
  fi
  if [ "$BUILD_WINDOWS_STCGAL" = true ]; then
    build_stcgal_windows || FAILED+=("stcgal-windows")
  fi
fi

echo ""
echo "=========================================="
echo "  BUILD SUMMARY"
echo "=========================================="
echo ""

if [ -d "$OUTPUT_DIR" ] && ls "$OUTPUT_DIR"/*.{tar.gz,zip} &>/dev/null 2>&1; then
  ok "Successfully built archives:"
  echo ""
  for f in "$OUTPUT_DIR"/*; do
    if [ -f "$f" ]; then
      local_sha=$(compute_sha256 "$f")
      local_size=$(file_size "$f")
      echo "  $(basename "$f")"
      echo "    checksum: \"SHA-256:${local_sha}\""
      echo "    size:     \"${local_size}\""
      echo ""
    fi
  done
fi

if [ ${#FAILED[@]} -gt 0 ]; then
  warn "Failed builds: ${FAILED[*]}"
fi

echo ""
echo "=========================================="
echo "  NEXT STEPS"
echo "=========================================="
echo ""
echo "1. Update package_stc8051_index.json with the checksums and sizes above"
echo "2. Upload the archives to GitHub Release v1.0.1:"
echo "   gh release create v1.0.1 ${OUTPUT_DIR}/* --title 'STC Arduino Core v1.0.1'"
echo "3. Commit and push the updated package_stc8051_index.json"
echo "=========================================="
