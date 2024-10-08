cmake_minimum_required(VERSION 3.15)

project(DocumentFiltersSample)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

add_subdirectory (DocumentFiltersBinaries)
add_subdirectory (DocumentFiltersObjects)
add_subdirectory (DocumentFiltersSamples)
add_subdirectory (ConvertDocumentToHDHTML)
add_subdirectory (ConvertDocumentToInteractiveHDHTML)
add_subdirectory (ConvertDocumentToJSON)
add_subdirectory (ConvertDocumentToMarkdown)
add_subdirectory (ConvertDocumentToPDF)
add_subdirectory (ConvertDocumentToPNG)
add_subdirectory (ConvertDocumentToPostscript)
add_subdirectory (ConvertDocumentToStructuredXML)
add_subdirectory (ConvertDocumentToSVG)
add_subdirectory (ConvertDocumentToThumbnail)
add_subdirectory (ConvertDocumentToTIFF)
add_subdirectory (ConvertDocumentToTIFFStream)
add_subdirectory (ConvertDocumentToUnicode)
add_subdirectory (ConvertDocumentToUTF8)
add_subdirectory (ConvertDocumentToXML)
add_subdirectory (GetDocumentType)
add_subdirectory (GetDocumentWords)
add_subdirectory (WatermarkDocument)