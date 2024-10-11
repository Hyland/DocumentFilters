cmake_minimum_required(VERSION 3.16)

project(DocumentFilters
    VERSION 24.4.0
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
target_compile_features(${LIBRARY_NAME} PRIVATE cxx_std_17)
target_compile_definitions(${LIBRARY_NAME} PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
target_include_directories(${LIBRARY_NAME} PUBLIC include)
target_link_libraries(${LIBRARY_NAME} PUBLIC DocumentFilters)

# Create an alias for the library
add_library(DocumentFilters::Cpp17 ALIAS ${LIBRARY_NAME})

export(TARGETS ${LIBRARY_NAME} FILE ${LIBRARY_NAME}Config.cmake)