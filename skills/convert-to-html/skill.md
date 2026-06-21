---
name: convert-to-html
description: Convert any document to HD HTML5 using Document Filters
---

# Convert a Document to HTML

Hyland Document Filters renders documents to high-definition HTML5 — preserving layout, fonts, and images — from over 600 file formats including Word, PDF, PowerPoint, and email. Processing runs entirely in-process with no external servers or conversion services required, making it suitable for on-premise pipelines and air-gapped environments. This skill is designed for AI engineers building document ingestion pipelines and for teams evaluating browser-based document viewing without third-party cloud services.

## Setup

```bash
pip install document-filters
```

Native runtime binaries (~60 MB) are downloaded automatically on first use. Set `DF_LICENSE_KEY` in your environment for full features — omitting it runs in evaluation mode (watermarked output, limited pages).

Note: HD HTML output (`IGR_DEVICE_HTML`) requires a valid license key — eval mode raises an error rather than producing watermarked output. Request a free evaluation key at DocFiltersEval@hyland.com.

## Python

### HD HTML5 (layout-preserving)

```python
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")  # set DF_LICENSE_KEY env var; "" = eval mode

def convert_to_html(input_path, output_path, doc_opts="", canvas_opts="HTML_INLINE_IMAGES=on"):
    with api.GetExtractor(input_path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, doc_opts)
        with api.MakeOutputCanvas(output_path, IGR_DEVICE_HTML, canvas_opts) as canvas:
            for page in doc.Pages:
                with page:  # closes the native page handle after rendering
                    canvas.RenderPage(page)

# Basic conversion
convert_to_html("report.docx", "report.html")

# In-memory output (no disk I/O — pass html_bytes directly to a downstream service)
import io
buf = io.BytesIO()
with api.GetExtractor("report.docx") as doc:
    doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
    with api.MakeOutputCanvas(buf, IGR_DEVICE_HTML, "HTML_INLINE_IMAGES=on") as canvas:
        for page in doc.Pages:
            with page:
                canvas.RenderPage(page)
html_bytes = buf.getvalue()  # bytes — decode("utf-8") if a string is needed
# io.BytesIO (not io.StringIO) is required because MakeOutputCanvas writes raw bytes

# Optimised for PDFs with tables and complex layout
convert_to_html(
    "report.pdf", "report.html",
    doc_opts="PDF_LAYOUT_DETECTION=on;PDF_TABLE_DETECTION=on",
    canvas_opts="HTML_INLINE_IMAGES=on;HDHTML_INCLUDE_WORD_INDEXES=on"
)

# Page-fragment output (no <html>/<body> wrapper) with injected <head> content
convert_to_html(
    "report.docx", "report.html",
    canvas_opts="HTML_INLINE_IMAGES=on;HDHTML_OUTPUT_BOILERPLATE=off;HDHTML_OUTPUT_INJECT_HEAD=@/path/to/extra_head.html"
)
```

### Classic HTML (text extraction, no license required)

Classic HTML uses a document-level streaming API — not per-page page handles. The page count is always 0 when opened with `IGR_FORMAT_HTML`, so iterating `doc.Pages` silently produces empty output.

```python
def convert_to_classic_html(input_path, output_path):
    with api.GetExtractor(input_path) as doc:
        if not doc.getSupportsText():
            raise ValueError(f"File does not support text extraction: {input_path}")
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_HTML, "")
        output_dir = os.path.dirname(output_path)
        with open(output_path, "w", encoding="utf-8") as f:
            while not doc.getEOF():
                f.write(doc.GetText(4096, stripControlCodes=True))
        # Extract embedded images referenced by the HTML
        for image in doc.Images:
            with image:
                image.CopyTo(os.path.join(output_dir, image.getName()))

convert_to_classic_html("report.docx", "report.html")
```

Note: `HTML_INLINE_IMAGES` has no effect in classic HTML mode — images are always written as separate files in the same directory as the output HTML.

### Async page-on-demand HD HTML (for web apps)

When embedding HD HTML output in a web application, you can render each page as a separate HTML fragment and load them on demand. This avoids generating one large monolithic file.

```python
def convert_to_html_async(input_path, output_path, canvas_opts="HTML_INLINE_IMAGES=on;HDHTML_INCLUDE_WORD_INDEXES=on"):
    # main canvas keeps boilerplate (<html>/<body> wrapper); per-page fragments omit it via page_opts
    page_opts = canvas_opts + ";HDHTML_OUTPUT_BOILERPLATE=off"
    with api.GetExtractor(input_path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
        # binary mode required — MakeOutputCanvas writes raw bytes
        with open(output_path, "wb") as out_stream:
            with api.MakeOutputCanvas(out_stream, IGR_DEVICE_HTML, canvas_opts) as canvas:
                for page_index, page in enumerate(doc.Pages):
                    with page:
                        if page_index == 0:
                            canvas.RenderPage(page)  # first page inline
                        else:
                            w, h = page.GetWidth(), page.GetHeight()
                            page_file = f"{os.path.splitext(output_path)[0]}_page_{page_index + 1}.html"
                            placeholder = (
                                f'<div class="idf-page" style="width:{w}px;height:{h}px" '
                                f'title="{os.path.basename(page_file)}"></div>\n'
                            )
                            out_stream.write(placeholder.encode())
                            with api.MakeOutputCanvas(page_file, IGR_DEVICE_HTML, page_opts) as pc:
                                pc.RenderPage(page)
```

