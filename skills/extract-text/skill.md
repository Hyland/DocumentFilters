---
name: extract-text
description: Extract plain text from any document supported by Document Filters (600+ formats)
---

# Extract Text from a Document

Hyland Document Filters extracts clean, AI-ready text from 600+ file formats — Word, PDF, Excel, email, archives, and more — running entirely locally with no cloud dependency, so your RAG pipeline or LLM application can index any document your organization works with, without format-specific parsers or external services. It works equally well on-premises, in air-gapped environments, and in cloud pipelines.

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
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")  # key ("" = eval/watermarked mode), resource_path for temp files

def extract_text(filename):
    with api.GetExtractor(filename) as doc:
        if not doc.getSupportsText():
            return ""
        doc.Open(IGR_BODY_AND_META, "")
        text = []
        while not doc.getEOF():
            text.append(doc.GetText(4096, stripControlCodes=True))
        return "".join(text)

print(extract_text("report.pdf"))
```

### Convenience: write directly to a file

```python
with api.GetExtractor("report.pdf") as doc:
    doc.Open(IGR_BODY_AND_META, "")
    doc.SaveTo("output.txt")
    # Note: stripControlCodes has no effect when passing a file path (SDK bug: the parameter is
    # dropped in the recursive call). To strip control codes, use either the explicit GetText loop
    # shown above, or pass an open file object instead of a path string:
    #
    #   with open("output.txt", "w", encoding="utf-8") as f:
    #       doc.SaveTo(f, stripControlCodes=True)
```

## Subfiles (archives, emails)

```python
def extract_text_recursive(doc):
    text = []
    # Always open explicitly before reading text or subfiles — avoids relying on
    # implicit auto-open behavior that may not be obvious to readers of this code.
    doc.Open(IGR_BODY_AND_META, "")
    if doc.getSupportsText():
        while not doc.getEOF():
            text.append(doc.GetText(4096, stripControlCodes=True))
    if doc.getSupportsSubFiles():
        # Recursion handles arbitrary nesting depth: archives within archives,
        # emails with zip attachments containing further documents, etc.
        # This makes the function suitable for fully flattening email archives for RAG indexing.
        for child in doc.SubFiles:
            with child:
                text.extend(extract_text_recursive(child))
    return text

with api.GetExtractor("archive.zip") as doc:
    print("\n".join(extract_text_recursive(doc)))
```

## Page-level chunking (for AI/RAG)

To chunk a document by page for embedding pipelines, open with `IGR_BODY_AND_META | IGR_FORMAT_IMAGE` and iterate over `doc.Pages`:

```python
with api.GetExtractor("report.pdf") as doc:
    doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
    if not doc.getSupportsText():
        print("No text layer — file may be image-only or a non-text format")
    else:
        for i, page in enumerate(doc.Pages):
            text = page.GetText()
            # page.GetText() may return an empty string for image-only pages
            # (e.g. scanned PDFs without an OCR layer); check for this in production code
            # embed or store text as a chunk for page i
            print(f"--- Page {i + 1} ---\n{text}")
```

Each page also exposes `page.Words` (with bounding boxes) for coordinate-grounded citation or highlight workflows — see the `extract-document-structure` skill.

## OCR (scanned PDFs and images)

The OCR engine is primarily designed for image-based formats (TIFF, JPEG, BMP, PNG) and scanned PDFs with no embedded text layer. Using `OCR=ON` with `IGR_BODY_AND_META` on Office documents or PDFs with selectable text may duplicate or corrupt the extracted text; use `IGR_BODY_ONLY` for those cases.

```python
with api.GetExtractor("scan.pdf") as doc:
    doc.Open(IGR_BODY_ONLY, "OCR=ON;OCR_REORIENT_PAGES=ON")
    text = []
    while not doc.getEOF():
        text.append(doc.GetText(4096, stripControlCodes=True))
    print("".join(text))
```

## Notes

- `stripControlCodes=True` removes non-printable characters from the output
- Eval mode limits extraction to the first few pages of each document
- **Password-protected files:** `Open()` raises `IGRException` with `errorCode == IGR_E_PASSWORD` (5) if no callback is supplied. To provide a password programmatically, set the `PasswordCallback` property on the extractor before calling `Open()`:

  ```python
  with api.GetExtractor("protected.pdf") as doc:
      doc.PasswordCallback = lambda doc_id: "secret"
      doc.Open(IGR_BODY_AND_META, "")
  ```

- **Error handling:** `GetExtractor()` does not raise for missing files — the exception is raised in `Open()`. Common error codes:

  | Code | Constant | Meaning |
  |------|----------|---------|
  | 1 | `IGR_E_OPEN_ERROR` | File could not be opened (access/permission denied) |
  | 5 | `IGR_E_PASSWORD` | File is password-protected and no password was supplied |
  | 10 | `IGR_E_NOT_FOUND` | File does not exist |
  | 17 | `IGR_E_FILE_CORRUPT` | File is truncated or corrupt |
  | 2 | `IGR_E_WRONG_TYPE` | Format detection failed |

- **Markdown for AI/LLM pipelines:** For RAG or AI ingestion where document structure matters (headings, lists, tables), consider using a Markdown canvas (`IGR_DEVICE_MARKDOWN`) instead of `GetText()`, which preserves that structure. See the `convert-to-markdown` skill.

- Useful `doc.Open()` option strings for text extraction:
  - `"PDF_LAYOUT_DETECTION=ON;PDF_TABLE_DETECTION=ON"` — improves table extraction from PDFs
  - `"PDF_WORD_WRAP_DETECTION=ON"` — improves word wrap handling in PDFs
  - `"OCR=ON;OCR_REORIENT_PAGES=ON"` — enable OCR (use with `IGR_BODY_ONLY`)
  - `"EXTRACT_EMBEDDED_CONTENT=ON"` — extracts text from embedded OLE/attachment objects
  - `LOCALE` and `CHARSET_HINT` options can improve extraction accuracy for non-ASCII or legacy-encoded documents — particularly relevant for email archives and older Office formats targeting international content

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
