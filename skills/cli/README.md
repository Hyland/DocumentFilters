# Document Filters CLI

A command-line tool for extracting, converting, and processing 600+ document formats powered by [Hyland Document Filters](https://github.com/Hyland/DocumentFilters).

## Install

```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
```

On first use the native runtime binaries (~60 MB) are downloaded automatically from GitHub Releases and cached to `~/.document-filters/runtimes/`.

Pre-download them explicitly:

```bash
df init
```

## License key

Evaluation mode works without a key (watermarked output, limited pages). For full access:

```bash
export DF_LICENSE_KEY="your-key-here"
```

HTML and email formats (PST, OST, MBOX, EML, MSG) require a license key — they are not available in evaluation mode. This applies to all commands including `df extract-metadata`; attempting these formats without a key returns error 4015.

## Commands

| Command | Description |
|---|---|
| `df init` | Download native runtime |
| `df info` | Show version, platform, runtime, and license key status |
| `df extract-text` | Extract plain text |
| `df convert-to-markdown` | Convert to Markdown |
| `df convert-to-pdf` | Convert to PDF |
| `df convert-to-html` | Convert to HD HTML5 |
| `df convert-to-png` | Render pages as PNG images |
| `df extract-metadata` | Show file type, page count, properties |
| `df extract-structure` | Extract structure as Markdown or JSON |
| `df redact` | Redact text matching patterns |
| `df compare` | Compare two documents |

### Extract text

```bash
df extract-text report.pdf
df extract-text scan.pdf --ocr
df extract-text archive.zip --subfiles
df extract-text document.docx --output text.txt
```

### Convert to Markdown

```bash
df convert-to-markdown report.pdf
df convert-to-markdown report.pdf --pdf-tables
df convert-to-markdown report.pdf --flavor GPT --locations
df convert-to-markdown slides.pptx --output slides.md --metadata
```

### Convert to PDF

```bash
df convert-to-pdf document.docx
df convert-to-pdf document.docx --bookmarks
```

### Convert to HTML

```bash
df convert-to-html document.docx
df convert-to-html document.pdf --no-inline-images
```

### Render as PNG

```bash
df convert-to-png document.pdf
df convert-to-png document.pdf --dpi 150 --output-prefix ./pages/slide
```

### Extract metadata

```bash
df extract-metadata unknown-file.bin
df extract-metadata report.pdf --json
df extract-metadata sample.html --json
```

The `--json` flag returns a JSON object with the following fields:

```json
{
  "path": "sample.html",
  "file_type_id": 49,
  "file_type_name": "HTML Text Only",
  "page_count": 1,
  "supports_text": true,
  "supports_subfiles": true
}
```

HTML and email formats require a license key (`DF_LICENSE_KEY`); without one, `extract-metadata` returns error 4015 for those formats.

### Extract structure

```bash
df extract-structure report.pdf
df extract-structure report.pdf --format json --words
df extract-structure report.pdf --locations
```

### Redact

```bash
df redact contract.pdf "SSN: \d{3}-\d{2}-\d{4}" "Account: \d{10,16}"
df redact contract.pdf "John Smith" --output redacted.pdf
```

### Compare documents

```bash
df compare original.docx revised.docx
df compare original.pdf revised.pdf --ignore-case --output-left left.pdf --output-right right.pdf
```

## Environment variables

| Variable | Description |
|---|---|
| `DF_LICENSE_KEY` | License key for full-feature access |
| `DF_VERSION` | Override the runtime version to download (default: matches the installed CLI version) |
| `DF_RUNTIME_PATH` | Override the native runtime directory path entirely |
| `DOCFILTERS_RELEASES_URL` | Override the GitHub Releases base URL |

## Reference

- [Document Filters documentation](https://hyland.github.io/DocumentFilters-Docs/latest/)
- [GitHub repository](https://github.com/Hyland/DocumentFilters)