See `samples/python/ConvertDocumentToInteractiveHDHTML.py` for the full implementation including CSS/JS injection.

## Options

PDF options are **document-level** — pass them to `doc.Open()`. HTML options are **canvas-level** — pass them to `MakeOutputCanvas()`.

### PDF Input Options (document-level)

| Option                 | Values      | Default | Notes                                                         |
| ---------------------- | ----------- | ------- | ------------------------------------------------------------- |
| `PDF_LAYOUT_DETECTION` | `on`, `off` | `off`   | Master switch — enable for better structure detection in PDFs |
| `PDF_TABLE_DETECTION`  | `on`, `off` | `off`   | Detect tables in PDFs; requires `PDF_LAYOUT_DETECTION=on`     |

### HD HTML Output Options (canvas-level)

| Option                       | Values      | Default | Notes                                                                                 |
| ---------------------------- | ----------- | ------- | ------------------------------------------------------------------------------------- |
| `HTML_INLINE_IMAGES`         | `on`, `off` | `off`   | `on` embeds images as base64 data URIs (single self-contained file); `off` writes images as separate files alongside the HTML. The `convert_to_html` helper defaults this to `on` as a convenience. |
| `HDHTML_INCLUDE_WORD_INDEXES` | `on`, `off` | `off`   | Embeds per-word coordinate data as HTML data attributes, enabling JS-driven text selection, search highlighting, and redaction overlays. Required for AI document Q&A interfaces that need to highlight retrieved passages in the rendered view. |
| `HDHTML_OUTPUT_BOILERPLATE`  | `on`, `off` | `on`    | `off` omits the `<html>`/`<body>` wrapper — useful when embedding the output as a page fragment |
| `HDHTML_OUTPUT_INJECT_HEAD`  | `@path`     | —       | Path to a file whose contents are injected into the `<head>` of the output HTML. The `@` prefix is required — it signals that the value is a file path to read, not a literal string. |

## Notes

- **HD HTML output requires a license key** — unlike other formats, `IGR_DEVICE_HTML` is fully blocked in eval mode and will raise an error rather than producing watermarked output. Set `DF_LICENSE_KEY` or request a free eval key from `DocFiltersEval@hyland.com`
- **`HDHTML_INCLUDE_WORD_INDEXES`** — word coordinates are embedded as data attributes on `<span>` elements using the Document Filters page coordinate space. See `samples/python/ConvertDocumentToInteractiveHDHTML.py` for the JS that maps these to browser coordinates for redaction and highlight overlays.
- Document Filters has two HTML output modes:
  - **HD HTML5** (`IGR_DEVICE_HTML` + `RenderPage()`) — canvas-based rendering, layout-preserving, requires a license
  - **Classic HTML** (`IGR_FORMAT_HTML` + `doc.GetText()`) — text-extraction-based, structure-only, no license required
- Each page must be used as a context manager (`with page:`) to release native page handles promptly
- For PDFs with complex layouts, enable `PDF_LAYOUT_DETECTION=on` and `PDF_TABLE_DETECTION=on` together for best table and structure fidelity
- **Classic HTML** uses a streaming text API (`doc.GetText()` + `doc.getEOF()`), not per-page handles. The page count is always 0 when opened with `IGR_FORMAT_HTML`
- When `HTML_INLINE_IMAGES=off` (the SDK default), HD HTML output writes image files as separate files alongside the HTML output file. Use `HTML_INLINE_IMAGES=on` to produce a single self-contained HTML file with base64-embedded images.
- **HD HTML: unsupported or corrupt files** — if a file is not renderable (e.g. a corrupt or unsupported binary), `doc.Open()` raises `IGRException`. Wrap the call in a try/except to produce a meaningful error rather than a bare traceback. (Classic HTML has an equivalent guard via `doc.getSupportsText()`, but the HD HTML path does not.)
- **Password-protected files**: `doc.Open()` raises `IGRException` when a password is required. Supply the password via a callback:
  ```python
  doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "",
           callback=lambda cb: (cb.Password.SetPassword("secret"), IGR_OK)[1])
  ```
  Or pass `callback=...` to `api.GetExtractor()`. Without it, password-protected files raise an `IGRException` with no actionable message.

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
