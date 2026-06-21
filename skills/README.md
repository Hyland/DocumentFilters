# Document Filters AI Skills

Skills for AI coding agents and AI pipeline agents. Each skill is a structured prompt that teaches an AI agent how to use the [Hyland Document Filters](https://www.documentfilters.com) SDK.

## Skills

| Skill | Audience | Description |
|---|---|---|
| [use-document-filters-api](./use-document-filters-api/skill.md) | Developer | Initialize and use the DF SDK in Python, C#, Java, C++ |
| [extract-text](./extract-text/skill.md) | Developer / Non-technical | Extract plain text from any supported file format |
| [convert-to-pdf](./convert-to-pdf/skill.md) | Developer / Non-technical | Render a document to PDF |
| [convert-to-html](./convert-to-html/skill.md) | Developer / Non-technical | Render a document to HD HTML |
| [convert-to-png](./convert-to-png/skill.md) | Developer / Non-technical | Render each page of a document to PNG images |
| [convert-to-markdown](./convert-to-markdown/skill.md) | Developer / Non-technical | Convert a document to Markdown |
| [process-documents-for-ai](./process-documents-for-ai/skill.md) | AI Builder | Extract and chunk text for LLM ingestion |
| [summarize-document](./summarize-document/skill.md) | Non-technical / AI Builder | Extract text from a file and summarize it |
| [extract-document-metadata](./extract-document-metadata/skill.md) | Developer / Non-technical / AI Builder | Get file type, page count, and metadata |
| [redact-document](./redact-document/skill.md) | Developer / Non-technical | Redact sensitive content from a document |
| [compare-documents](./compare-documents/skill.md) | Developer / Non-technical | Compare two documents and report differences |
| [process-email-archives](./process-email-archives/skill.md) | Developer / AI Builder | Extract messages and attachments from PST, MBOX, EML, and MSG archives |
| [extract-document-structure](./extract-document-structure/skill.md) | Developer / AI Builder | Extract document structure as Markdown or JSON with paragraphs, words, and bounding boxes |

## Installation

### Automated (recommended)

**macOS / Linux / WSL:**
```bash
# Install all skills for all platforms
bash skills/install.sh

# Install for a specific platform only
bash skills/install.sh --platform claude-code
bash skills/install.sh --platform cursor
bash skills/install.sh --platform copilot

# Install a subset of skills
bash skills/install.sh --skills extract-text,convert-to-pdf
```

**Windows (PowerShell):**
```powershell
# Install all skills for all platforms
.\skills\install.ps1

# Install for a specific platform only
.\skills\install.ps1 -Platform claude-code

# Install a subset of skills
.\skills\install.ps1 -Skills extract-text,convert-to-pdf
```

### Manual

**Claude Code** — copy the skill folder into your plugins directory:
```bash
cp -r skills/extract-text ~/.claude/plugins/document-filters-extract-text
```

**Cursor** — append a skill's content to `.cursorrules` in your project root:
```bash
cat skills/extract-text/skill.md >> .cursorrules
```

**GitHub Copilot** — append a skill's content to `.github/copilot-instructions.md`:
```bash
mkdir -p .github
cat skills/extract-text/skill.md >> .github/copilot-instructions.md
```

**Any other AI tool** — paste the contents of a `skill.md` file into your tool's system prompt or custom instructions field.
