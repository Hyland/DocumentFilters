cmake_minimum_required(VERSION 3.15)

project(DocumentFiltersSample)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(
  CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG        v2.4.2  # Replace with the desired version
)
FetchContent_MakeAvailable(CLI11)

add_subdirectory (CombineDocuments)
add_subdirectory (CompareDocuments)
add_subdirectory (ConvertDocumentToClassicHTML)
add_subdirectory (ConvertDocumentToHDHTML)
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
add_subdirectory (ConvertDocumentToUTF8)
add_subdirectory (CreateBarcode)
add_subdirectory (ExtractSubfiles)
add_subdirectory (DocumentFiltersSamples)
add_subdirectory (GetDocumentType)
add_subdirectory (GetDocumentWords)
add_subdirectory (WatermarkDocument)

#set_target_properties(CLI11 PROPERTIES FOLDER "Third-Party")