cmake_minimum_required(VERSION 3.16)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(DocumentFilters
    VERSION 25.3.0
    DESCRIPTION "Document Filters - c++17 bindings"
    LANGUAGES CXX
)

set(LIBRARY_NAME "DocumentFiltersObjects")

set(SOURCES
    "src/DocumentFiltersObjects.cpp"
    "src/DocFiltersAnnotations.cpp"
    "src/DocFiltersAnnotations.h"
    "src/DocFiltersBookmark.cpp"
    "src/DocFiltersCanvas.cpp"
    "src/DocFiltersCommon.cpp"
    "src/DocFiltersCompareDocumentSettings.cpp" 
    "src/DocFiltersCompareDocumentSource.cpp" 
    "src/DocFiltersCompareResultDifference.cpp"
    "src/DocFiltersCompareResultDifferenceDetail.cpp" 
    "src/DocFiltersCompareResults.cpp"
    "src/DocFiltersCompareSettings.cpp" 
    "src/DocFiltersDateTime.cpp"
    "src/DocFiltersExtractor.cpp"
    "src/DocFiltersFormat.cpp"
    "src/DocFiltersFormElement.cpp"
    "src/DocFiltersHyperlink.cpp"
    "src/DocFiltersOcrImage.cpp"
    "src/DocFiltersOcrStyleInfo.cpp"
    "src/DocFiltersOption.cpp"
    "src/DocFiltersPage.cpp"
    "src/DocFiltersPageElement.cpp"
    "src/DocFiltersPagePixels.cpp"
    "src/DocFiltersRenderPageProperties.cpp"
    "src/DocFiltersStreams.cpp"
    "src/DocFiltersStrings.cpp"
    "src/DocFiltersSubFile.cpp"
    "src/DocFiltersWord.cpp"
)

set(HEADERS include/DocumentFiltersObjects.h)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../c bindings)

# Create the library for the C++17 bindings
add_library(${LIBRARY_NAME} STATIC ${SOURCES} ${HEADERS})
set_property(TARGET ${LIBRARY_NAME} PROPERTY CXX_STANDARD 17)
target_include_directories(${LIBRARY_NAME} PUBLIC include)
target_compile_definitions(${LIBRARY_NAME} PRIVATE _LARGEFILE64_SOURCE)
target_link_libraries(${LIBRARY_NAME} PUBLIC DocumentFilters)

# Create an alias for the library
add_library(DocumentFilters::Cpp17 ALIAS ${LIBRARY_NAME})

export(TARGETS ${LIBRARY_NAME} FILE ${LIBRARY_NAME}Config.cmake)