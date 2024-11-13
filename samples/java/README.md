# Document Filters Java Samples

This repository contains samples and utilities for Document Filters, a set of
tools for converting and processing various document formats. Explore the
following directories and files to understand and use the capabilities of
Document Filters.

## Summary

The Document Filters Sample GitHub Repository includes:

- Samples for converting documents to different formats such as PDF, PNG, SVG,
  and more.
- Utilities for common tasks like extracting words from documents.
- A shared common library for Document Filters samples.

Explore the contents to leverage the power of Document Filters in your document
processing projects.

To get started on your own project, check out the [Getting
Started](https://docs.hyland.com/DocumentFilters/en_US/Print/getting_started_with_document_filters/create_a_java_api_application.html)
section in the documentation.

## Projects and Files

| Name                                                                           | Description                                                                                    |
| ------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------------- |
| [convert-document](./convert-document)                                         | Converts documents to either text or flow-html. Additionally, sub-files can also be processed. |
| [convert-document-to-html](./convert-document-to-html)                         | Converts documents to high-definition, paginated HTML.                                         |
| [convert-document-to-pdf](./convert-document-to-pdf)                           | Converts documents to PDF format.                                                              |
| [convert-document-to-png](./convert-document-to-png)                           | Converts documents to PNG format.                                                              |
| [convert-document-to-structured-xml](./convert-document-to-structured-xml)     | Converts documents to structured XML.                                                          |
| [convert-document-to-thumbnail](./convert-document-to-thumbnail)               | Converts documents to thumbnail images.                                                        |
| [convert-document-to-tiff](./convert-document-to-tiff)                         | Converts documents to TIFF format.                                                             |
| [convert-document-to-tiff-from-memory](./convert-document-to-tiff-from-memory) | Converts documents to TIFF format from memory.                                                 |
| [convert-document-to-tiff-stream](./convert-document-to-tiff-stream)           | Converts documents to TIFF format using streams.                                               |
| [extract-document-words](./extract-document-words)                             | Retrieves words from documents.                                                                |
| [docfilters-sample](./docfilters-sample)                                       | Common functionality for Document Filters samples.                                             |

## Getting Started

You can run the sample applications without a license key, with some
limitations.  See [Document Filters Evaluation](../../EVAL.md) for details.

To run the sample applications without feature limitations, ensure you have a
valid Document Filters license key. You can provide this key by either modifying
the
`docfilters-sample/src/main/java/com/documentfilters/DocumentFiltersLicense.java`
file or setting it in an environment variable named `DF_LICENSE_KEY`.

To compile all samples from the command line, execute:

```bash
cd samples/java
./gradlew jar
```

This will compile each sample and copy its `jar` file into the `libs` directory.

Each jar file contains a copy of the Document Filters java bindings, but it does
not include the native libraries. These must be passed when running each sample.
To help facilitate this, the `run.sh` and `run.cmd` scripts will download the
native binaries and font assets into a `runtimes` directory.

The sample scan be run using the syntax of `run.sh PROJECT_NAME args...`, for
example:

**Linux and MacOS**

```bash
./run.sh convert-document-to-pdf "my-file.docx"
```

**Windows**

```bash
.\run.cmd convert-document-to-pdf "my-file.docx"
```

## Troubleshooting

### No authorization to run this application, check license key

To run the sample applications, ensure you have a valid Document Filters license
key. You can provide this key by either modifying the
`docfilters-sample/src/main/java/com/documentfilters/DocumentFiltersLicense.java`
file or setting it in an environment variable named `DF_LICENSE_KEY`.

### Exception in thread "main" java.lang.UnsatisfiedLinkError: no ISYS11df in java.library.path

This means that it could not find the native Document Filters libraries. The
`run.sh` and `run.cmd` scripts will download the binaries for your platform. If
running from within an IDE, you may need to manually set the `java.library.path`
property to the directory containing the libraries.
