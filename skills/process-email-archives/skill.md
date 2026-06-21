---
name: process-email-archives
description: Extract text and attachments from email archives (PST, MBOX, EML, MSG) using Document Filters
---

# Process Email Archives

Hyland Document Filters lets AI engineers and data teams extract every email, attachment, and nested file from PST, MBOX, EML, and MSG archives using a single Python API — locally, with no cloud calls or Microsoft software required. It treats email archives as recursive containers, making them first-class sources for RAG pipelines, LLM ingestion, and eDiscovery workflows. Supporting 600+ formats including PST and OST (Outlook's proprietary archive formats normally requiring Microsoft software to open), all processing stays on-premises.

Supported formats include: `.pst`, `.ost`, `.mbox`, `.eml`, `.msg`, `.emlx`, `.mbx`

## Setup

```bash
pip install document-filters
```

Native runtime binaries (~60 MB) are downloaded automatically on first use. Set `DF_LICENSE_KEY` in your environment for full features — omitting it runs in evaluation mode (watermarked output, limited pages). All email formats (PST, OST, MBOX, EML, MSG) require a valid license key — they are not available in evaluation mode.

## Quick Start — In-Memory EML Demo

The snippet below builds a minimal EML entirely in memory — ideal for first-run tests and conference booth demos (requires `DF_LICENSE_KEY`):

```python
import io, os
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

# Build a minimal EML in memory — no file needed
msg = MIMEMultipart()
msg['Subject'] = 'Hello from Document Filters'
msg['From'] = 'demo@example.com'
msg['To'] = 'you@example.com'
msg.attach(MIMEText('This is a demo message for the conference booth.'))
eml_bytes = msg.as_bytes()

def extract_email(doc):
    """Extract text and metadata from a single email/message subfile."""
    text_parts = []
    if doc.getSupportsText():
        doc.Open(IGR_BODY_AND_META, "")
        while not doc.getEOF():
            text_parts.append(doc.GetText(4096, stripControlCodes=True))
    return {
        "file_type": doc.getFileType(IGR_FORMAT_LONG_NAME),
        "text": "".join(text_parts),
    }

with api.GetExtractor(io.BytesIO(eml_bytes)) as doc:
    result = extract_email(doc)
    print(result['text'][:200])
```

> Note: All email formats require a valid `DF_LICENSE_KEY` — see the Setup section above.

## Extract All Messages from an Archive

For PST/MBOX archives (all email formats require a license key — see Setup), use the same `extract_email` helper defined above, then add a recursive walker:

```python
# import io, os and api initialization — already done in the Quick Start block above
# extract_email() — already defined above

def walk_archive(doc, results=None, depth=0):
    """Recursively walk an email archive or container, extracting all messages."""
    if results is None:
        results = []

    result = extract_email(doc)
    if result["text"]:
        results.append(result)

    # Note: if getSupportsText() returned False above, doc.Open() was never called.
    # _needHandle() will auto-open the doc with IGR_BODY_AND_META here so SubFiles
    # can be iterated. This is intentional — container formats (ZIP, nested PST)
    # need to be opened to enumerate their children even when they have no text body.
    if doc.getSupportsSubFiles():
        for subfile in doc.SubFiles:
            with subfile:
                walk_archive(subfile, results, depth + 1)

    return results

def process_email_archive(path):
    with api.GetExtractor(path) as doc:
        return walk_archive(doc)

messages = process_email_archive("inbox.pst")
print(f"Extracted {len(messages)} messages")
```

## Rich Extraction — Text, Subject, Sender, Attachments

```python
def parse_email_headers(text):
    """
    Extract common email headers from text produced by IGR_BODY_AND_META.
    The header block appears at the top of the text stream as 'Key: Value' lines
    followed by a blank line before the message body.
    """
    headers = {}
    for line in text.splitlines():
        if not line.strip():
            break  # end of header block
        if ": " in line:
            key, _, value = line.partition(": ")
            headers[key.strip().lower()] = value.strip()
    return headers

def extract_email_rich(doc, name=""):
    """Extract message body, metadata headers, plus recurse into attachments."""
    result = {
        "name": name,
        "file_type": doc.getFileType(IGR_FORMAT_LONG_NAME),
        "text": "",
        "metadata": {},   # subject, from, to, date — parsed from header block
        "attachments": [],
    }

    if doc.getSupportsText():
        doc.Open(IGR_BODY_AND_META, "")
        parts = []
        while not doc.getEOF():
            parts.append(doc.GetText(4096, stripControlCodes=True))
        result["text"] = "".join(parts)
        # IGR_BODY_AND_META injects email headers (Subject, From, To, Date, etc.)
        # at the top of the text stream before the message body.
        result["metadata"] = parse_email_headers(result["text"])

    # Recurse into subfiles (attachments, nested messages)
    if doc.getSupportsSubFiles():
        for subfile in doc.SubFiles:
            with subfile:
                child = extract_email_rich(subfile, name=subfile.getName())
                result["attachments"].append(child)

    return result

def process_archive_rich(path):
    with api.GetExtractor(path) as archive:
        return extract_email_rich(archive, name=os.path.basename(path))
```

## Flatten for RAG Ingestion

```python
def flatten_for_rag(node, source_path="", records=None):
    """
    Flatten the recursive email tree into a list of RAG-ready records.
    Each record has: source, text, depth indicator.
    """
    if records is None:
        records = []

    text = node.get("text", "").strip()
    if text:
        records.append({
            "source": source_path or node["name"],
            "name": node["name"],
            "file_type": node["file_type"],
            "text": text,
            "metadata": node.get("metadata", {}),  # subject, from, to, date
        })

    for attachment in node.get("attachments", []):
        child_path = f"{source_path or node['name']} / {attachment['name']}"
        flatten_for_rag(attachment, child_path, records)

    return records

# Full pipeline: PST → flat records → ready for chunking + embedding
archive = process_archive_rich("inbox.pst")
records = flatten_for_rag(archive)
print(f"{len(records)} extractable items (messages + attachments)")
for r in records[:5]:
    print(f"  [{r['file_type']}] {r['source']}: {r['text'][:60]}...")
```

## Convert Emails to Markdown

For richer structure preservation, convert each message to Markdown. This builds on the initialized `api` from the section above:

```python
import io, os  # already imported at the top of this file; repeated here for standalone clarity
# assumes api is already initialized (see above)
def email_to_markdown(doc, include_locations=False):
    """
    Convert a message subfile to a Markdown string.

    include_locations=True: adds <!-- LOC: page, (l,t,r,b) --> comments before
    each paragraph when converting emails that embed PDF or Word attachments.
    These location comments link extracted text back to its source position,
    which is useful for AI citation and highlight-back features.
    """
    if not doc.getSupportsText():
        return ""
    opts = "MARKDOWN_FLAVOR=GPT;MARKDOWN_INCLUDE_METADATA=on;MARKDOWN_METADATA_FORMAT=YAML"
    if include_locations:
        opts += ";MARKDOWN_INCLUDE_LOCATIONS=on"
    buf = io.BytesIO()
    doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
    with api.MakeOutputCanvas(buf, IGR_DEVICE_MARKDOWN, opts) as canvas:
        for page in doc.Pages:
            with page:
                canvas.RenderPage(page)
    return buf.getvalue().decode("utf-8")
```

Use `email_to_markdown` in a recursive walk over the archive — the same pattern as `walk_archive` but producing Markdown strings instead of plain text:

```python
def archive_to_markdown_records(path, include_locations=False):
    """Convert every text-bearing item in an archive to a Markdown record."""
    results = []
    with api.GetExtractor(path) as archive:
        def _walk(doc, source):
            md = email_to_markdown(doc, include_locations=include_locations)
            if md:
                results.append({"source": source, "markdown": md})
            if doc.getSupportsSubFiles():
                for subfile in doc.SubFiles:
                    with subfile:
                        child_source = f"{source} / {subfile.getName()}"
                        _walk(subfile, child_source)
        _walk(archive, os.path.basename(path))
    return results

md_records = archive_to_markdown_records("inbox.pst")
# Each record: {"source": "inbox.pst / Message 001", "markdown": "---\nsubject: ...\n---\n..."}
# Ready for text splitting and embedding.
print(f"{len(md_records)} Markdown records produced")

# For emails with embedded PDF or Word attachments, enable location comments:
md_records_with_locs = archive_to_markdown_records("inbox.pst", include_locations=True)
# Markdown will contain lines like:
#   <!-- LOC: 1, (72,144,540,156) -->
# before each paragraph, linking text back to its source position in the attachment.
```

## Notes

- `walk_archive()` recurses into any container format, including ZIP files or nested PST/MBOX archives attached to messages — no extra steps are required.
- `doc.getSupportsText()` returns `False` for binary attachments (images, executables) — these are skipped automatically
- All email formats (PST, OST, MBOX, EML, MSG) require a valid license key — none are available in evaluation mode
- For production pipelines, wrap the `with subfile:` block in `try/except IGRException` to skip corrupted or unsupported messages without aborting the full archive walk
- For large PST archives (10k+ messages), process in batches to avoid memory pressure

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
