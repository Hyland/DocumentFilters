---
name: compare-documents
description: Compare two documents and report inserted, deleted, or formatted content using Document Filters
---

# Compare Two Documents

Hyland Document Filters processes 600+ file formats natively — Word, PDF, HTML, spreadsheets, and more — without a conversion server or cloud dependency. Its compare API returns word-level insertions, deletions, and formatting differences with exact bounding boxes, making it straightforward to highlight changes in any document type inside a RAG pipeline or review workflow. For product teams, this means any Word or PDF document can be compared automatically — no manual track-changes, no format conversion step.

Comparison is supported on office and PDF formats including DOCX, DOC, PDF, XLSX, and PPTX. Plain text files and archive container formats (ZIP, RAR, etc.) themselves cannot be compared — extract the individual document files first, then compare them — as these formats do not support the `IGR_FORMAT_IMAGE` mode required for comparison.

## Setup

```bash
pip install document-filters
```

Native runtime binaries (~60 MB) are downloaded automatically on first use. Set `DF_LICENSE_KEY` in your environment for full features — omitting it runs in evaluation mode (watermarked output, limited pages).

**Note: document comparison requires a paid or eval license key — it is disabled in the default unlicensed (`""`) mode.** Request a free eval key from `DocFiltersEval@hyland.com`.

## Python

```python
import json
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")  # set DF_LICENSE_KEY env var; "" = eval mode

def compare_documents(left_path, right_path, ignore_case=False):
    """
    Returns a list of differences:
    [{"type": string (INSERT/DELETE/FORMATTING), "originalPage": N, "revisedPage": N,
      "parts": [{"text": str, "page": N, "bounds": {"l","t","r","b"}}]}]
    """
    settings = DocumentFilters.CompareSettings()
    settings.CompareType = IGR_COMPARE_DOCUMENTS_COMPARE_WORDS
    settings.Flags = IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE if ignore_case else 0

    TYPE_NAMES = {
        IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT: "INSERT",
        IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE: "DELETE",
        IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING: "FORMATTING",
    }

    with api.OpenExtractor(left_path, IGR_FORMAT_IMAGE, "") as left:
        with api.OpenExtractor(right_path, IGR_FORMAT_IMAGE, "") as right:
            with left.Compare(otherDocument=right, compareSettings=settings) as results:
                diffs = []
                while results.MoveNext():
                    diff = results.Current
                    if diff.Type in (IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL,
                                     IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH):
                        continue
                    diffs.append({
                        "type": TYPE_NAMES.get(diff.Type, str(diff.Type)),
                        "originalPage": diff.OriginalPageIndex + 1,
                        "revisedPage": diff.RevisedPageIndex + 1,
                        "parts": [{
                            "text": hit.Text,
                            "page": hit.PageIndex + 1,
                            "bounds": {"l": hit.Bounds.left, "t": hit.Bounds.top,
                                       "r": hit.Bounds.right, "b": hit.Bounds.bottom},
                        } for hit in diff.Details]
                    })
    return diffs

diffs = compare_documents("v1.docx", "v2.docx")
print(json.dumps(diffs, indent=2))

# Real-world example: comparing contract revisions to flag changed clauses
contract_diffs = compare_documents("contract_v1.docx", "contract_v2.docx", ignore_case=True)
for d in contract_diffs:
    if d["type"] in ("DELETE", "INSERT"):
        text = " ".join(p["text"] for p in d["parts"])
        print(f'[{d["type"]}] page {d["originalPage"]}: {text[:120]}')
```

## Flags

| Flag | Value | Effect |
|---|---|---|
| `IGR_COMPARE_DOCUMENTS_FLAGS_EQUALS` | `0x1` | Include equal/unchanged regions in results |
| `IGR_COMPARE_DOCUMENTS_FLAGS_MOVES` | `0x10` | Detect moved text blocks (introduces a MOVE diff type — see Notes) |
| `IGR_COMPARE_DOCUMENTS_FLAGS_FORMATTING` | `0x20` | Report formatting-only differences |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_COMMENTS` | `0x40` | Ignore comments/annotations |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE` | `0x80` | Case-insensitive comparison |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_WHITESPACE` | `0x100` | Ignore whitespace differences |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_PUNCTUATION` | `0x200` | Ignore punctuation |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_TABLES` | `0x400` | Ignore table content |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS` | `0x800` | Ignore header regions |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS` | `0x1000` | Ignore footer regions |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS_FOOTERS` | `0x1800` | Ignore both headers and footers (combines NO_HEADERS \| NO_FOOTERS) |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTNOTES` | `0x2000` | Ignore footnotes |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_TEXTBOXES` | `0x4000` | Ignore text box content |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_FIELDS` | `0x8000` | Ignore field content |

## Page Range Filtering

Use `DocumentFilters.CompareDocumentSettings` to limit comparison to specific pages or exclude header/footer areas by pixel margin. Pass settings for **both** documents — omitting either side leaves that document unrestricted:

```python
left_settings = DocumentFilters.CompareDocumentSettings()
left_settings.FirstPage = 1
left_settings.PageCount = 5

right_settings = DocumentFilters.CompareDocumentSettings()
right_settings.FirstPage = 1
right_settings.PageCount = 5  # count of pages, not end index — for pages 3-7 use FirstPage=3, PageCount=5

# Use within the outer `with left.Compare(...) as results:` block shown in the main example:
with left.Compare(otherDocument=right, compareSettings=settings,
                  thisDocumentSettings=left_settings, otherDocumentSettings=right_settings) as results:
    while results.MoveNext():
        ...
```

`Margins` can also be set on each `CompareDocumentSettings` to exclude regions near page edges (e.g., to skip headers or footers by pixel height).

## Password-Protected Files

Pass the password via the `options` string when opening:

```python
with api.OpenExtractor(left_path, IGR_FORMAT_IMAGE, options="PASSWORD(secret)") as left:
    ...
```

For runtime password prompts (when the password is not known in advance), pass a `callback` to `OpenExtractor`. If a document is encrypted and no password is supplied, `Open()` raises `IGRException`.

## Notes

- **Document comparison requires a license key** — not available in eval (`""`) mode. Set `DF_LICENSE_KEY` or request a free eval key from `DocFiltersEval@hyland.com`.
- `diff.Type` is an integer. Named constants: `IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT` (1), `IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE` (2), `IGR_COMPARE_DOCUMENTS_DIFFERENCE_FORMATTING` (3). There is no CHANGE type.
- When `IGR_COMPARE_DOCUMENTS_FLAGS_MOVES` is enabled, a **MOVE** diff type is also produced. The SDK does not define a named constant for the MOVE difference type — no `IGR_COMPARE_DOCUMENTS_DIFFERENCE_MOVE` constant exists in the public bindings. To discover the value at runtime, add a catch-all branch that prints the raw integer on your first test run: `else: print('Unknown diff type:', diff.Type)`. Once identified, add it to `TYPE_NAMES` (e.g., `IGR_COMPARE_DOCUMENTS_DIFFERENCE_MOVE = <value>`).
- Not all formats support `IGR_FORMAT_IMAGE` mode. `OpenExtractor` raises `IGRException` for unsupported formats. When processing mixed-format batches, wrap the call in a `try/except`:

```python
try:
    diffs = compare_documents("file.txt", "file2.txt")
except IGRException as e:
    print(f"Format not supported for comparison: {e}")
    diffs = []
```

- Bounding boxes are in points (1/72 inch) relative to the page origin; cast to `int()` when passing to `canvas.Rect()`.

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
