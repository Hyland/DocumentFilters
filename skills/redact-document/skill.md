---
name: redact-document
description: Redact text matching a pattern from a document and render the result as PDF
---

# Redact Content from a Document

Hyland Document Filters redacts sensitive content from 600+ file formats — PDFs, Word, Excel, email, and more — entirely on your own infrastructure, with no data sent to external services. It extracts word-level bounding boxes, strips matching text from the content stream, and overlays opaque black bars, producing a tamper-resistant redacted PDF.

## Setup

```bash
pip install document-filters
```

Native runtime binaries (~60 MB) are downloaded automatically on first use. Set `DF_LICENSE_KEY` in your environment for full features — omitting it runs in evaluation mode (watermarked output, limited pages).

## Python

Redaction requires two passes per page. The first pass calls `page.Redact()` before `RenderPage()` to strip matched text from the PDF content stream, preventing recovery via copy-paste or PDF text extraction tools. The second pass draws opaque black rectangles over the same word positions to visually obscure the redacted region in rendered or rasterized output. Both passes are required for tamper-resistant redaction.

> **Pattern scope:** Each pattern is matched against a single word token. Patterns that span whitespace between words (e.g. `r'John\s+Doe'`) will never match because a word object contains only one token's text. Use single-token patterns only; if you need to redact a multi-word phrase, match each token individually (e.g. `r'^John$'` and `r'^Doe$'`).

```python
import os
import re
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")  # set DF_LICENSE_KEY env var; "" = eval mode

def redact_document(input_path, output_path, patterns):
    """
    Redact words matching any of the given regex patterns.
    patterns: list of regex strings matched against individual word tokens.
    Multi-word span patterns (containing r'\s+') will not match — see note above.
    Example patterns: [r'\b\d{3}-\d{2}-\d{4}\b', r'\bConfidential\b']
    """
    compiled = [re.compile(p, re.IGNORECASE) for p in patterns]

    with api.GetExtractor(input_path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "GRAPHIC_DPI=300")
        if not doc.getSupportsText():
            raise ValueError(f'{input_path} does not support word-level extraction; redaction is not possible')
        with api.MakeOutputCanvas(output_path, IGR_DEVICE_IMAGE_PDF, "") as canvas:
            for page in doc.Pages:
                with page:
                    for i, word in enumerate(page.Words):
                        for pattern in compiled:
                            if pattern.search(word.Text):
                                page.Redact(i, i)
                                break
                    canvas.RenderPage(page)
                    # Post-render: draw opaque bars on top of the rendered page content (canvas draw calls overlay the rendered image)
                    canvas.SetBrush(0x000000, IGR_BRUSH_SOLID)  # solid black fill
                    canvas.SetPen(0, 0, IGR_PEN_NONE)
                    for word in page.Words:
                        for pattern in compiled:
                            if pattern.search(word.Text):
                                canvas.Rect(word.X, word.Y,
                                            word.X + word.Width,
                                            word.Y + word.Height)
                                break

redact_document(
    "contract.pdf",
    "contract_redacted.pdf",
    patterns=[r'\b\d{3}-\d{2}-\d{4}\b',                        # SSN
              r'\b\d{4}[- ]?\d{4}[- ]?\d{4}[- ]?\d{4}\b',     # credit card
              r'[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}']  # email address
)
```

## Notes

- The first loop calls `page.Redact(wordIndex, wordIndex)` before `RenderPage()` to remove matched text from the PDF content stream. The second loop draws opaque rectangles over the same words as a visual black bar. Both are required: content stream redaction prevents text recovery via copy-paste; the rectangle ensures visual concealment in rasterized output.
- `page.Words` returns word-level bounding boxes. Word and canvas coordinates are in the same page-geometry space (typically 1/100 mm for most formats). They are not affected by `GRAPHIC_DPI` — that option only controls rasterization quality for the PDF image stream, not coordinate values. Do not scale word coordinates by DPI before passing to `canvas.Rect`.
- `canvas.SetBrush(0x000000, IGR_BRUSH_SOLID)` sets a solid black fill; change the color for colored redaction bars
- Pass rendering options as a semicolon-separated string to `doc.Open()`, e.g. `GRAPHIC_DPI=300` for 300 DPI rasterization. `MakeOutputCanvas` options control output format metadata; pass them as the third argument, e.g. `api.MakeOutputCanvas(output_path, IGR_DEVICE_IMAGE_PDF, "PDF_TITLE=Redacted;PDF_AUTHOR=Compliance Team")`.
- For password-protected files, supply a password callback: `doc.PasswordCallback = lambda doc_id: 'secret'` before calling `doc.Open()`.
- Input must support word-level extraction; the example guards this with `doc.getSupportsText()` immediately after `Open()`. Documents without extractable text (e.g. image-only PDFs, some archive formats) will raise a `ValueError`.
- This skill redacts the rendered PDF output. If you also need to redact text extracted for RAG (e.g., via `convert-to-markdown` or plain-text extraction), apply your regex patterns to the extracted text separately — the PDF redaction does not propagate to text extraction pipelines.
- Eval mode limits output to the first few pages.

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
