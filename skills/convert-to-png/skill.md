---
name: convert-to-png
description: Render each page of a document as a PNG image using Document Filters
---

# Convert a Document to PNG Images

When building AI pipelines that pass document pages to vision models or multimodal LLMs, you need reliable, high-fidelity PNG renders from any file format. Hyland Document Filters renders pages from 600+ formats — PDFs, Office documents, email archives, CAD drawings, and more — into PNG images, locally, with no conversion server required.

## Setup

```bash
pip install document-filters
```

Native runtime binaries (~60 MB) are downloaded automatically on first use. Set `DF_LICENSE_KEY` in your environment for full features — omitting it runs in evaluation mode.

For production-quality rendering, set `ISYS_FONTS` and `ISYS_ASSETS` to the path of the downloaded font assets. Without this, output may have garbled or substituted fonts. See the use-document-filters-api skill for download instructions.

## Python

```python
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

def convert_to_png(input_path, output_prefix):
    """Renders each page to <output_prefix>_page1.png, _page2.png, etc."""
    with api.GetExtractor(input_path) as doc:
        # IGR_FORMAT_IMAGE is required; without it, doc.Pages will be empty (text-extraction mode)
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "GRAPHIC_DPI=150")
        page_count = doc.GetPageCount()
        if page_count == 0:
            raise ValueError(f"Document '{input_path}' yielded no renderable pages")
        for i, page in enumerate(doc.Pages, start=1):
            with page:
                out = f"{output_prefix}_page{i}.png"
                with api.MakeOutputCanvas(out, IGR_DEVICE_IMAGE_PNG, "") as canvas:
                    canvas.RenderPage(page)
                print(f"Wrote {out}")

convert_to_png("slides.pptx", "slide")
# Produces: slide_page1.png, slide_page2.png, ...
```

See the Options table below to control resolution or pixel dimensions.

### Render pages to in-memory bytes

`MakeOutputCanvas` accepts either a file path string or an `io.IOBase` object. Pass an `io.BytesIO()` directly to capture PNG bytes in memory — useful for passing pages directly to Pillow, base64-encoding for a vision API, or any pipeline that should avoid writing temp files.

```python
import io
import base64
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

def pages_as_png_bytes(input_path):
    """Yields raw PNG bytes for each page, without writing any files."""
    with api.GetExtractor(input_path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "GRAPHIC_DPI=150")
        for page in doc.Pages:
            with page:
                buf = io.BytesIO()
                with api.MakeOutputCanvas(buf, IGR_DEVICE_IMAGE_PNG, "") as canvas:
                    canvas.RenderPage(page)
                yield buf.getvalue()

for png_bytes in pages_as_png_bytes("slides.pptx"):
    b64 = base64.b64encode(png_bytes).decode("utf-8")
    # pass b64 to a vision API, or: Image.open(io.BytesIO(png_bytes))
```

## Options

Size and DPI options are passed as the second argument to `doc.Open()`, not to `MakeOutputCanvas()`. Options are semicolon-separated.

| Option           | Description                                             | Example                  |
| ---------------- | ------------------------------------------------------- | ------------------------ |
| `GRAPHIC_DPI=N`  | Output resolution. Default is 96 DPI (too low for most use cases). Use 150 for screen, 300 for print. | `"GRAPHIC_DPI=150"` |
| `GRAPHIC_WIDTH=N` | Scale each page to N pixels wide.                      | `"GRAPHIC_WIDTH=2000"`   |
| `GRAPHIC_HEIGHT=N` | Scale each page to N pixels tall.                     | `"GRAPHIC_HEIGHT=2000"`  |
| `GRAPHIC_BPP=N`  | Bits per pixel. Common values: `1` (bitonal), `8` (grayscale), `24` (RGB, default). Use `8` to reduce image size for OCR or vision pipelines. | `"GRAPHIC_BPP=8"` |

Example combining options (semicolon-separated):

```python
doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "GRAPHIC_DPI=150;GRAPHIC_WIDTH=2000")
```

When both `GRAPHIC_WIDTH` and `GRAPHIC_HEIGHT` are set, Document Filters preserves the aspect ratio and clamps to whichever dimension is more constraining — the page will not be distorted.

## Notes

- Each page produces one PNG file; there is no single-file PNG output for multi-page documents
- If a document yields no pages, the loop completes silently. Call `doc.GetPageCount()` after `doc.Open()` to verify the page count before iterating, or check that `len(list(doc.Pages)) > 0`.
- Eval mode adds an "Evaluation Copy" watermark to every page and limits input formats to Text, PDF, and Microsoft/OpenDocument Office files. Set `DF_LICENSE_KEY` to remove these restrictions.
- Unsupported formats, missing files, and corrupted documents raise `IGRException`. Wrap calls in `try/except IGRException` to handle gracefully.
- For password-protected files, set a `PasswordCallback` on an `OpenCallback` instance and pass it to `doc.Open()`:
  ```python
  from DocumentFilters import *
  cb = OpenCallback()
  cb.PasswordCallback = lambda doc_id: "secret"
  doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "GRAPHIC_DPI=150", cb)
  ```
  Without a callback, `doc.Open()` raises `IGRException` for encrypted documents.

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
