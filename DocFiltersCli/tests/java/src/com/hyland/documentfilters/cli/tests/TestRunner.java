/*
   (c) 2026 Hyland Software, Inc. and its affiliates. All rights reserved.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.hyland.documentfilters.cli.tests;

import java.io.File;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.regex.Pattern;

/**
 * Runs the suite on the JDK alone: no test framework is a dependency of this
 * tool, because it is a published sample that has to build from a clone.
 *
 * <p>Every class under the tool's root package whose name ends in {@code Tests}
 * is a test class --- most live under this package, a few beside the
 * package-private code they test --- and every public no-argument method on it
 * marked {@link Test} is a test. Each test gets a new instance, as under xUnit, so no state leaks between tests.
 *
 * <p>An optional argument filters the run. It is tried against class names
 * first, then against test ids such as {@code unit.PageRangeTests.mergesSpans},
 * where a pattern carrying no wildcard matches anywhere in the id. A filter that
 * selects nothing fails the run: an empty run is what a typo looks like.
 *
 * <p>{@code --list-tests} names what would run instead of running it, in the
 * same shape the other ports' runners list theirs; see {@link #list}.
 */
public final class TestRunner
{
	private static final String PACKAGE = TestRunner.class.getPackage().getName();

	/** Where discovery starts: the tool's root package, one above this one. */
	private static final String ROOT = PACKAGE.substring(0, PACKAGE.lastIndexOf('.'));

	private TestRunner()
	{
	}

	public static void main(String[] args) throws Exception
	{
		boolean listing = false;
		String pattern = null;
		for (String argument : args)
		{
			if ("--list-tests".equals(argument))
				listing = true;
			else if (pattern == null && !argument.startsWith("-"))
				pattern = argument;
		}

		System.exit(listing ? list(pattern) : run(pattern));
	}

	/**
	 * Names what would run, one id per line, and runs nothing.
	 *
	 * <p>tests/parity/compare_inventory.py reads this to check that every test
	 * the C# reference has exists here too --- the one failure a passing suite
	 * cannot show, because a case dropped in translation leaves no trace behind.
	 * The header is what tells that script which suite produced the listing, so
	 * it can pick a parser without being told.
	 */
	private static int list(String pattern) throws Exception
	{
		List<Method> tests = select(discover(), pattern);
		if (tests.isEmpty())
		{
			System.err.println("no test matches '" + (pattern == null ? "*Tests" : pattern)
				+ "': tried it against test class names, then against test ids");
			return 1;
		}

		List<String> ids = new ArrayList<>();
		for (Method test : tests)
			ids.add(idOf(test));
		Collections.sort(ids);

		StringBuilder out = new StringBuilder("The following Java tests are available:\n");
		for (String id : ids)
			out.append("    ").append(id).append('\n');
		System.out.print(out);
		return 0;
	}

	private static int run(String pattern) throws Exception
	{
		List<Class<?>> classes = discover();
		List<Method> tests = select(classes, pattern);
		if (tests.isEmpty())
		{
			System.err.println("no test matches '" + (pattern == null ? "*Tests" : pattern)
				+ "': tried it against test class names, then against test ids");
			return 1;
		}

		int failed = 0;
		long started = System.nanoTime();
		for (Method test : tests)
		{
			String failure = runOne(test);
			if (failure == null)
				continue;
			++failed;
			System.out.println("FAIL " + idOf(test));
			System.out.println(failure);
		}

		long millis = (System.nanoTime() - started) / 1000000;
		System.out.println();
		System.out.println("Ran " + tests.size() + " tests in " + millis + " ms: "
			+ (tests.size() - failed) + " passed, " + failed + " failed");
		return failed == 0 ? 0 : 1;
	}

	/** Null when the test passed, or what to print when it did not. */
	private static String runOne(Method test)
	{
		try
		{
			Object instance = test.getDeclaringClass().getConstructor().newInstance();
			test.invoke(instance);
			return null;
		}
		catch (InvocationTargetException e)
		{
			return describe(e.getCause());
		}
		catch (ReflectiveOperationException e)
		{
			return describe(e);
		}
	}

