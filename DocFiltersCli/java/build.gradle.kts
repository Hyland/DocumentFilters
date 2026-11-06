plugins {
    application
}

// What `docfilters version` prints on its first line, read back from the jar
// manifest at run time.
//
// ../VERSION.txt is the single source of it, as it is for the other three
// ports: the C# build reads it through Directory.Build.props, the C++17 build
// through CMakeLists.txt, and the Python package at import. A version written
// here instead would be one more place for the release workflow to stamp, and
// the first thing to fall behind -- `docfilters version` is compared between
// the implementations byte for byte, so a port carrying its own number fails
// the parity check rather than merely looking untidy.
//
// -Pversion=x.y.z on the command line still wins, which is how Gradle's own
// property precedence works and matches the -p:Version= the C# release build
// takes.
version = providers.gradleProperty("version").orNull?.takeIf { it.isNotBlank() }
    ?: file("../VERSION.txt").readText().trim().ifEmpty { "unknown" }

java.sourceCompatibility = JavaVersion.VERSION_1_8
java.targetCompatibility = JavaVersion.VERSION_1_8

sourceSets {
    main {
        java.setSrcDirs(listOf("src"))
    }
}

dependencies {
    implementation(files("../../bindings/java/lib/ISYS11df.jar"))
}

tasks.withType<JavaCompile>().configureEach {
    options.encoding = "US-ASCII"
    // Built against the Java 8 API rather than only for its bytecode, so a call
    // added in a later JDK fails here instead of on a customer's Java 8.
    if (JavaVersion.current().isJava9Compatible) {
        options.release.set(8)
    }
    options.compilerArgs.addAll(listOf("-Xlint:all", "-Xlint:-options", "-Werror"))
}

application {
    applicationName = "docfilters"
    mainClass.set("com.hyland.documentfilters.cli.Program")
}

tasks.jar {
    manifest {
        attributes("Implementation-Version" to project.version)
    }
}

// The binding loads its native libraries with System.loadLibrary, which searches
// java.library.path and nothing else, and the JVM fixes that path at start-up ---
// so unlike the other ports, --library and DF_PATH cannot pick the library from
// inside the program. The launcher has to: `gradlew run` passes DF_PATH through
// here, and the installed start script takes it from DOCFILTERS_OPTS, as in
//   DOCFILTERS_OPTS=-Djava.library.path=/path/to/native bin/docfilters version
tasks.named<JavaExec>("run") {
    System.getenv("DF_PATH")?.let { systemProperty("java.library.path", it) }
}
