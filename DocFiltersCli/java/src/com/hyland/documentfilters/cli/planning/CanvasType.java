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

package com.hyland.documentfilters.cli.planning;

/**
 * The engine's output canvas devices, as accepted when making an output canvas.
 *
 * <p>Defined here rather than taken from the Java binding, so that this
 * pure-contract package names no binding type and loads without the native
 * library. The values are the engine's {@code IGR_DEVICE_*} numbers, which are
 * ABI and therefore cannot drift.
 */
public enum CanvasType
{
	PNG(0),
	JPG(1),
	PDF(2),
	TIF(3),
	BMP(4),
	XML(5),
	HTML(6),
	PBM(7),
	PGM(8),
	PPM(9),
	WEBP(10),
	XPS(11),
	SVG(12),
	EPS(13),
	PS(14),
	WEB_SAFE(15),
	TGA(16),
	BRK(17),
	PCX(18),
	DCX(19),
	JPEG2000(20),
	GIF(21),
	JSON(22),
	MARKDOWN(23);

	private final int _device;

	CanvasType(int device)
	{
		_device = device;
	}

	/** The {@code IGR_DEVICE_*} number the engine takes. */
	public int getDevice()
	{
		return _device;
	}
}