	private static String describe(Throwable thrown)
	{
		StringWriter text = new StringWriter();
		thrown.printStackTrace(new PrintWriter(text));
		// The runner's own frames say nothing about the failure.
		StringBuilder kept = new StringBuilder();
		for (String line : text.toString().split("\r?\n"))
		{
			if (line.contains("jdk.internal.reflect") || line.contains("java.lang.reflect.Method.invoke")
				|| line.contains("sun.reflect.") || line.contains(TestRunner.class.getName()))
				continue;
			kept.append("    ").append(line).append('\n');
		}
		return kept.toString();
	}

	private static List<Method> select(List<Class<?>> classes, String pattern)
	{
		if (pattern != null)
		{
			Pattern byClass = glob(pattern, false);
			List<Class<?>> matched = new ArrayList<>();
			for (Class<?> type : classes)
			{
				if (byClass.matcher(type.getSimpleName()).matches())
					matched.add(type);
			}
			if (!matched.isEmpty())
				return testsOf(matched, null);
		}
		return testsOf(classes, pattern == null ? null : glob(pattern, true));
	}

	private static List<Method> testsOf(List<Class<?>> classes, Pattern byId)
	{
		List<Method> tests = new ArrayList<>();
		for (Class<?> type : classes)
		{
			List<Method> methods = new ArrayList<>();
			for (Method method : type.getMethods())
			{
				if (method.isAnnotationPresent(Test.class) && method.getParameterCount() == 0
					&& !Modifier.isStatic(method.getModifiers()) && (byId == null || byId.matcher(idOf(method)).matches()))
					methods.add(method);
			}
			// getMethods has no defined order; by name, so that runs are repeatable.
			Collections.sort(methods, Comparator.comparing(Method::getName));
			tests.addAll(methods);
		}
		return tests;
	}

	/**
	 * The test's name below this package, e.g. {@code unit.PageRangeTests.mergesSpans},
	 * or below the root for a test outside it, e.g. {@code engine.DocumentFiltersEngineTests.x}.
	 */
	private static String idOf(Method test)
	{
		String name = test.getDeclaringClass().getName();
		String base = name.startsWith(PACKAGE + ".") ? PACKAGE : ROOT;
		return name.substring(base.length() + 1) + "." + test.getName();
	}

	/**
	 * A glob as a regular expression. A pattern with no wildcard matches anywhere
	 * when {@code substring} is set, since nobody types a whole id to run one test.
	 */
	private static Pattern glob(String pattern, boolean substring)
	{
		boolean wild = pattern.indexOf('*') >= 0 || pattern.indexOf('?') >= 0;
		StringBuilder regex = new StringBuilder();
		if (substring && !wild)
			regex.append(".*");
		for (char c : pattern.toCharArray())
		{
			if (c == '*')
				regex.append(".*");
			else if (c == '?')
				regex.append('.');
			else
				regex.append(Pattern.quote(String.valueOf(c)));
		}
		if (substring && !wild)
			regex.append(".*");
		return Pattern.compile(regex.toString());
	}

	/** Every {@code *Tests} class under the root package, found on the class path's directories. */
	private static List<Class<?>> discover() throws ClassNotFoundException
	{
		List<String> names = new ArrayList<>();
		String packagePath = ROOT.replace('.', File.separatorChar);
		for (String entry : System.getProperty("java.class.path").split(Pattern.quote(File.pathSeparator)))
		{
			File root = new File(entry, packagePath);
			if (root.isDirectory())
				walk(root, ROOT, names);
		}
		Collections.sort(names);

		List<Class<?>> classes = new ArrayList<>();
		for (String name : names)
		{
			Class<?> type = Class.forName(name);
			if (!Modifier.isAbstract(type.getModifiers()))
				classes.add(type);
		}
		return classes;
	}

	private static void walk(File directory, String packageName, List<String> names)
	{
		File[] children = directory.listFiles();
		if (children == null)
			return;
		Arrays.sort(children);
		for (File child : children)
		{
			String name = child.getName();
			if (child.isDirectory())
				walk(child, packageName + "." + name, names);
			else if (name.endsWith("Tests.class") && name.indexOf('$') < 0)
				names.add(packageName + "." + name.substring(0, name.length() - ".class".length()));
		}
	}
}
