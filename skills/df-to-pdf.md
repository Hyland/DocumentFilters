# Document Filters: Convert to PDF

Convert any document to PDF — 600+ input formats supported including Office, PDF, HTML, email, CAD files, and images. Works in batch, from the command line, without opening any application. Output preserves layout, fonts, and images.

## Prerequisites

`df` CLI must be installed and the runtime downloaded. If `df info` fails, install first:
```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
df init
```

## Usage

```bash
# Word or PowerPoint → PDF (output: <input>.pdf)
df convert-to-pdf report.docx

# Add a clickable table of contents sidebar from document headings
df convert-to-pdf report.docx --bookmarks --output report.pdf  # --bookmarks: adds clickable nav panel from document headings

# HTML page → PDF
df convert-to-pdf page.html --output page.pdf  # requires DF_LICENSE_KEY

# Email message → PDF for archiving
df convert-to-pdf message.eml --output message.pdf

# Batch: convert every DOCX in a folder
for f in *.docx; do df convert-to-pdf "$f" --bookmarks --output "${f%.docx}.pdf"; done
```

> Note: this shell loop requires Bash (Git Bash, WSL, or macOS/Linux terminal). In PowerShell, use: `Get-ChildItem *.docx | ForEach-Object { df convert-to-pdf $_.Name --bookmarks --output ($_.BaseName + ".pdf") }`

Output is always written to a file — the command prints only the output path to stdout (e.g., `Wrote report.pdf`).

## Flags

| Flag | Description |
|------|-------------|
| `--bookmarks` | Generate a clickable PDF navigation panel from document headings. Recommended for Word, PowerPoint, and multi-section documents. |
| `--output / -o <path>` | Output file path. Defaults to `<input>.pdf` in the same directory as the input. |

## Common use cases

```bash
# Archive a PowerPoint deck as a portable PDF
df convert-to-pdf quarterly_review.pptx --bookmarks

# Convert an email with attachments to PDF for records retention
df convert-to-pdf message.msg --output message.pdf

# Lock in a Word contract as a non-editable PDF
df convert-to-pdf contract_draft.docx --output contract_final.pdf

# Normalize a mixed-format corpus to PDF before text extraction (RAG preprocessing)
# (brace expansion is not available in all shells or Git Bash on Windows — use explicit loops)
for f in contracts/*.docx contracts/*.msg contracts/*.html; do df convert-to-pdf "$f"; done
for f in contracts/*.pdf; do df extract-text "$f" --output "${f%.pdf}.txt"; done
```

## License

With DF_LICENSE_KEY set (as in this environment), all 600+ input formats are supported with no watermark. Without a key, PDF, Office, ODF, and plain-text files convert cleanly with no watermark. HTML, email, CAD, and non-Office formats return error 4015.

## RAG pipeline tip

Use convert-to-pdf as a normalization step when you need consistent page-based coordinates for chunk attribution or bounding-box citation. After converting, run `df extract-structure output.pdf --format json --words` to get word-level bounding boxes suitable for chunk attribution or citation highlighting. If calling the Python SDK directly, the equivalent option is `PDF_BOOKMARKS=true` passed to `doc.Open()`. For plain text extraction from mixed formats, extract-text handles all 600+ formats directly without the PDF intermediate step.
