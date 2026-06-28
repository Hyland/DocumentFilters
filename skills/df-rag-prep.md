This skill turns a folder of documents — PDFs, Word files, emails, spreadsheets — into clean text files an AI assistant can search through. It is the first step in building a document Q&A system, a company knowledge base, or any AI tool that needs to read your files.

Prepare documents so an AI assistant can read and search them. Turn a folder of mixed files (PDFs, Word docs, emails, spreadsheets) into clean text for RAG pipelines (AI search systems).

Based on: `samples/python/ConvertDocumentToMarkdown.py`, `ConvertDocumentToUTF8.py`

**Common use cases:**
- Answer questions from a folder of company policy PDFs or Word docs
- Make scanned contracts searchable by an AI assistant
- Index a mailbox (PST/EML) so an AI can find relevant email threads
- Feed a LangChain or LlamaIndex pipeline from mixed office documents

## Prerequisites

`df` CLI must be installed. If `df info` fails:
```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
df init
```

> **License:** With `DF_LICENSE_KEY` set, all 600+ formats are available. Without a key (eval mode), PDF, Office (docx/xlsx/pptx), ODF, and plain text work with clean output — no watermark text is injected. HTML, email (PST, MBOX, EML, MSG), CAD, and other licensed-only formats return error 4015. Run `df info` to confirm.

## Batch processing

For a clean folder of native (non-scanned) documents, this one-liner processes everything into `./corpus/`:

```bash
# Process all files in a folder into ./corpus/
for F in "$FOLDER"/**/*; do
  [ -f "$F" ] || continue
  df convert-to-markdown "$F" --flavor GPT --pdf-tables \
    --no-headers --no-footers --normalize \
    --metadata --locations \
    --output "./corpus/$(basename "${F%.*}").md"
done
```

For mixed folders with archives or scanned PDFs, follow the full Steps 1–3 classification workflow below.

## Steps

### 1. Identify all files

```bash
df extract-metadata <file> --json
```

The agent will enumerate files automatically — you only need to provide the folder path. Internally it uses:

- Linux/macOS: `find "$FOLDER" -type f`
- Windows with Git Bash: `find "$FOLDER" -type f` works identically. The `dir /s /b` form is for cmd.exe only — avoid mixing shells.

Collect results into a local array — do not echo each JSON blob; build the summary table after all files are processed.

| Condition | Action |
|---|---|
| supports_text: true, supports_subfiles: false | Renderable → Step 2 |
| supports_subfiles: true, email container (PST/MBOX/EML/MSG) | Email → Step 3 (each message/attachment extracted recursively) |
| supports_subfiles: true, archive (ZIP/tar/RAR) | Archive → Step 3 |
| supports_text: true but output blank | Likely scanned → Step 3 with `--ocr`. Detection: run `df convert-to-markdown "$F" --output ./test_probe.md` then check if the output file is empty or contains only whitespace/image-alt-text. Clean up with `rm ./test_probe.md`. Note: page_count > 0 with supports_text: true but zero text is the reliable signal. |
| extract-metadata exits with error containing 'password' (password-protected file) | Password-protected → skip; the CLI does not currently accept a password. Ask the user to remove the password protection in the source application first. |
| Neither | Skip |

### 2. Convert renderable documents to Markdown

```bash
mkdir -p ./corpus
df convert-to-markdown "$F" \
  --flavor GPT --pdf-tables \
  --no-headers --no-footers --normalize \
  --metadata --locations \
  --output ./corpus/$(basename "${F%.*}").md
```

Always pass `--output`; without it, the .md is written alongside the source file.

**Flag guide for RAG:**
- `--flavor GPT` — LLM-optimised, ~10-15% fewer tokens than GFM. Default.
- `--pdf-tables` — enables layout analysis, table and list detection on PDF input. Safe to pass for mixed folders; only PDFs incur extra time.
- `--no-headers --no-footers` — explicit flags to exclude page headers and footers. The SDK defaults to excluding both already, so these flags make intent explicit and guard against config changes. Recommended to include in all RAG pipelines for clarity.
- `--normalize` — converts smart quotes and em-dashes to ASCII. Prevents tokenizer inconsistencies in embedding models.
- `--metadata` — YAML front matter (title, author, date) for LangChain/LlamaIndex metadata filtering. `--metadata-format JSON` for JSON front matter.
- `--locations` — embeds `<!-- LOC: page, (l,t,r,b) -->` for chunk-to-source attribution. Adds ~20-40 bytes per paragraph; omit for strict token budgets.

**`--ocr` is unavailable for `convert-to-markdown`.** Scanned PDFs must use `df extract-text --ocr --output "$F.txt"` and load the text file directly. After running extract-text --ocr, the resulting .txt file can be loaded directly into LangChain/LlamaIndex. If heading/section structure is needed, pass the .txt through a subsequent chunking step or use a heading-detection prompt. If a `.md` output is empty or image-only, the file is likely scanned — use Step 3.

### 3. Archives and scanned documents

> **REQUIRED: always pass `--output`** — omitting it writes all extracted text to stdout. For PST/MBOX files this can be hundreds of megabytes, flooding the agent context with large volumes of raw text.

```bash
# Archive or email container:
df extract-text "$F" --subfiles --output "$F.txt"

# Scanned (image-only) PDF:
df extract-text "$F" --ocr --output "$F.txt"
```

Note: `--subfiles` and `--ocr` can both be passed together if a container is known to contain scanned attachments.

`--subfiles` unpacks archives (ZIP, tar) and email containers (PST, MBOX, EML, MSG) — each attachment is extracted and processed recursively. `--ocr` also applies to standalone scanned PDFs.

### 4. Summary

| File | Format | Pages | Output | Est. Tokens |
|------|--------|-------|--------|-------------|

~4 chars/token for English prose; ~3-4 for code; ~1-2 chars/token for CJK (Chinese/Japanese/Korean) and other non-Latin scripts.

> **For AI engineers:** For precise token counts use `cl100k_base` (OpenAI text-embedding-3), `o200k_base` (GPT-4o), or `client.beta.messages.count_tokens` (Anthropic SDK — the official way to count tokens for Claude models).

Typical RAG chunk targets: 256-512 tokens for embedding models (how the AI reads the text), 2000-8000 tokens for re-ranking or LLM-as-judge. If a single .md file exceeds your chunk target, split on heading boundaries using the structure output from `df extract-structure <file> --format json --output <file>.json`. If `--output` is omitted, the JSON is written next to the source as `<input>.json` — not to stdout, so there is no context-flood risk.

### Next steps

If you are not using LangChain/LlamaIndex, the .md files in `./corpus/` are plain text you can copy-paste or upload to any AI chat tool (e.g. ChatGPT, Claude).

Load `.md` files with LangChain `DirectoryLoader` or LlamaIndex `SimpleDirectoryReader`. Re-run Step 2 with `--locations` added if you need chunk-to-page attribution.

For word-level bounding boxes (useful for highlight mapping or citation systems), use `df extract-structure <file> --format json --words --output <file>.json` — each word carries page index and (l,t,r,b) bounds.

> **Not using a coding framework?** The .md files in `./corpus/` are plain text — you can drag them into any AI chat tool (ChatGPT, Claude, Gemini) or upload them to a knowledge base (Notion AI, SharePoint Copilot). No coding required.
