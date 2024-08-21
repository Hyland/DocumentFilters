$ARTIFACT = "windows-intel-msvc-64"
$VERSION = Get-Content "../DF_VERSION.txt"
$SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path
$RUNTIME_DIR = Join-Path $SCRIPT_DIR "runtimes"

$ARTIFACT_URL = "https://github.com/HylandSoftware/DocumentFilters/releases/download/v$VERSION/$ARTIFACT.zip"
$ASSETS_URL = "https://github.com/HylandSoftware/DocumentFilters/releases/download/v$VERSION/assets.zip"

if ($env:DOCFILTERS_RELEASES_URL -ne "") {
    $ARTIFACT_URL = "$($env:DOCFILTERS_RELEASES_URL)/$ARTIFACT.zip"
    $ASSETS_URL = "$($env:DOCFILTERS_RELEASES_URL)/assets.zip"
}

New-Item -ItemType Directory -Force -Path $RUNTIME_DIR | Out-Null
$global:ProgressPreference = 'SilentlyContinue'

if (-not (Test-Path "$RUNTIME_DIR\$ARTIFACT")) {
    if (-not (Test-Path "$RUNTIME_DIR\$ARTIFACT.zip")) {
        Write-Host "Downloading native binaries from GitHub."
        Write-Host "  - $ARTIFACT_URL"
        Invoke-WebRequest -Uri $ARTIFACT_URL -OutFile "$RUNTIME_DIR\$ARTIFACT.zip" -UseBasicParsing | Out-Null
    }
    New-Item -ItemType Directory -Force -Path "$RUNTIME_DIR\$ARTIFACT" | Out-Null
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::ExtractToDirectory("$RUNTIME_DIR\$ARTIFACT.zip", "$RUNTIME_DIR\$ARTIFACT")
}

if (-not (Test-Path "$RUNTIME_DIR\assets")) {
    if (-not (Test-Path "$RUNTIME_DIR\assets.zip")) {
        Write-Host "Downloading fonts and assets from GitHub."
        Write-Host "  - $ASSETS_URL"
        Invoke-WebRequest -Uri $ASSETS_URL -OutFile "$RUNTIME_DIR\assets.zip" -UseBasicParsing | Out-Null
    }
    New-Item -ItemType Directory -Force -Path "$RUNTIME_DIR\assets" | Out-Null
    [System.IO.Compression.ZipFile]::ExtractToDirectory("$RUNTIME_DIR\assets.zip", "$RUNTIME_DIR\assets")
}

$PROJECT = $args[0]
$scriptArgs = $args[1..($args.Length - 1)]
$RUNTIME = Join-Path $RUNTIME_DIR $ARTIFACT

$env:ISYS_FONTS = Join-Path $RUNTIME_DIR "assets"

java -D"java.library.path"="$RUNTIME" -jar "libs/${PROJECT}-1.0.0-SNAPSHOT.jar" $scriptArgs
