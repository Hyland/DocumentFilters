---
applyTo: "**"
---

# Hyland Document Filters SDK Guide

Comprehensive documentation for the Hyland Document Filters SDK — a powerful document processing toolkit for file inspection, text extraction, content conversion, and document comparison across 600+ file formats.

---

## Table of Contents

1. [Overview](#overview)
2. [Licensing](#licensing)
3. [Installation](#installation)
   - [Python Bindings](#python-bindings)
   - [Native Runtime Binaries](#native-runtime-binaries)
   - [Assets & Fonts](#assets--fonts)
4. [Initialization](#initialization)
5. [Core Capabilities](#core-capabilities)
   - [File Type Identification](#file-type-identification)
   - [Text Extraction](#text-extraction)
   - [HTML Conversion (High-Fidelity)](#html-conversion-high-fidelity)
   - [PDF Conversion](#pdf-conversion)
   - [Image Rendering (PNG/TIFF)](#image-rendering-pngtiff)
   - [Document Comparison](#document-comparison)
   - [Metadata Extraction](#metadata-extraction)
   - [Sub-File Extraction](#sub-file-extraction)
6. [Python API Reference](#python-api-reference)
   - [DocumentFilters Class](#documentfilters-class)
   - [Extractor](#extractor)
   - [Pages & Canvas](#pages--canvas)
   - [Compare API](#compare-api)
   - [Constants & Flags](#constants--flags)
7. [Canvas Options](#canvas-options)
8. [Environment Variables](#environment-variables)
9. [Supported Platforms](#supported-platforms)
10. [Best Practices & Pitfalls](#best-practices--pitfalls)
11. [Error Handling](#error-handling)
12. [Code Examples](#code-examples)
13. [Troubleshooting](#troubleshooting)

---

## Overview

Hyland Document Filters is an SDK that provides developers with tools to embed rich document processing capabilities into applications. It operates as a local library (not a REST API) with native binaries for each platform and language bindings for Python, C#, Java, and C/C++.

### Key Features

- **Deep Content Inspection** — Identify file types and extract text, metadata, and structure from 600+ formats
- **High-Fidelity Rendering** — Convert documents to HTML5, PDF, PNG, TIFF, and other output formats
- **Document Comparison** — Word-level comparison of two documents with bounding box location data
- **Content Transformation** — Combine pages from multiple documents, apply watermarks, redactions, and annotations
- **OCR Support** — Optical Character Recognition for scanned documents and images
- **Markdown & JSON Output** — Convert documents to Markdown and structured MDAST JSON
- **Cross-Platform** — Runs on Windows, Linux (x64/ARM), macOS (Intel/ARM), and FreeBSD

### Resources

| Resource                 | URL                                                                                                                    |
| ------------------------ | ---------------------------------------------------------------------------------------------------------------------- |
| Home Page                | https://www.documentfilters.com                                                                                        |
| Documentation            | https://hyland.github.io/DocumentFilters-Docs/latest/                                                                  |
| API Reference            | https://hyland.github.io/DocumentFilters-Docs/latest/reference.html                                                    |
| Python Getting Started   | https://hyland.github.io/DocumentFilters-Docs/latest/getting_started_with_document_filters/getting_started_python.html |
| GitHub (Public)          | https://github.com/Hyland/DocumentFilters                                                                              |
| GitHub (Internal Source) | https://github.com/HylandExperience/cin-docfilters                                                                     |
| Release Notes            | https://hyland.github.io/DocumentFilters-Docs/latest/release_notes/index.html                                          |
| Security Hub             | https://hyland.github.io/DocumentFilters-SecurityHub/                                                                  |
| Supported Formats        | https://hyland.github.io/DocumentFilters-Docs/latest/supported_platforms.html                                          |

---

## Licensing

Document Filters supports three licensing modes:

| Mode                           | License Key         | Features | Expiration |
| ------------------------------ | ------------------- | -------- | ---------- |
| **Full Featured**              | REQUIRED            | ALL      | _Optional_ |
| **Time-Limited Evaluation**    | REQUIRED            | ALL      | REQUIRED   |
| **Feature-Limited Evaluation** | NONE (empty string) | LIMITED  | NONE       |

### Feature-Limited Evaluation

When no license key is provided (empty string `""`), the following limitations apply:

- **Supported input formats** (for processing beyond file identification): Text, PDF, Microsoft Office (Word, Excel, PowerPoint), OpenDocument Format (ODF)
- **Supported output formats**: Text, PDF, Raster images (PNG, JPEG, GIF, WEBP, BMP, TIFF), Markdown
- **Watermark**: Each page of output contains an "Evaluation Copy" watermark

File type identification (`getFileType()`) is **not** restricted in evaluation mode.

### Requesting a License

- **Evaluation license**: Email `DocFiltersEval@hyland.com`
- **Full license**: Contact [Hyland Sales](https://www.hyland.com/en/company/contact-us) and enter into an OEM Agreement

### License Key Configuration

Store the license key in an environment variable and load it at initialization:

```env
DF_LICENSE_KEY=XXXXX XXXXX XXXXX XXXXX XXXXX XXXXX
```

---

## Installation

### Python Bindings

Install from GitHub via pip:

```bash
pip install DocumentFilters@git+https://github.com/Hyland/DocumentFilters.git@v26.1.0#subdirectory=bindings/python
```

Or add to `requirements.txt`:

```text
DocumentFilters @ git+https://github.com/Hyland/DocumentFilters.git@v26.1.0#subdirectory=bindings/python
```

Replace `v26.1.0` with the desired version tag (latest release: `v26.1.0`).

### Native Runtime Binaries

The Python package does **not** bundle native shared libraries. These must be downloaded separately and provided at initialization time via the `dll_path` parameter.

Binaries are available as GitHub Release assets:

```
https://github.com/Hyland/DocumentFilters/releases/download/v{VERSION}/{ARTIFACT}.zip
```

#### Platform Artifact Names

| Platform | Architecture          | Artifact Name                 |
| -------- | --------------------- | ----------------------------- |
| Windows  | x64                   | `windows-intel-msvc-64`       |
| Windows  | x86                   | `windows-intel-msvc-32`       |
| Linux    | x64 (glibc)           | `linux-intel-gcc-64`          |
| Linux    | x64 (musl)            | `linux-intel-clang-musl-64`   |
| Linux    | ARM64 (glibc)         | `linux-aarch64-gcc-64`        |
| Linux    | ARM64 (musl)          | `linux-aarch64-clang-musl-64` |
| macOS    | ARM64 (Apple Silicon) | `macos-arm64-clang-64`        |
| macOS    | x64 (Intel)           | `macos-intel-clang-64`        |

It is recommended to cache downloaded binaries locally (e.g., in `runtimes/{VERSION}/{ARTIFACT}/`) to avoid re-downloading.

### Assets & Fonts

Font assets are also required for rendering and are downloaded separately:

```
https://github.com/Hyland/DocumentFilters/releases/download/v{VERSION}/assets.zip
```

Extracted to `runtimes/{VERSION}/assets/` and referenced via the `ISYS_FONTS` and `ISYS_ASSETS` environment variables.

---

## Initialization

The Document Filters engine must be initialized once before use. The three required inputs are:

1. **License key** — string (empty for evaluation mode)
2. **Working directory** — typically `"."`
3. **Runtime path** — path to the directory containing native binaries

```python
from DocumentFilters import DocumentFilters

api = DocumentFilters()

# Set font/asset paths
os.environ['ISYS_FONTS'] = '/path/to/assets'
os.environ['ISYS_ASSETS'] = '/path/to/assets'

# Initialize the engine
api.Initialize(license_key, ".", "/path/to/runtime/binaries")
```

### Recommended Initialization Steps

1. Read `DF_LICENSE_KEY` from environment variables
2. Download runtime binaries for the current platform if not cached locally
3. Download font assets if not cached locally
4. Set `ISYS_FONTS` and `ISYS_ASSETS` environment variables
5. Call `api.Initialize(license_key, ".", runtime_path)`

A singleton or module-level instance is recommended since the engine only needs to be initialized once.

---

## Core Capabilities

### File Type Identification

Identify a file's format without fully opening it. Works on 600+ formats and is unrestricted in evaluation mode.

```python
with api.GetExtractor("document.pdf") as extractor:
    file_type_id = extractor.getFileType()                        # Numeric type ID
    file_type_name = extractor.getFileType(IGR_FORMAT_LONG_NAME)  # Human-readable name
    supports_text = extractor.getSupportsText()                   # Boolean
    supports_subfiles = extractor.getSupportsSubFiles()           # Boolean (e.g., ZIP, email)
```

### Text Extraction

Extract plain text from documents.

```python
with api.GetExtractor("document.pdf") as extractor:
    if extractor.getSupportsText():
        extractor.Open(IGR_BODY_AND_META)
        while not extractor.getEOF():
            text = extractor.GetText(4096, stripControlCodes=True)
            print(text)
```

Check `getSupportsText()` before the loop — rendering-only formats (e.g., scanned TIFF without OCR) open successfully but return no text. Without the guard, callers get an empty result with no indication of failure.

### Idiomatic Short-Form: `OpenExtractor()`

`api.OpenExtractor(source, mode, options)` combines `GetExtractor` and `Open` in one call and is the preferred form when no per-file `Open` customisation is needed:

```python
with api.OpenExtractor("document.pdf", IGR_BODY_AND_META) as extractor:
    if extractor.getSupportsText():
        while not extractor.getEOF():
            print(extractor.GetText(4096))
```

The `callback` parameter on `OpenExtractor` is the only path to supply a password callback before `Open()` is invoked (see Error Handling below).

### HTML Conversion (High-Fidelity)

Convert documents to high-definition, paginated HTML5 with inline images. **Requires a license** for full HTML output (evaluation mode only supports PDF and raster output).

```python
with api.GetExtractor("document.pdf") as extractor:
    extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

    with api.MakeOutputCanvas("output.html", IGR_DEVICE_HTML, "HTML_INLINE_IMAGES=on") as canvas:
        for page in extractor.Pages:
            canvas.RenderPage(page)
```

### PDF Conversion

```python
with api.GetExtractor("document.docx") as extractor:
    extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

    with api.MakeOutputCanvas("output.pdf", IGR_DEVICE_IMAGE_PDF, "") as canvas:
        for page in extractor.Pages:
            canvas.RenderPage(page)
```

### Image Rendering (PNG/TIFF)

Render individual pages as images.

```python
with api.GetExtractor("document.pdf") as extractor:
    extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

    for i, page in enumerate(extractor.Pages):
        with api.MakeOutputCanvas(f"page_{i}.png", IGR_DEVICE_IMAGE_PNG, "") as canvas:
            canvas.RenderPage(page)
```

### Document Comparison

Compare two documents word-by-word and get difference details with bounding box locations. **Requires a license.**

```python
compare_settings = DocumentFilters.CompareSettings()
compare_settings.CompareType = IGR_COMPARE_DOCUMENTS_COMPARE_WORDS
compare_settings.Flags = 0  # Add flags as needed (see Constants section)

with api.GetExtractor("original.pdf") as left:
    left.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

    with api.GetExtractor("revised.pdf") as right:
        right.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

        with left.Compare(otherDocument=right, compareSettings=compare_settings) as results:
            while results.MoveNext():
                diff = results.Current

                if diff.Type in (IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL,
                                 IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH):
                    continue

                print(f"Type: {diff.Type}")
                print(f"Original Page: {diff.OriginalPageIndex + 1}")
                print(f"Revised Page: {diff.RevisedPageIndex + 1}")
                print(f"Text: {diff.GetText()}")

                for hit in diff.Details:
                    print(f"  Part: '{hit.Text}' at page {hit.PageIndex + 1}")
                    print(f"  Bounds: ({hit.Bounds.left}, {hit.Bounds.top}, "
                          f"{hit.Bounds.right}, {hit.Bounds.bottom})")
```

### Metadata Extraction

Extract document metadata (author, title, dates, etc.) when opening with `IGR_BODY_AND_META`.

### Sub-File Extraction

Extract embedded files from containers (ZIP, email, PST, etc.):

```python
with api.GetExtractor("archive.zip") as extractor:
    if extractor.getSupportsSubFiles():
        for subfile in extractor.SubFiles:
            with subfile:
                # Open must be called inside the with block, before reading text
                text = ""
                if subfile.getSupportsText():
                    subfile.Open(IGR_BODY_AND_META)
                    while not subfile.getEOF():
                        text += subfile.GetText(4096)
```

---

## Python API Reference

### DocumentFilters Class

The main entry point for the SDK.

| Method                                                     | Description                                                                                                        |
| ---------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------ |
| `Initialize(license_key, working_dir, dll_path)`           | Initialize the engine. Must be called before any other operations.                                                 |
| `GetExtractor(source)`                                     | Create an extractor for a file path, bytes, or file-like object. Returns a context manager.                        |
| `OpenExtractor(source, mode=IGR_BODY_AND_META, options="", callback=None)` | Combines `GetExtractor` and `Open` in one call. Preferred when no per-file `Open` customisation is needed. |
| `MakeOutputCanvas(filename, device_type, options)`         | Create an output canvas for rendering. Returns a context manager.                                                  |

### Extractor

Returned by `GetExtractor()`. Use as a context manager (`with` statement).

| Method/Property                                | Description                                                           |
| ---------------------------------------------- | --------------------------------------------------------------------- |
| `getFileType(format?)`                         | Get file type. Pass `IGR_FORMAT_LONG_NAME` for human-readable string. |
| `getSupportsText()`                            | Returns `True` if text extraction is supported.                       |
| `getSupportsSubFiles()`                        | Returns `True` if the file contains sub-files (archives, email).      |
| `Open(flags, options?)`                        | Open the document for processing.                                     |
| `getEOF()`                                     | Returns `True` when all text has been read.                           |
| `GetText(max_chars, stripControlCodes?)`       | Read text from the document.                                          |
| `Pages`                                        | Iterable of page objects for rendering.                               |
| `SubFiles`                                     | Iterable of sub-file extractors.                                      |
| `Compare(otherDocument, compareSettings, ...)` | Compare with another extractor. Returns comparison results.           |

### Pages & Canvas

| Canvas Method                    | Description                           |
| -------------------------------- | ------------------------------------- |
| `RenderPage(page)`               | Render a page to the canvas output.   |
| `SetBrush(color, style)`         | Set brush for drawing operations.     |
| `SetPen(color, width, style)`    | Set pen for drawing operations.       |
| `Rect(left, top, right, bottom)` | Draw a rectangle (for highlighting).  |
| `Close()`                        | Close the canvas and finalize output. |

### Compare API

| Class/Method                                | Description                                                             |
| ------------------------------------------- | ----------------------------------------------------------------------- |
| `DocumentFilters.CompareSettings()`         | Settings object for comparison.                                         |
| `.CompareType`                              | Set to `IGR_COMPARE_DOCUMENTS_COMPARE_WORDS` for word-level comparison. |
| `.Flags`                                    | Bitwise OR of `IGR_COMPARE_DOCUMENTS_FLAGS_*` constants.                |
| `DocumentFilters.CompareDocumentSettings()` | Per-document settings (page ranges, margins).                           |
| `.FirstPage`                                | First page to compare (0-based).                                        |
| `.PageCount`                                | Number of pages to compare.                                             |
| `.Margins`                                  | `IGR_FRect` for margin exclusion.                                       |
| `results.MoveNext()`                        | Advance to next difference. Returns `False` when done.                  |
| `results.Current`                           | Current difference object.                                              |
| `diff.Type`                                 | Difference type (see constants below).                                  |
| `diff.OriginalPageIndex`                    | Page index in original document (0-based).                              |
| `diff.RevisedPageIndex`                     | Page index in revised document (0-based).                               |
| `diff.GetText()`                            | Get text content of the difference.                                     |
| `diff.Details`                              | Iterable of detail parts with `.Text`, `.PageIndex`, `.Bounds`.         |

### Constants & Flags

#### Open Flags

| Constant                              | Description                                                                                                                        |
| ------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| `IGR_BODY_AND_META`                   | Extract body text and metadata                                                                                                     |
| `IGR_FORMAT_IMAGE`                    | Enable page rendering. Use alone when only rendering is needed (e.g., pure canvas workflows). Combine with `IGR_BODY_AND_META` (`IGR_BODY_AND_META \| IGR_FORMAT_IMAGE`) to enable both text extraction and rendering. |

#### Output Devices

| Constant                    | Description                                                                                                              |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| `IGR_DEVICE_HTML`           | High-definition HTML5 output                                                                                             |
| `IGR_DEVICE_IMAGE_PNG`      | PNG image output                                                                                                         |
| `IGR_DEVICE_IMAGE_PDF`      | PDF output                                                                                                               |
| `IGR_DEVICE_IMAGE_TIFF`     | TIFF image output                                                                                                        |
| `IGR_DEVICE_MARKDOWN`       | Markdown output                                                                                                          |
| `IGR_DEVICE_STRUCTURED_XML` | Page-level XML with word bounding-box data — useful for downstream NLP; not plain text                                   |
| `IGR_DEVICE_JSON`           | MDAST JSON with word locations (enable `JSON_INCLUDE_WORD_LOCATIONS=on`); page-level structure, not a plain text extract |

#### File Type Formats

| Constant               | Description                          |
| ---------------------- | ------------------------------------ |
| `IGR_FORMAT_LONG_NAME` | Return human-readable file type name |

#### Comparison Flags

| Constant                                     | Description                |
| -------------------------------------------- | -------------------------- |
| `IGR_COMPARE_DOCUMENTS_COMPARE_WORDS`        | Word-level comparison      |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE`        | Ignore case differences    |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_FIELDS`      | Exclude fields             |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS`     | Exclude footers            |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS`     | Exclude headers            |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_PUNCTUATION` | Exclude punctuation        |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_TABLES`      | Exclude tables             |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_TEXTBOXES`   | Exclude textboxes          |
| `IGR_COMPARE_DOCUMENTS_FLAGS_FORMATTING`     | Include formatting changes |

#### Difference Types

| Constant                                      | Description                                |
| --------------------------------------------- | ------------------------------------------ |
| `IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL`      | Content is equal (skip)                    |
| `IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH` | Batch boundary marker (skip)               |
| `IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT`     | Content was inserted in revised document   |
| `IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE`     | Content was deleted from original document |
| `IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING` | Formatting-only change                     |

#### Drawing Constants

| Constant          | Description         |
| ----------------- | ------------------- |
| `IGR_BRUSH_SOLID` | Solid brush fill    |
| `IGR_PEN_NONE`    | No pen (no outline) |

---

## Canvas Options

Options are passed as a semicolon-separated string to `MakeOutputCanvas()`:

| Option                           | Description                                                                               | Example                            |
| -------------------------------- | ----------------------------------------------------------------------------------------- | ---------------------------------- |
| `HTML_INLINE_IMAGES=on`          | Embed images as Base64 in HTML output                                                     | `"HTML_INLINE_IMAGES=on"`          |
| `GRAPHIC_DPI=N`                  | Set output DPI for image rendering                                                        | `"GRAPHIC_DPI=300"`                |
| `GRAPHIC_WIDTH=N`                | Set output width in pixels                                                                | `"GRAPHIC_WIDTH=4000"`             |
| `OCR_INLINE_IMAGES=on`           | Enable OCR on inline images                                                               | `"OCR_INLINE_IMAGES=on"`           |
| `DOCUMENT_HEADERS=ON`            | Include document headers in output                                                        | `"DOCUMENT_HEADERS=ON"`            |
| `DOCUMENT_FOOTERS=ON`            | Include document footers in output                                                        | `"DOCUMENT_FOOTERS=ON"`            |
| `TEXT_INLINE_LINKS=on`           | Include hyperlinks in text output                                                         | `"TEXT_INLINE_LINKS=on"`           |
| `JSON_INCLUDE_WORD_LOCATIONS=on` | Include word locations in MDAST JSON                                                      | `"JSON_INCLUDE_WORD_LOCATIONS=on"` |
| `MARKDOWN_INCLUDE_METADATA=ON`   | Embed document metadata (author, title, etc.) in Markdown output                         | `"MARKDOWN_INCLUDE_METADATA=ON"`   |
| `MARKDOWN_METADATA_FORMAT=X`     | Format for embedded metadata: `JSON`, `YAML`, `COMMENTS`, or `None` (suppresses metadata) | `"MARKDOWN_METADATA_FORMAT=YAML"`  |

Multiple options can be combined: `"HTML_INLINE_IMAGES=on;GRAPHIC_DPI=300"`

---

## Environment Variables

| Variable         | Description                                            | Example                    |
| ---------------- | ------------------------------------------------------ | -------------------------- |
| `DF_LICENSE_KEY` | Document Filters license key                           | `HXM72 RG6U7 MTU65 ...`    |
| `DF_PATH`        | Override path to native runtime binaries               | `/opt/docfilters/lib`      |
| `ISYS_FONTS`     | Path to font assets directory                          | `./runtimes/26.1.0/assets` |
| `ISYS_ASSETS`    | Path to asset directory (typically same as ISYS_FONTS) | `./runtimes/26.1.0/assets` |

---

## Input Sources

The `GetExtractor()` method accepts:

| Input Type        | Description                           |
| ----------------- | ------------------------------------- |
| `str` (file path) | Path to a file on disk                |
| `bytes`           | Raw file content in memory            |
| `BinaryIO`        | File-like object (e.g., `io.BytesIO`) |

---

## Supported Platforms

### Runtime Binaries

| OS      | Architecture          | Artifact                      |
| ------- | --------------------- | ----------------------------- |
| Windows | x64                   | `windows-intel-msvc-64`       |
| Windows | x86                   | `windows-intel-msvc-32`       |
| Linux   | x64 (glibc)           | `linux-intel-gcc-64`          |
| Linux   | x64 (musl/Alpine)     | `linux-intel-clang-musl-64`   |
| Linux   | ARM64 (glibc)         | `linux-aarch64-gcc-64`        |
| Linux   | ARM64 (musl/Alpine)   | `linux-aarch64-clang-musl-64` |
| Linux   | ARMv7                 | `linux-armv7l-gcc-32`         |
| Linux   | PPC64LE               | `linux-ppc64le-gcc-64`        |
| macOS   | Apple Silicon (ARM64) | `macos-arm64-clang-64`        |
| macOS   | Intel (x64)           | `macos-intel-clang-64`        |
| FreeBSD | x86                   | `freebsd-intel-clang-32`      |
| FreeBSD | x64                   | `freebsd-intel-clang-64`      |

### Supported File Formats (600+)

Major categories include:

- **Office**: Microsoft Word, Excel, PowerPoint (all versions), OpenDocument (ODT, ODS, ODP)
- **PDF**: PDF, PDF/A
- **Email**: EML, MSG, PST, OST, OLM, MBOX
- **Archives**: ZIP, RAR, 7Z, TAR, GZIP
- **Images**: JPEG, PNG, TIFF, BMP, GIF, SVG, WEBP, DICOM, HEIC
- **Web**: HTML, XML, XHTML
- **Text**: TXT, CSV, RTF, Markdown
- **CAD**: DWG, DXF
- **And many more** — see [full format list](https://hyland.github.io/DocumentFilters-Docs/latest/supported_platforms.html)

---

## Best Practices & Pitfalls

| Practice                                   | Detail                                                                                                                                |
| ------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------- |
| **Initialize once, reuse**                 | Call `api.Initialize()` once at application startup and reuse the `api` instance. It is not thread-safe to initialize multiple times. |
| **Always use context managers**            | Use `with api.GetExtractor(...)` and `with api.MakeOutputCanvas(...)` to ensure proper resource cleanup.                              |
| **Set `ISYS_FONTS` and `ISYS_ASSETS`**     | Rendering without fonts produces poor output. Always download and configure the assets directory.                                     |
| **Native binaries must match platform**    | The Python bindings are pure Python but call native C libraries. Always download the correct platform artifact.                       |
| **License required for HD HTML**           | High-fidelity HTML5 output requires a valid license. Evaluation mode falls back to PDF/image output only.                             |
| **License required for comparison**        | Document comparison is a licensed feature.                                                                                            |
| **Handle `IGRException`**                  | Catch errors from Document Filters operations. Error code `4015` (in `e.args[0]['errorCode']`) indicates a license limitation. Use `extractor.PasswordCallback = lambda id: 'secret'` before `Open()` for password-protected files; without it, protected files raise an unhandled `IGRException`. |
| **Pass `IGR_FORMAT_IMAGE` for rendering**  | When using `Pages` or `Canvas`, open with `IGR_BODY_AND_META \| IGR_FORMAT_IMAGE`.                                                    |
| **Large files need memory**                | Rendering large documents loads pages into memory. Process page-by-page when possible.                                                |
| **Runtime path is platform-specific**      | The `dll_path` must point to the directory containing `ISYS11df.dll` (Windows) or `libISYS11df.so` (Linux).                           |
| **Cache runtime downloads**                | Native binaries are ~50-60 MB per platform. Cache them to avoid re-downloading.                                                       |
| **Use `HTML_INLINE_IMAGES=on`**            | For self-contained HTML output without external image files.                                                                          |
| **Strip overlay divs for selectable text** | Generated HTML may contain `idf-page-overlay` divs that block text selection — remove them in post-processing.                        |

---

## Error Handling

### IGRException

All SDK errors raise `IGRException` (exported at module level). Wrap extractor operations in `try/except`:

```python
from DocumentFilters import DocumentFilters, IGRException, IGR_BODY_AND_META

try:
    with api.GetExtractor("document.pdf") as extractor:
        extractor.Open(IGR_BODY_AND_META)
        # ...
except IGRException as e:
    error_code = e.args[0].get('errorCode') if e.args else None
    if error_code == 4015:
        print("License limitation — feature not available in evaluation mode")
    else:
        raise
```

### Password-Protected Files

Set `extractor.PasswordCallback` before calling `Open()`. The callback receives a document ID and must return the password string (or `None` to abort):

```python
with api.GetExtractor("protected.pdf") as extractor:
    extractor.PasswordCallback = lambda doc_id: "secret"
    extractor.Open(IGR_BODY_AND_META)
    # ...
```

Without a password callback, any password-protected document raises an unhandled `IGRException` in automated pipelines.

---

## Code Examples

### Python: Initialize Document Filters

```python
import os
from DocumentFilters import DocumentFilters

def initialize_document_filters(license_key: str = "", runtime_path: str = "./runtimes") -> DocumentFilters:
    """Initialize Document Filters with license and runtime binaries."""
    api = DocumentFilters()

    # Set font/asset paths
    assets_path = os.path.join(runtime_path, "assets")
    os.environ['ISYS_FONTS'] = assets_path
    os.environ['ISYS_ASSETS'] = assets_path

    # Initialize (empty string for evaluation mode)
    api.Initialize(license_key, ".", runtime_path)
    return api
```

### Python: Identify File Type

```python
from DocumentFilters import DocumentFilters, IGR_FORMAT_LONG_NAME

def identify_file(api: DocumentFilters, file_path: str) -> dict:
    """Identify a file's type and capabilities."""
    with api.GetExtractor(file_path) as extractor:
        return {
            "type_id": extractor.getFileType(),
            "type_name": extractor.getFileType(IGR_FORMAT_LONG_NAME),
            "supports_text": extractor.getSupportsText(),
            "supports_subfiles": extractor.getSupportsSubFiles(),
        }
```

### Python: Extract Text

```python
from DocumentFilters import DocumentFilters, IGR_BODY_AND_META

def extract_text(api: DocumentFilters, file_path: str) -> str:
    """Extract all text from a document."""
    text = []
    with api.GetExtractor(file_path) as extractor:
        if not extractor.getSupportsText():
            return ""
        extractor.Open(IGR_BODY_AND_META)
        while not extractor.getEOF():
            text.append(extractor.GetText(4096, stripControlCodes=True))
    return "".join(text)
```

### Python: Convert to HTML5

```python
import os
from DocumentFilters import (
    DocumentFilters, IGR_BODY_AND_META, IGR_FORMAT_IMAGE, IGR_DEVICE_HTML
)

def convert_to_html(api: DocumentFilters, input_path: str, output_path: str) -> bool:
    """Convert a document to high-fidelity HTML5."""
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    with api.GetExtractor(input_path) as extractor:
        extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

        with api.MakeOutputCanvas(output_path, IGR_DEVICE_HTML, "HTML_INLINE_IMAGES=on") as canvas:
            for page in extractor.Pages:
                canvas.RenderPage(page)

    return os.path.exists(output_path)
```

### Python: Convert to PDF

```python
from DocumentFilters import (
    DocumentFilters, IGR_BODY_AND_META, IGR_FORMAT_IMAGE, IGR_DEVICE_IMAGE_PDF
)

def convert_to_pdf(api: DocumentFilters, input_path: str, output_path: str) -> None:
    """Convert a document to PDF."""
    with api.GetExtractor(input_path) as extractor:
        extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

        with api.MakeOutputCanvas(output_path, IGR_DEVICE_IMAGE_PDF, "") as canvas:
            for page in extractor.Pages:
                canvas.RenderPage(page)
```

### Python: Render Pages as PNG

```python
from DocumentFilters import (
    DocumentFilters, IGR_BODY_AND_META, IGR_FORMAT_IMAGE, IGR_DEVICE_IMAGE_PNG
)

def render_pages_as_png(api: DocumentFilters, input_path: str, output_dir: str) -> list:
    """Render each page as a PNG image."""
    pages = []
    with api.GetExtractor(input_path) as extractor:
        extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

        for i, page in enumerate(extractor.Pages):
            output_path = f"{output_dir}/page_{i + 1}.png"
            with api.MakeOutputCanvas(output_path, IGR_DEVICE_IMAGE_PNG, "") as canvas:
                canvas.RenderPage(page)
            pages.append(output_path)

    return pages
```

### Python: Compare Documents

```python
from DocumentFilters import (
    DocumentFilters, DocumentFilters as DF,
    IGR_BODY_AND_META, IGR_FORMAT_IMAGE,
    IGR_COMPARE_DOCUMENTS_COMPARE_WORDS,
    IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL,
    IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH,
    IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT,
    IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE,
    IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING,
)

def compare_documents(api: DocumentFilters, original: str, revised: str, options: dict = None) -> list:
    """Compare two documents and return list of differences."""
    compare_settings = DocumentFilters.CompareSettings()
    compare_settings.CompareType = IGR_COMPARE_DOCUMENTS_COMPARE_WORDS
    compare_settings.Flags = 0

    options = options or {}
    # Apply optional comparison flags from options dict
    # e.g., options={"ignore_case": True, "ignore_headers": True}

    differences = []

    with api.GetExtractor(original) as left:
        left.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

        with api.GetExtractor(revised) as right:
            right.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE)

            with left.Compare(otherDocument=right, compareSettings=compare_settings) as results:
                while results.MoveNext():
                    diff = results.Current

                    if diff.Type in (IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL,
                                     IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH):
                        continue

                    diff_type = "Unknown"
                    if diff.Type == IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT:
                        diff_type = "Insert"
                    elif diff.Type == IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE:
                        diff_type = "Delete"
                    elif diff.Type == IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING:
                        diff_type = "Formatting"

                    parts = []
                    for hit in diff.Details:
                        parts.append({
                            "text": hit.Text,
                            "page": hit.PageIndex + 1,
                            "bounds": {
                                "l": hit.Bounds.left,
                                "t": hit.Bounds.top,
                                "r": hit.Bounds.right,
                                "b": hit.Bounds.bottom,
                            }
                        })

                    differences.append({
                        "type": diff_type,
                        "originalPage": diff.OriginalPageIndex + 1,
                        "revisedPage": diff.RevisedPageIndex + 1,
                        "text": diff.GetText(),
                        "parts": parts,
                    })

    return differences
```

### Python: Process from Memory (bytes/stream)

```python
import io
from DocumentFilters import DocumentFilters, IGR_BODY_AND_META

def extract_text_from_bytes(api: DocumentFilters, file_bytes: bytes) -> str:
    """Extract text from document bytes (e.g., from a web upload)."""
    text = []
    with api.GetExtractor(file_bytes) as extractor:
        extractor.Open(IGR_BODY_AND_META)
        while not extractor.getEOF():
            text.append(extractor.GetText(4096, stripControlCodes=True))
    return "".join(text)

def extract_text_from_stream(api: DocumentFilters, stream: io.BytesIO) -> str:
    """Extract text from a file-like stream."""
    text = []
    with api.GetExtractor(stream) as extractor:
        extractor.Open(IGR_BODY_AND_META)
        while not extractor.getEOF():
            text.append(extractor.GetText(4096, stripControlCodes=True))
    return "".join(text)
```

### Python: Auto-Download Runtime & Assets

```python
import os
import platform
import sys
import zipfile
import urllib.request

DF_VERSION = "26.1.0"

def download_runtime(version: str = DF_VERSION) -> str:
    """Download platform-specific runtime binaries."""
    current_os = platform.system().lower()
    machine = platform.machine().lower()

    if current_os == "windows" and machine in ("amd64", "x86_64"):
        artifact = "windows-intel-msvc-64"
    elif current_os == "linux" and machine == "x86_64":
        artifact = "linux-intel-gcc-64"
    elif current_os == "darwin" and machine == "arm64":
        artifact = "macos-arm64-clang-64"
    elif current_os == "darwin" and machine == "x86_64":
        artifact = "macos-intel-clang-64"
    else:
        raise RuntimeError(f"Unsupported platform: {current_os}-{machine}")

    dest_dir = f"runtimes/{version}/{artifact}"
    if not os.path.exists(dest_dir):
        url = f"https://github.com/Hyland/DocumentFilters/releases/download/v{version}/{artifact}.zip"
        archive = f"runtimes/{version}/{artifact}.zip"
        os.makedirs(os.path.dirname(archive), exist_ok=True)
        urllib.request.urlretrieve(url, archive)
        os.makedirs(dest_dir, exist_ok=True)
        with zipfile.ZipFile(archive, 'r') as z:
            z.extractall(dest_dir)

    return dest_dir

def download_assets(version: str = DF_VERSION) -> str:
    """Download font assets."""
    dest_dir = f"runtimes/{version}/assets"
    if not os.path.exists(dest_dir):
        url = f"https://github.com/Hyland/DocumentFilters/releases/download/v{version}/assets.zip"
        archive = f"runtimes/{version}/assets.zip"
        os.makedirs(os.path.dirname(archive), exist_ok=True)
        urllib.request.urlretrieve(url, archive)
        os.makedirs(dest_dir, exist_ok=True)
        with zipfile.ZipFile(archive, 'r') as z:
            z.extractall(dest_dir)

    return dest_dir
```

---

## Troubleshooting

### Initialization Errors

| Error                                                    | Cause                                        | Solution                                                           |
| -------------------------------------------------------- | -------------------------------------------- | ------------------------------------------------------------------ |
| `ModuleNotFoundError: No module named 'DocumentFilters'` | Python bindings not installed                | Run `pip install -r requirements.txt`                              |
| `libISYS11df.so: cannot open shared object file`         | Native libraries not found                   | Set `LD_LIBRARY_PATH` or pass correct `dll_path` to `Initialize()` |
| `ISYS11df.dll not found`                                 | Windows native libraries not found           | Ensure runtime binaries are downloaded and path is correct         |
| `Failed to initialize DocumentFilters`                   | Runtime path incorrect or binaries corrupted | Re-download binaries; verify platform matches                      |

### Licensing Errors

| Error                                       | Cause                                       | Solution                                                            |
| ------------------------------------------- | ------------------------------------------- | ------------------------------------------------------------------- |
| Error code `4015`                           | License limitation reached                  | Provide a valid license key or stay within evaluation limits        |
| `limited evaluation` in error message       | Trying to use licensed features without key | Obtain a license key from Hyland                                    |
| HTML output has "Evaluation Copy" watermark | No license key provided                     | Set `DF_LICENSE_KEY` in `.env` file                                 |
| Comparison fails with `PermissionError`     | Comparison requires a valid license         | Provide license key; comparison is not available in evaluation mode |

### Rendering Issues

| Issue                                 | Cause                                    | Solution                                                                   |
| ------------------------------------- | ---------------------------------------- | -------------------------------------------------------------------------- |
| Missing or garbled fonts in output    | Font assets not configured               | Download assets and set `ISYS_FONTS` / `ISYS_ASSETS` environment variables |
| Blank pages in PNG output             | File type or page issue                  | Check that `IGR_FORMAT_IMAGE` is set when opening                          |
| Overlay blocks text selection in HTML | `idf-page-overlay` div in generated HTML | Strip overlay divs in post-processing                                      |
| Large HTML file size                  | Images not inlined efficiently           | Use `HTML_INLINE_IMAGES=on` for self-contained output                      |

### Comparison Issues

| Issue                            | Cause                           | Solution                                                             |
| -------------------------------- | ------------------------------- | -------------------------------------------------------------------- |
| Empty comparison results         | Documents are identical         | Expected behavior — no differences to report                         |
| `GetText()` returns empty string | Some diff types don't have text | Handle gracefully with try/except                                    |
| Bounding boxes seem offset       | Page index is 0-based           | Add 1 to `PageIndex` for display; coordinates are in document points |

### Platform Issues

| Issue                               | Cause                       | Solution                                                            |
| ----------------------------------- | --------------------------- | ------------------------------------------------------------------- |
| Download fails for runtime binaries | Network or URL issue        | Verify GitHub access; check that version tag exists in releases     |
| Wrong binary architecture           | Platform detection mismatch | Override with `DF_PATH` environment variable                        |
| Slow processing                     | Large or complex files      | Expected for some formats; process pages individually               |
| Python 3.13 iterator error          | SDK compatibility           | Update to Document Filters v26.1.0+ which fixes Python 3.13 support |

### HTTP Status Codes (Python Samples Download)

| Code  | Meaning                                                                 |
| ----- | ----------------------------------------------------------------------- |
| `200` | Download successful                                                     |
| `404` | Version or artifact not found — check the version tag and artifact name |
| `403` | Repository access denied — ensure GitHub access is available            |
