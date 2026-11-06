rootProject.name = "docfilters-cli"

// The suite lives beside the other ports' suites, under tests/, and builds as
// part of this project so that `gradlew check` runs it.
include(":tests")
project(":tests").projectDir = file("../tests/java")
