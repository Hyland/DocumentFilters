"""Download and locate the Document Filters native runtime binaries."""

import os
import platform
import sys
import urllib.request
import zipfile
from pathlib import Path

from document_filters_cli import __version__ as _PKG_VERSION

# DF_VERSION env var overrides the package version (useful for testing a newer runtime
# without reinstalling the CLI).
DF_VERSION = os.environ.get("DF_VERSION", _PKG_VERSION)
RELEASES_BASE = f"https://github.com/Hyland/DocumentFilters/releases/download/v{DF_VERSION}"
CACHE_DIR = Path.home() / ".document-filters" / "runtimes" / DF_VERSION


def _artifact_name():
    system = platform.system().lower()
    machine = platform.machine().lower()

    musl = (
        hasattr(sys, "implementation")
        and hasattr(sys.implementation, "_multiarch")
        and "musl" in sys.implementation._multiarch
    )

    key = f"{system}-{machine}"
    if system.startswith("cygwin_nt"):
        key = f"windows-{machine}"

    mapping = {
        "windows-amd64":   "windows-intel-msvc-64",
        "windows-x86_64":  "windows-intel-msvc-64",
        "windows-x64":     "windows-intel-msvc-64",
        "windows-i386":    "windows-intel-msvc-32",
        "windows-x86":     "windows-intel-msvc-32",
        "linux-x86_64":    "linux-intel-clang-musl-64" if musl else "linux-intel-gcc-64",
        "linux-x86":       "linux-intel-gcc-32",
        "linux-i386":      "linux-intel-gcc-32",
        "linux-aarch64":   "linux-aarch64-clang-musl-64" if musl else "linux-aarch64-gcc-64",
        "linux-armv7l":    "linux-armv7l-gcc-32",
        "linux-ppc64le":   "linux-ppc64le-gcc-64",
        "darwin-arm64":    "macos-arm64-clang-64",
        "darwin-x86_64":   "macos-intel-clang-64",
        "freebsd-amd64":   "freebsd-intel-clang-64",
        "freebsd-x86_64":  "freebsd-intel-clang-64",
        "freebsd-i386":    "freebsd-intel-clang-32",
    }
    artifact = mapping.get(key)
    if artifact is None:
        raise RuntimeError(
            f"Unsupported platform: {system}-{machine}. "
            "Check https://github.com/Hyland/DocumentFilters/releases for available binaries."
        )
    return artifact


def runtime_dir(quiet=False):
    """
    Return the path to the native runtime directory, downloading it first if needed.
    Caches to ~/.document-filters/runtimes/<version>/<artifact>/
    """
    override = os.environ.get("DF_RUNTIME_PATH")
    if override:
        return override

    artifact = _artifact_name()
    dest_dir = CACHE_DIR / artifact

    if not dest_dir.exists():
        url = os.environ.get("DOCFILTERS_RELEASES_URL", RELEASES_BASE) + f"/{artifact}.zip"
        archive = CACHE_DIR / f"{artifact}.zip"
        CACHE_DIR.mkdir(parents=True, exist_ok=True)

        if not quiet:
            print(f"Downloading Document Filters {DF_VERSION} runtime for {artifact}...")
            print(f"  From: {url}")
            print(f"  To:   {archive}")

        def _progress(count, block, total):
            if quiet or total <= 0:
                return
            pct = min(int(count * block * 100 / total), 100)
            print(f"\r  {pct}%", end="", flush=True)

        urllib.request.urlretrieve(url, archive, reporthook=_progress)
        if not quiet:
            print()

        if not quiet:
            print(f"  Extracting to {dest_dir}...")
        dest_dir.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(archive, "r") as zf:
            zf.extractall(dest_dir)
        archive.unlink(missing_ok=True)

        if not quiet:
            print("  Done.")

    return str(dest_dir)


def make_api(quiet=False):
    """Initialize and return a DocumentFilters API instance, downloading runtime if needed."""
    from DocumentFilters import DocumentFilters

    dll_path = runtime_dir(quiet=quiet)
    license_key = os.environ.get("DF_LICENSE_KEY", "")

    api = DocumentFilters()
    api.Initialize(license_key, ".", dll_path)
    return api
