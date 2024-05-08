import os, platform, zipfile, urllib.request, fnmatch, DocumentFiltersLicense
from DocumentFilters import *

DF_VERSION='24.1'
with open('../DF_VERSION.txt', 'r') as version_file:
    DF_VERSION=version_file.read().strip()

def GetLicenseCode(settings):
    res = None
    try:
        res = settings.license_key
    except NameError:
        pass

    if res is None:
        res = DocumentFiltersLicense.LICENSE_KEY
    if res is None:
        res = ""
    return res

def GetRuntimePath(settings):
    res = None
    try:
        res = settings.library_path
    except NameError:
        pass

    res = os.environ.get("DF_PATH")
    if res is None:
        current_os = platform.system().lower()
        architecture = platform.machine().lower()
        artifact = None

        if f"{current_os}-{architecture}" == "windows-amd64":
            artifact="windows-intel-msvc-64"
        elif f"{current_os}-{architecture}" == "windows-x64":
            artifact="windows-intel-msvc-32"
        elif f"{current_os}-{architecture}" == "linux-x86_64":
            artifact="linux-intel-gcc-64"
        elif f"{current_os}-{architecture}" == "linux-x86":
            artifact="linux-intel-gcc-32"
        elif f"{current_os}-{architecture}" == "darwin-arm64":
            artifact="macos-arm64-clang-64"
        elif f"{current_os}-{architecture}" == "darwin-x86_64":
            artifact="macos-intel-clang-64"

        if artifact is not None:
            url=f'https://github.com/HylandSoftware/DocumentFilters/releases/download/v{DF_VERSION}/{artifact}.zip'
            dest_archive=f'runtimes/{DF_VERSION}/{artifact}.zip'
            dest_dir=f'runtimes/{DF_VERSION}/{artifact}'
            if not os.path.exists(dest_dir):
                print(f"Downloading Document Filters {DF_VERSION} binaries for {artifact} from GitHub...")
                os.makedirs(dest_dir, exist_ok=True)
                if not os.path.exists(dest_archive):
                    urllib.request.urlretrieve(url, dest_archive)
                with zipfile.ZipFile(dest_archive, 'r') as zip_ref:
                    zip_ref.extractall(dest_dir)

            res = os.path.realpath(dest_dir)
    return res

def GetFontsDir(settings):
    dest_dir = f'runtimes/{DF_VERSION}/assets'
    if not os.path.exists(dest_dir):
        url=f'https://github.com/HylandSoftware/DocumentFilters/releases/download/v{DF_VERSION}/assets.zip'
        dest_archive=f'runtimes/{DF_VERSION}/assets.zip'
        if not os.path.exists(dest_archive):
            print(f"Downloading Document Filters Fonts and Assets from GitHub...")
            urllib.request.urlretrieve(url, dest_archive)
            with zipfile.ZipFile(dest_archive, 'r') as zip_ref:
                zip_ref.extractall(dest_dir)

    return dest_dir

def InitializeAPI(api, settings):
    key = GetLicenseCode(settings)
    runtime_path = GetRuntimePath(settings)

    os.environ.setdefault('ISYS_FONTS', GetFontsDir(settings))
    api.Initialize(key, ".", runtime_path)

