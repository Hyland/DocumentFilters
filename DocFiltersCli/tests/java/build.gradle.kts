plugins {
    java
}

java.sourceCompatibility = JavaVersion.VERSION_1_8
java.targetCompatibility = JavaVersion.VERSION_1_8

sourceSets {
    main {
        java.setSrcDirs(listOf("src"))
    }
}

dependencies {
    implementation(rootProject)
    // The engine adapter's tests construct the binding's exception type, and the
    // tool's own dependency on the jar does not reach this project's compile.
    implementation(rootProject.files("../../bindings/java/lib/ISYS11df.jar"))
}

tasks.withType<JavaCompile>().configureEach {
    options.encoding = "US-ASCII"
    if (JavaVersion.current().isJava9Compatible) {
        options.release.set(8)
    }
    options.compilerArgs.addAll(listOf("-Xlint:all", "-Xlint:-options", "-Werror"))
}

// The suite is its own small runner, on the JDK alone; see TestRunner. Arguments
// after --args are name filters, as for the other ports' runners.
val runTests by tasks.registering(JavaExec::class) {
    group = "verification"
    description = "Runs the Java port's unit, conformance and CLI tests."
    classpath = sourceSets.main.get().runtimeClasspath
    mainClass.set("com.hyland.documentfilters.cli.tests.TestRunner")
    workingDir = projectDir
}

tasks.named("check") {
    dependsOn(runTests)
}
