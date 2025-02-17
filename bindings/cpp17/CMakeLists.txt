set(LIBRARY_NAME "DocumentFilters")

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

set(HEADERS
    include/DocumentFiltersObjects.h
)

set(DEPENDENCIES
    ISYS11df
)

# Create the executable
add_library(${LIBRARY_NAME} ${SOURCES} ${HEADERS})
target_compile_features(${LIBRARY_NAME} PRIVATE cxx_std_17)
target_compile_definitions(${LIBRARY_NAME} PRIVATE _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
target_include_directories(${LIBRARY_NAME} PUBLIC include)
target_link_libraries(${LIBRARY_NAME} ${DEPENDENCIES})
target_link_directories(${LIBRARY_NAME} PUBLIC /usr/local/lib "$ENV{HOME}/builds/df/develop/linux-intel-gcc-64" "c:\\dev\\builds\\x64")
