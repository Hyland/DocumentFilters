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

package com.hyland.documentfilters.cli.runtime;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/**
 * Ctrl+C, as a request rather than an exit.
 *
 * <p>Pressing it does not kill the process: it is recorded here and acted on at
 * a point where stopping leaves nothing half-written. {@link BatchRunner} checks
 * it between documents, and the engine's open callback polls it during a parse.
 */
public final class Interruption
{
	/** Volatile because the signal is delivered on another thread than the one doing the work. */
	private volatile boolean _requested;

	/** True once the user has interrupted the run. */
	public boolean isRequested()
	{
		return _requested;
	}

	/** Records that the user asked to stop. */
	public void request()
	{
		_requested = true;
	}

	/**
	 * An interruption wired to the process's Ctrl+C.
	 *
	 * <p>The JDK has no public way to intercept SIGINT; a shutdown hook runs only
	 * once the JVM has already decided to exit. {@code sun.misc.Signal} can, and
	 * exists on every mainstream JVM from 8 onwards, but it is not part of the
	 * platform --- so it is reached reflectively, and where it is missing Ctrl+C
	 * keeps its default behaviour of ending the process.
	 */
	public static Interruption install()
	{
		Interruption interruption = new Interruption();
		try
		{
			Class<?> signalClass = Class.forName("sun.misc.Signal");
			Class<?> handlerClass = Class.forName("sun.misc.SignalHandler");
			Object signal = signalClass.getConstructor(String.class).newInstance("INT");
			Object handler = Proxy.newProxyInstance(handlerClass.getClassLoader(), new Class<?>[] { handlerClass },
				new Handler(interruption));
			signalClass.getMethod("handle", signalClass, handlerClass).invoke(null, signal, handler);
		}
		catch (ReflectiveOperationException | RuntimeException | LinkageError e)
		{
			// No way to intercept the signal here: Ctrl+C ends the process, as it
			// would for any other program.
		}
		return interruption;
	}

	/** The {@code SignalHandler}: records the request and swallows the signal. */
	private static final class Handler implements InvocationHandler
	{
		private final Interruption _interruption;

		Handler(Interruption interruption)
		{
			_interruption = interruption;
		}

		@Override
		public Object invoke(Object proxy, Method method, Object[] args)
		{
			switch (method.getName())
			{
				case "handle":
					_interruption.request();
					return null;
				case "hashCode":
					return System.identityHashCode(proxy);
				case "equals":
					return proxy == args[0];
				case "toString":
					return "Interruption.Handler";
				default:
					return null;
			}
		}
	}
}
