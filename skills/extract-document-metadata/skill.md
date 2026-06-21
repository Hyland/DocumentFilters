---
name: extract-document-metadata
description: Get file type, page count, and document properties from any document using Document Filters
---

# Extract Document Metadata

Hyland Document Filters identifies the file type and extracts properties — format name, MIME type, page count, and content fingerprints — from over 600 formats in a single local call, with no conversion server required and no documents leaving the machine. In an AI document pipeline, this lets you route files by type, skip already-indexed documents using MD5/SHA1 deduplication, and gate expensive LLM calls on page count before they start.

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

def get_metadata(filename, password=None):
    with api.GetExtractor(filename) as doc:
        # Set a password callback for protected files (preferred over passing PASSWORD= in options string)
        if password:
            doc.PasswordCallback = lambda fname: password
        # Or derive at runtime: doc.PasswordCallback = lambda fname: os.environ.get("DOC_PASSWORD", "")

        file_type_id = doc.getFileType()
        file_type_name = doc.getFileType(IGR_FORMAT_LONG_NAME)  # None if format unrecognised
        supports_text = doc.getSupportsText()
        supports_subfiles = doc.getSupportsSubFiles()
        supports_html = doc.getSupportsHTML()
        hash_md5 = doc.getHashMD5()
        hash_sha1 = doc.getHashSHA1()

        # Open with IGR_BODY_AND_META (enables metadata) | IGR_FORMAT_IMAGE (enables GetPageCount).
        # Omitting Open() lets GetPageCount() auto-open in image-only mode, but metadata properties
        # (capabilities, file type from handle) will not reflect the opened state.
        try:
            doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
            page_count = doc.GetPageCount()
        except IGRException as e:
            code = e.args[0].get("errorCode") if e.args else None
            if code == IGR_E_PASSWORD:
                raise ValueError(f"File is password-protected: {filename}") from e
            if code == IGR_E_NOT_FOUND:
                raise FileNotFoundError(filename) from e
            if code == IGR_E_FILE_CORRUPT:
                raise ValueError(f"File is corrupt or truncated: {filename}") from e
            raise

        return {
            "file_type_id": file_type_id,
            "file_type_name": file_type_name,
            "supports_text": supports_text,
            "supports_subfiles": supports_subfiles,
            "supports_html": supports_html,
            "hash_md5": hash_md5,
            "hash_sha1": hash_sha1,
            "page_count": page_count,
        }

info = get_metadata("report.docx")
print(info)
# {'file_type_id': 289, 'file_type_name': 'Microsoft Word 2007',
#  'supports_text': True, 'supports_subfiles': False, 'supports_html': True,
#  'hash_md5': 'a1b2c3...', 'hash_sha1': 'd4e5f6...', 'page_count': 12}
```

## Notes

- Calling `doc.getFileType()` with no argument returns the numeric type ID. Pass one of the format-query constants for a string form:

  | Constant              | Value | Returns                          |
  |-----------------------|-------|----------------------------------|
  | `IGR_FORMAT_LONG_NAME`  | 0     | Human-readable name              |
  | `IGR_FORMAT_SHORT_NAME` | 1     | Short/abbreviated name           |
  | `IGR_FORMAT_CONFIG_NAME`| 2     | Internal config name             |
  | `IGR_FORMAT_CLASS_NAME` | 3     | Class name                       |
  | `IGR_FORMAT_MIME_TYPE`  | 5     | MIME type string                 |
  | `IGR_FORMAT_CATEGORY`   | 6     | Category string                  |

- If `file_type_id == 0` or `file_type_name` is `None`, the format was not recognised. Check the file before further processing rather than assuming all results are valid. Use these two guards at the top of any processing loop:

  ```python
  if info['file_type_id'] == 0:
      pass  # skip — unrecognised format
  if not info['supports_text']:
      pass  # skip — no extractable text (e.g. pure image PDF, binary file)
  ```
- For non-paginated formats (plain text, CSV, XML), `GetPageCount()` typically returns 0 or 1 — use character-count-based chunking rather than page-based chunking in these cases.
- For metadata-only queries (no page count), open with `doc.Open(IGR_BODY_AND_META, "")` to avoid the heavier render-mode open.
- For password-protected files, the `PasswordCallback` property is the preferred approach when the password is not known until runtime or when processing many files with different passwords: `doc.PasswordCallback = lambda fname: os.environ.get("DOC_PASSWORD", "")`. You can also pass it inline: `doc.Open(IGR_BODY_AND_META, "PASSWORD=secret")`.
- `getSupportsSubFiles()` is `True` for archives, email containers, and compound documents
- `getSupportsHTML()` returns `True` if the format supports conversion to HD HTML
- To check multiple capabilities at once, call `doc.GetFileCapabilities()` and test against `IGR_FILE_SUPPORTS_TEXT`, `IGR_FILE_SUPPORTS_SUBFILES`, and `IGR_FILE_SUPPORTS_HDHTML` bitmask constants. This is useful for building a routing table in batch pipelines.
- `getHashMD5()` and `getHashSHA1()` return content fingerprints and are available before or after `Open()`
- **Process for AI:** For LLM/AI pipelines, consider using the `convert-to-markdown` skill (`IGR_FORMAT_MARKDOWN`) instead of reading raw text. Markdown conversion preserves heading structure, tables, and document flow far better than plain text extraction, making it the preferred approach when the downstream consumer is an LLM.

## Real-world example: batch pipeline pre-filter

Gate expensive LLM processing with three cheap checks — format recognition, deduplication, and page count — before queuing any file.

```python
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

seen_md5 = set()   # populated from your index DB at startup
llm_queue = []

for filepath in file_list:
    info = get_metadata(filepath)

    if info["file_type_id"] == 0:
        continue  # unrecognised format — skip

    if info["hash_md5"] in seen_md5:
        continue  # already indexed — skip

    if info["page_count"] > 50:
        continue  # too large for LLM context — skip or route to chunker

    seen_md5.add(info["hash_md5"])
    llm_queue.append({"path": filepath, "type": info["file_type_name"]})
```

Three guard clauses eliminate the bulk of unprocessable or redundant files before any I/O-heavy conversion or LLM call occurs.

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
