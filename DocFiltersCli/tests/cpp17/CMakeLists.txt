cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
#  The C++17 test suite --- a separate CMake project from cpp17/, the same
#  relationship tests/csharp/DocFiltersCli.Tests.csproj has to
#  csharp/DocFiltersCli.csproj: this project references the other by path
#  and adds a test framework, so building the tool itself never needs one.
#
#  Catch2 is test-only, in the same sense xUnit is test-only for the C#
#  suite (see DF-3104's no-new-dependencies constraint, and tests/csharp/
#  DocFiltersCli.Tests.csproj's own comment to that effect): nothing here
#  ships in the tool, and a customer running the sample never fetches it.
# ---------------------------------------------------------------------------

project(DocFiltersCliTests LANGUAGES CXX)

# The engine version is a property of this tree, read here rather than passed
# in on the command line. bindings/c/CMakeLists.txt resolves an unset
# DOCFILTERS_VERSION to "latest", which would quietly put this port on a
# different engine from the reference it is compared against --- so the one
# place that knows the answer, samples/DF_VERSION.txt, answers it. Set before
# the add_subdirectory below, because that is where the binding declares the
# cache entry, and the first setting of a cache entry wins.
#
# -DDOCFILTERS_VERSION still overrides it; cli.yml passes it explicitly.
if(NOT DOCFILTERS_VERSION)
  file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/../../../samples/DF_VERSION.txt
       DOCFILTERS_VERSION LIMIT_COUNT 1)
  string(STRIP "${DOCFILTERS_VERSION}" DOCFILTERS_VERSION)
  set(DOCFILTERS_VERSION "${DOCFILTERS_VERSION}" CACHE STRING
      "Document Filters version" FORCE)
  message(STATUS "DOCFILTERS_VERSION from samples/DF_VERSION.txt: ${DOCFILTERS_VERSION}")
endif()

# The test binary goes where the tool's does --- cpp17/CMakeLists.txt sets the
# same three variables, but a `set` in a subdirectory does not reach back up
# here, so the test target used to land wherever the generator chose:
# `build/Release/` under Visual Studio, `build/` under Makefiles or Ninja.
# Nothing could then name it in one way that worked on both, which made
# languages.yml's `list_tests` either wrong on one platform or a shell lookup.
# All three variables, because a multi-config generator appends the
# configuration to the plain one and reads the per-config ones verbatim.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../cpp17 cpp17)

set(DOCFILTERSCLI_TEST_SOURCES
    Conformance/Fixture.cpp
    Conformance/FixtureTests.cpp
    Fakes/InMemoryFileSystem.cpp
    Fakes/FakeDocumentFiltersEngine.cpp
    Unit/StringsTests.cpp
    Unit/CaseProbeTests.cpp
    Unit/ExitCodeTests.cpp
    Unit/JsonWriterTests.cpp
    Unit/FormatCatalogTests.cpp
    Unit/PageRangeTests.cpp
    Unit/OutputNameTemplateTests.cpp
    Unit/NameMatcherTests.cpp
    Unit/ValueParsersTests.cpp
    Unit/OptionsValidatorTests.cpp
    Unit/InputEnumeratorTests.cpp
    Unit/OutputPlannerTests.cpp
    Unit/ArgumentTableTests.cpp
    Conformance/ContractDumpTests.cpp
    Unit/ArgumentBindingTests.cpp
    Conformance/PlanCorpus.cpp
    Conformance/PlanCorpusTests.cpp
    Unit/SubfileExtractionTests.cpp
    Unit/TextConversionTests.cpp
    Unit/ExtractorOpenSettingsTests.cpp
    Unit/DiagnosticsTests.cpp
    Unit/DiagnosticSwitchTests.cpp
    Unit/DocumentFiltersSessionTests.cpp
    Unit/MetadataReportTests.cpp
    Unit/EngineReportOutcomeTests.cpp
    Unit/EngineSubfileOutcomeTests.cpp
    Unit/CanvasConversionTests.cpp
    Unit/EngineOutcomeTests.cpp
    Cli/CliHarness.cpp
    Cli/CliTests.cpp
    Conformance/JsonShapeTests.cpp
    Conformance/ErrorCodeTests.cpp
    Unit/SystemConsoleTests.cpp
)

add_executable(docfilters_tests ${DOCFILTERSCLI_TEST_SOURCES})
target_compile_features(docfilters_tests PRIVATE cxx_std_17)
target_link_libraries(docfilters_tests PRIVATE docfilters_lib Catch2::Catch2WithMain)

# So a test can #include "Fakes/InMemoryFileSystem.h" or "Conformance/Fixture.h"
# by the same qualified path every other test in this tree uses to reach them.
target_include_directories(docfilters_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# The conformance fixtures are shared with the C#, Python and Java suites and
# live one level up (tests/conformance/) so that no language owns them --- the
# same file tree tests/csharp/DocFiltersCli.Tests.csproj copies next to itself
# via CopyToOutputDirectory. This suite reads them from the source tree
# directly instead of copying them, which is the one place this port's build
# wiring departs from the C# reference's: no copy step to go stale, at the
# cost of the test binary no longer being relocatable on its own. See the
# phase-0 write-up for the tradeoff.
target_compile_definitions(docfilters_tests PRIVATE
    CONFORMANCE_FIXTURES_DIR="${CMAKE_CURRENT_LIST_DIR}/../conformance")

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

# DL_PATHS is not optional here, and its absence fails the *build* rather than a
# test. catch_discover_tests enumerates cases by running the test executable as a
# post-build step, and that executable links the Document Filters shared library
# --- which cpp17/CMakeLists.txt puts next to the tool in bin/, not next to the
# test binary. Without this the process cannot start, the enumeration returns
# nothing, and the step exits non-zero: MSB3073 out of CatchAddTests.cmake, after
# a compile and link that both succeeded. The same path is what lets `ctest` run
# the suite afterwards.
catch_discover_tests(docfilters_tests DL_PATHS "$<TARGET_FILE_DIR:docfilters>")
