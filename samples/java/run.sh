#!/bin/bash

PLATFORM="$(uname -s)-$(uname -m)"
SCRIPT_DIR="$(realpath "$(dirname "${BASH_SOURCE[0]}")")"

ARTIFACT=""
VERSION="$(cat "${SCRIPT_DIR}/../DF_VERSION.txt")"
RUNTIME_DIR="${SCRIPT_DIR}/runtimes"

if [ "${PLATFORM}" == "Linux-x86_64" ]; then
    ARTIFACT="linux-intel-gcc-64"
    if [ -f "/etc/alpine-release" ]; then
        ARTIFACT="linux-intel-clang-musl-64"
    fi
elif [ "${PLATFORM}" == "Linux-ppc64le" ]; then
    ARTIFACT="linux-ppc64le-gcc-64"
elif [ "${PLATFORM}" == "Linux-ppc64le" ]; then
    ARTIFACT="linux-ppc64le-gcc-64"
elif [ "${PLATFORM}" == "Linux-aarch64" ]; then
    ARTIFACT="linux-aarch64-gcc-64"
    if [ -f "/etc/alpine-release" ]; then
        ARTIFACT="linux-aarch64-clang-musl-64"
    fi
elif [ "${PLATFORM}" == "Darwin-x86_64" ]; then
    ARTIFACT="macos-intel-clang-64"
elif [ "${PLATFORM}" == "Darwin-arm64" ]; then
    ARTIFACT="macos-arm64-clang-64"
elif [ "${PLATFORM}" == "Darwin-aarch64" ]; then
    ARTIFACT="macos-arm64-clang-64"
elif [ "${OS:-}" == "Windows_NT" ]; then
    ARTIFACT="windows-intel-msvc-64"
else
    echo "Unknown Platform: ${PLATFORM}"
    exit 1
fi

ARTIFACT_URL="https://github.com/HylandSoftware/DocumentFilters/releases/download/v${VERSION}/${ARTIFACT}.zip"
ASSETS_URL="https://github.com/HylandSoftware/DocumentFilters/releases/download/v${VERSION}/assets.zip"

if [ "${DOCFILTERS_RELEASES_URL}" != "" ]; then
    ARTIFACT_URL="${DOCFILTERS_RELEASES_URL}/${ARTIFACT}.zip"
fi

mkdir -p "${RUNTIME_DIR}"

if [ ! -d "${RUNTIME_DIR}/${ARTIFACT}" ]; then
    if [ ! -f "${RUNTIME_DIR}/${ARTIFACT}.zip" ];  then
        echo "Downloading native binaries from GitHub."
        echo "  - ${ARTIFACT_URL}"
        curl -sL -o "${RUNTIME_DIR}/${ARTIFACT}.zip" "${ARTIFACT_URL}" || (echo "failed to download" && exit 1)
    fi
    mkdir -p "${RUNTIME_DIR}/${ARTIFACT}"
    pushd "${RUNTIME_DIR}/${ARTIFACT}" > /dev/null || exit
    unzip "../${ARTIFACT}.zip" 2> /dev/null || true
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
    unzip  "../assets.zip" 2> /dev/null || true
    popd > /dev/null || exit    
fi

PROJECT="$1"
shift

export ISYS_FONTS="${SCRIPT_DIR}/runtimes/assets"

java -Djava.library.path="${SCRIPT_DIR}/runtimes/${ARTIFACT}" \
    -jar "${SCRIPT_DIR}/libs/${PROJECT}-1.0.0-SNAPSHOT.jar" \
    "$@"

