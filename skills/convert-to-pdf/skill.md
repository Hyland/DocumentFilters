---
name: convert-to-pdf
description: Convert any document to PDF using Document Filters
---

# Convert a Document to PDF

Document Filters converts Word, Excel, PowerPoint, CAD, email archives, and 600+ other formats to PDF — locally, with no LibreOffice and no cloud API, just `pip install`. This makes it a strong fit for air-gapped environments, high-throughput pipelines, and any workflow where external dependencies are not an option. The output is a standard PDF file. When the input is already a PDF, pass `PDF_PRESERVE_ORIGINAL=on` to preserve vector content; otherwise, pages are rendered at high resolution.

## Setup

```bash
pip install document-filters
```

Native runtime binaries (~60 MB) are downloaded automatically on first use. Set `DF_LICENSE_KEY` in your environment for full features — omitting it runs in evaluation mode (watermarked output, limited pages).

## Python

```python
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")  # set DF_LICENSE_KEY env var; "" = eval mode

def convert_to_pdf(input_path, output_path, bookmarks=False, open_options=""):
    # PDF_BOOKMARKS must be passed to Open(), not MakeOutputCanvas()
    open_opts = f"PDF_BOOKMARKS={str(bookmarks).lower()}"
    if open_options:
        open_opts += f";{open_options}"
    with api.GetExtractor(input_path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, open_opts)
        with api.MakeOutputCanvas(output_path, IGR_DEVICE_IMAGE_PDF, "") as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)

convert_to_pdf("report.docx", "report.pdf")
convert_to_pdf("report.docx", "report.pdf", bookmarks=True)
# multi-column PDFs or PDFs with tables: improves text-flow and cell extraction for downstream text use
convert_to_pdf("report.pdf", "report_out.pdf", open_options="PDF_LAYOUT_DETECTION=on;PDF_TABLE_DETECTION=on")

# In-memory output — write PDF bytes to a buffer instead of a file
# MakeOutputCanvas accepts either a file path (str) or an IGRStream subclass.
# Plain io.BytesIO() is NOT accepted directly — wrap it in a subclass of IGRStream.
import io

class BytesIOStream(IGRStream):
    def __init__(self):
        super().__init__()
        self._buf = io.BytesIO()
    def Read(self, length):
        return self._buf.read(length)
    def Write(self, data):
        self._buf.write(data)
        return len(data)
    def Seek(self, offset, whence):
        return self._buf.seek(offset, whence)
    def Tell(self):
        return self._buf.tell()
    def getvalue(self):
        self._buf.seek(0)
        return self._buf.read()

def convert_to_pdf_bytes(input_path, open_options=""):
    stream = BytesIOStream()
    with api.GetExtractor(input_path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, open_options)
        with api.MakeOutputCanvas(stream, IGR_DEVICE_IMAGE_PDF, "") as canvas:
            page_count = 0
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)
                    page_count += 1
        if page_count == 0:
            raise ValueError(f"No pages rendered from {input_path!r}. The document may be empty or the format does not support image rendering.")
    return stream.getvalue()

pdf_bytes = convert_to_pdf_bytes("report.docx")
# pdf_bytes is a bytes object — store in object storage, pass to a model, etc.

# Combine multiple documents into a single PDF
# PDF_PRESERVE_ORIGINAL=on keeps existing PDF pages as vectors; without it they are rasterized
def combine_to_pdf(input_paths, output_path):
    with api.MakeOutputCanvas(output_path, IGR_DEVICE_IMAGE_PDF, "PDF_PRESERVE_ORIGINAL=on") as canvas:
        for path in input_paths:
            with api.GetExtractor(path) as doc:
                doc.Open(IGR_FORMAT_IMAGE, "")
                for page in doc.Pages:
                    with page:
                        canvas.RenderPage(page)

combine_to_pdf(["chapter1.docx", "chapter2.docx", "appendix.xlsx"], "combined.pdf")

# Password-protected files: set PasswordCallback before calling Open()
def convert_protected_pdf(input_path, output_path, password):
    with api.GetExtractor(input_path) as doc:
        doc.PasswordCallback = lambda doc_id: password
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
        with api.MakeOutputCanvas(output_path, IGR_DEVICE_IMAGE_PDF, "") as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)
```

## Notes

- Internally, Document Filters opens the source file in image mode (`IGR_FORMAT_IMAGE`) and renders each page to a PDF canvas. When the input is already a PDF and `PDF_PRESERVE_ORIGINAL=on` is set, the original vector content is preserved rather than rasterized.
- `IGR_FORMAT_IMAGE` is required in `Open()` to enable page rendering. `IGR_BODY_AND_META` alone (without `IGR_FORMAT_IMAGE`) is for text extraction. The examples above use `IGR_BODY_AND_META | IGR_FORMAT_IMAGE` — the `IGR_BODY_AND_META` flag is optional for a pure render but harmless and makes it easier to add text extraction later without reopening the document. For a pure page-render combine (no text access needed), `IGR_FORMAT_IMAGE` alone is sufficient.
- `PDF_BOOKMARKS=true` generates a navigable bookmark tree in the output PDF from heading styles in the source document (applies to Word, PowerPoint, PDF, and any format with structural headings). Pass it as an option to `Open()`, not to `MakeOutputCanvas()`. Option values are case-insensitive; `true`/`false`, `on`/`off`, and `1`/`0` are all accepted.
- `PDF_PRESERVE_ORIGINAL=on` preserves existing PDF page content as vectors when combining documents. Without it, PDF pages in input files are rasterized, causing quality loss. Pass this to `MakeOutputCanvas()`.
- `PDF_LAYOUT_DETECTION=on` improves column and text-flow detection in multi-column PDFs. Pass to `Open()`.
- `PDF_TABLE_DETECTION=on` improves table cell extraction in PDFs. Pass to `Open()`.
- Multiple options are separated by semicolons: `"PDF_LAYOUT_DETECTION=on;PDF_TABLE_DETECTION=on"`.
- If a file does not exist or its format is unsupported, `GetExtractor()` or `Open()` raises an `IGRException`. Wrap calls in `try/except IGRException`. Document Filters does not expose a pre-open rendering-capability flag. Call `doc.Open(IGR_FORMAT_IMAGE, "")` and check that `doc.Pages` yields at least one page before writing output. `IGRException` will be raised for completely unsupported formats. Note: `getSupportsText()` tests for text-extraction support (`IGR_FILE_SUPPORTS_TEXT`) and is unrelated to rendering — do not use it as a render-capability check.
- If `doc.Pages` yields no pages (empty document, or a text-only format opened with `IGR_FORMAT_IMAGE`), the output canvas will contain an empty or invalid PDF. Check the page count during iteration and raise an error or skip writing the output file if zero pages are rendered.
- For password-protected files, set `doc.PasswordCallback = lambda doc_id: 'password'` before calling `Open()`. Without a callback, encrypted files raise an `IGRException`.
- Each `api.GetExtractor()` handle is single-use. To re-process a document, create a new extractor — do not call `doc.Open()` a second time on the same handle.
- Eval mode limits output to the first few pages.

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
