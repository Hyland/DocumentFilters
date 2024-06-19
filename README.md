<div align="center">

# Hyland Document Filters

Providing developers with everything needed for file inspection, extraction, and transformation, in one powerful software development kit (SDK).

[Home Page](http://www.documentfilters.com) | [Documentation](https://docs.hyland.com/Portal_Public/Products/en/Document_Filters.htm) | [Samples](./samples) | [Release Notes](https://docs.hyland.com/DocumentFilters/en_US/Print/release_notes/index.html) | [Blog](https://hyland.github.io/DocumentFilters/blog/?utm_source=github)

</div>

---

Hyland’s Document Filters SDK equips software developers with powerful tools to embed rich document processing capabilities into their applications. It is ideal for enabling file inspection, data extraction, content manipulation, and document conversion across a wide array of document types and programming languages.

## Key Features

- **Deep Content Inspection**: Identify and extract data from documents, emails, archives, and container formats, analyzing all associated text and metadata.
- **Rendering and Annotation**: Render high-definition content in a web-safe format with tools for redaction, annotations, and more.
- **Content Transformation**: Export and convert content for use in other locations, replicate original files, and combine pages from different documents to create packets.
- **Advanced Filtering Platform**: Deploy across 31 software platforms and architectures, supporting nearly any programming language and over 600 file formats.

## Applications

Document Filters can be applied to:

- Enhancing AI/ML models by structuring unstructured data
- Content indexing
- E-discovery
- Data migration

## Getting Started

In this repository, you'll find shared libraries and DLLs for releases since version 23.2. For SDK installers, samples, and documentation, visit the [Hyland Community website](https://community.hyland.com).

## Additional Resources

[Document Filters Blog](https://hyland.github.io/DocumentFilters/blog/?utm_source=github)

In this repository, you'll find the shared libraries and DLLs for releases published since version 23.2. If you're searching for the SDK installers that include samples and documentation, kindly obtain them from the Hyland Community website.

## Getting Started

Document Filters is callable from C#, Java, Python, C/C++ or any language that supports calling C APIs.

You can try Document Filters without a license key, in a feature-limited evaluation.  See [Document Filters Evaluation](./EVAL.md) for details.

To use Document Filters without feature limitations, you will need either an evaluation license key or a full license key. You can request an evaluation license key from [Hyland Software](http://www.documentfilters.com) by selecting **Request a free trial**.

<table width=100% border=0 style="max-width: 800px">
  <tbody>
    <tr>
      <td>
        <img align="left" width=52px src="https://user-images.githubusercontent.com/371009/230673036-fad1e8e6-5d48-49b1-a9c1-6f9834e0d165.png" alt="C#">
        <div>
          <a href="https://docs.hyland.com/DocumentFilters/en_US/Print/getting_started_with_document_filters/create_a_net_api_c_and_vbnet_application.html">Using DocFilters in C#</a> &nbsp<br/>
        </div>
      </td>
      <td>
        <img align="left" width=52px src="https://raw.githubusercontent.com/devicons/devicon/master/icons/python/python-original.svg" alt="Python">
        <div>
          <a href="https://docs.hyland.com/DocumentFilters/en_US/Print/getting_started_with_document_filters/create_a_python_api_application.html">Using DocFilters in Python</a>
        </div>
      </td>
      <td>
        <img align="left" width=52px height=52px src="https://upload.wikimedia.org/wikipedia/en/3/30/Java_programming_language_logo.svg" alt="Java">
        <div>
          <a href="https://docs.hyland.com/DocumentFilters/en_US/Print/getting_started_with_document_filters/create_a_java_api_application.html">Using DocFilters in Java</a>
        </div>
      </td>
      <td>
        <img align="left" width=52px src="https://upload.wikimedia.org/wikipedia/commons/1/18/ISO_C%2B%2B_Logo.svg" style="padding-right:3pt" alt="C++">
        <div>
          <a href="https://docs.hyland.com/DocumentFilters/en_US/Print/getting_started_with_document_filters/create_a_c_api_class_wrapper_around_native_library_functions_application.html">Using DocFilters in C++</a> &nbsp<br/>
        </div>
      </td>
    </tr>
  </tbody>
</table>

## Documentation

- [Getting Started](https://docs.hyland.com/DocumentFilters/en_US/Print/getting_started_with_document_filters.html) contains details of integrating Document Filters with your language of choice.

- [API Documentation](https://docs.hyland.com/DocumentFilters/en_US/Print/reference.html) captures the details of calling Document Filters, from then low-level C API, through to object language.

- [Platforms, Formats and More](https://docs.hyland.com/DocumentFilters/en_US/Print/supported_platforms.html) as the stats on where you can run Document Filters, and what you can process with it.

## Samples

This repo contains samples that demonstrating using Document Filters for different uses cases in different languages.

| Task                                    | Projects                                                                                                                                                                                                        |
| --------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Extract text from a file                | The [ConvertDocument](./samples/csharp/ConvertDocument/) sample demonstrates extracting plain text from over 600 file types.                                                                                    |
| Extract files from a container          | [ExtractSubfile](./samples/csharp/ExtractSubfiles/) or [ConvertDocuments](./samples/csharp/ConvertDocument/) shows how to extract sub-files from archives, containers, or other file types.                     |
| Convert a file to PDF                   | [ConvertDocumentToPDF](./samples/csharp/ConvertDocumentToPDF/) and [ConvertDocumentWithComments](./samples/csharp/ConvertDocumentWithComments/) demonstrate rendering input files to create new PDF renditions. |
| Combine multiple files into one         | [CombineDocuments](./samples/csharp/CombineDocuments/) sample shows how multiple documents can be combined into a single output.                                                                                |
| Apply markup and annotations            | [MarkupAnnotationsDemo](./samples/csharp/MarkupAnnotationsDemo/) demonstrates the markup API and modifying pages in the final output.                                                                           |
| Create a redacted version of a document | [RedactionDemo](./samples/csharp/RedactionDemo/) demonstrates redacting content while rendering to a canvas, removing text and images from the output.                                                          |

Check more samples here: [C#](./samples/csharp/README.md) | [Java](./samples/java/) | [Python](./samples/python/) | [C++](./samples/cpp/)

## License

Sample Code: MIT License | Release Binaries: Commercial License

Here you'll find a collection of sample code that is covered under the MIT
License. This means that you are free to use, modify, and distribute the sample
code in accordance with the terms specified in the MIT License.

However, please note that the release binaries provided in this repository are
governed by a different commercial license. These binaries are intended for
users who require pre-compiled and ready-to-use versions of our software. The commercial license grants specific permissions and restrictions for the use of
the release binaries.

Please see the [LICENSE](LICENSE.md) for details.
