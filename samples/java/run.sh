#!/bin/bash

PLATFORM="$(uname -s)-$(uname -m)"
ARTIFACT=""
VERSION="23.3"
SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"
RUNTIME_DIR="${SCRIPT_DIR}/runtimes"

if [ "${PLATFORM}" == "Linux-x86_64" ]; then
    ARTIFACT="linux-intel-gcc-64"
elif [ "${PLATFORM}" == "Linux-ppc64le" ]; then
    ARTIFACT="linux-ppc64le-gcc-64"
elif [ "${PLATFORM}" == "Linux-ppc64le" ]; then
    ARTIFACT="linux-ppc64le-gcc-64"
elif [ "${PLATFORM}" == "Darwin-x86_64" ]; then
    ARTIFACT="macos-intel-clang-64"
elif [ "${PLATFORM}" == "Darwin-arm64" ]; then
    ARTIFACT="macos-arm64-clang-64"
elif [ "${PLATFORM}" == "Darwin-aarch64" ]; then
    ARTIFACT="macos-arm64-clang-64"
else
    echo "Unknown Platform: ${PLATFORM}"
    exit 1
fi

ARTIFACT_URL="https://github.com/HylandSoftware/DocumentFilters/releases/download/v23.3/${ARTIFACT}.zip"
ASSETS_URL="https://github.com/HylandSoftware/DocumentFilters/releases/download/v23.3/assets.zip"

mkdir -p "${RUNTIME_DIR}"

if [ ! -d "${RUNTIME_DIR}/${ARTIFACT}" ]; then
    if [ ! -f "${RUNTIME_DIR}/${ARTIFACT}.zip" ];  then
        echo "Downloading native binaries from GitHub."
        echo "  - ${ARTIFACT_URL}"
        curl -sL -o "${RUNTIME_DIR}/${ARTIFACT}.zip" "${ARTIFACT_URL}" || (echo "failed to download" && exit 1)
    fi
    mkdir -p "${RUNTIME_DIR}/${ARTIFACT}"
    pushd "${RUNTIME_DIR}/${ARTIFACT}" > /dev/null || exit
    unzip -qq "../${ARTIFACT}.zip" 2> /dev/null || true
    popd > /dev/null || exit
fi

if [ ! -d "${RUNTIME_DIR}/assets" ]; then
    if [ ! -f "${RUNTIME_DIR}/assets.zip" ];  then
        echo "Downloading fonts and assets from GitHub."
        echo "  - ${ASSETS_URL}"
        curl -sL -o "${RUNTIME_DIR}/assets.zip" "${ASSETS_URL}" || (echo "failed to download" && exit 1)
    fi
    mkdir -p "${RUNTIME_DIR}/assets"
    pushd "${RUNTIME_DIR}/assets" > /dev/null || exit
    unzip -qq "../assets.zip" 2> /dev/null || true
    popd > /dev/null || exit    
fi

PROJECT="$1"
shift

export ISYS_FONTS="runtimes/assets"

java -Djava.library.path="runtimes/${ARTIFACT}" \
    -jar "libs/${PROJECT}-1.0.0-SNAPSHOT.jar" \
    "$@"

