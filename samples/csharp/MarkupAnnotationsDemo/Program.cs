/*
   (c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved.

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
using Hyland.DocumentFilters;
using McMaster.Extensions.CommandLineUtils;
using System.Reflection;

/// <summary>
/// Demonstrates creating different annotations onto a blank page. The same
/// annotation logic can be used to annotate any page rendered into a canvas.
/// </summary>
class Program
{
    [Option("-o,--output", Description = "Output file name")]
    public string OutputFileName { get; set; } = "output.pdf";

    private readonly Hyland.DocumentFilters.Api _api = new();

    public int OnExecute()
    {
        _api.Initialize(DocumentFiltersLicense.Get(), ".");

        using Stream? cdStream = GetTestResource("media-import-audio-cd.svg");
        using Stream? flowerStream = GetTestResource("colorful_flower.png");
        using Stream? newsletterStream = GetTestResource("Newsletter.docx");

        int blankWidth = (int)(8.5 * 96);
        int blankHeight = (int)(11 * 96);

        using (Extractor newsletter = _api.OpenExtractor(newsletterStream, OpenMode.Paginated))
        using (Canvas canvas = _api.MakeOutputCanvas(OutputFileName, ExtToCanvasType(Path.GetExtension(OutputFileName))))
        {
            Action<string> printTitle = (string title) =>
            {
                canvas.SetFont("Arial", 30, 0);
                canvas.SetBrush(0, 0);
                canvas.TextOut(10, 10, title);
            };
            Action<string, int, int> printDescription = (string text, int x, int y) =>
            {
                canvas.SetFont("Arial", 10, 0);
                canvas.SetBrush(0, 0);
                canvas.TextOut(x, y, text);
            };

            // Markup content from the newsletter, highlighting every instance of 'financial'.
            RenderHighlights(newsletter, canvas);

            // Create a new blank page and add some hyperlinks
            RenderLinks(blankWidth, blankHeight, canvas, printTitle);

            // Create a new blank page and draw some text with markup
            RenderTextMarkup(blankWidth, blankHeight, canvas, printTitle);

            // Create a new blank page and draw some sticky notes
            RenderStickyNotes(blankWidth, blankHeight, canvas, printTitle);

            // Create a new blank page and draw lines with different line endings
            RenderLineEndings(blankWidth, blankHeight, canvas, printTitle);

            // Create a new blank page and draw polygons and polylines
            RenderPolygons(blankWidth, blankHeight, canvas, printTitle);

            // Create a new blank page and draw rectangles, ellipse, and stamps
            RenderShapesAndStamps(cdStream, flowerStream, blankWidth, blankHeight, canvas, printTitle);

            // Create a new blank page and draw free text
            RenderText(blankWidth, blankHeight, canvas, printTitle, printDescription);

            // Create a new blank page and draw 2D Barcodes
            Render2DBarcodes(blankWidth, blankHeight, canvas, printTitle);

            // Create a new blank page and draw 1-D Barcodes
            Render1DBarcodes(blankWidth, blankHeight, canvas);
        }

        Console.Out.WriteLine($"Saved to {OutputFileName}");

        return 0;

        static void RenderHighlights(Extractor newsletter, Canvas canvas)
        {
            using (Page page = newsletter.FirstPage)
            {
                canvas.RenderPage(page);

                foreach (Word? word in page.Words.Where(x => x.Text.Contains("financial", StringComparison.InvariantCultureIgnoreCase)))
                {
                    canvas.Annotate(new Hyland.DocumentFilters.Annotations.Highlight
                    {
                        Rect = new System.Drawing.Rectangle(word.X, word.Y, word.Width, word.Height),
                        Color = System.Drawing.Color.Yellow
                    });
                }
            }
        }

        static void RenderLinks(int blankWidth, int blankHeight, Canvas canvas, Action<string> printTitle)
        {
            canvas.BlankPage(blankWidth, blankHeight);
            printTitle("Links");

            // Go to next page
            {
                System.Drawing.Rectangle rect = new(200, 200, 200, 100);
                canvas.SetBrush(0xff00ff, 1);
                canvas.Rect(rect);
                canvas.SetBrush(0xffffff, 1);
                canvas.SetFont("Arial", 12, 0);
                canvas.TextOut(rect.Left + 10, rect.Top + 10, "Go to next page");

                canvas.Annotate(new Hyland.DocumentFilters.Annotations.Link
                {
                    Rect = rect,
                    Action = new Hyland.DocumentFilters.Annotations.ActionGoTo
                    {
                        Page = 3,
                    }
                });
            }

            // Go to hyland website
            {
                System.Drawing.Rectangle rect = new(200, 400, 200, 100);
                canvas.SetBrush(0xff00ff, 1);
                canvas.Rect(rect);
                canvas.SetBrush(0xffffff, 1);
                canvas.SetFont("Arial", 12, 0);
                canvas.TextOut(rect.Left + 10, rect.Top + 10, "www.hyland.com");

                canvas.Annotate(new Hyland.DocumentFilters.Annotations.Link
                {
                    Rect = rect,
                    Action = new Hyland.DocumentFilters.Annotations.ActionURI
                    {
                        Uri = "http://www.hyland.com"
                    }
                });
            }
        }

        static void RenderTextMarkup(int blankWidth, int blankHeight, Canvas canvas, Action<string> printTitle)
        {
            Action<string, int, int, Hyland.DocumentFilters.Annotations.Annotation> markup = (string text, int x, int y, Hyland.DocumentFilters.Annotations.Annotation anno) =>
            {
                int w = canvas.TextWidth(text);
                int h = canvas.TextHeight(text);
                canvas.TextOut(x, y, text);
                anno.Rect = new System.Drawing.Rectangle(x, y, w, h);
                canvas.Annotate(anno);
            };

            canvas.BlankPage(blankWidth, blankHeight);
            printTitle("Text Markup");

            canvas.SetBrush(0, 0);
            canvas.SetFont("Arial", 20, 0);

            markup("Strikeout", 50, 75, new Hyland.DocumentFilters.Annotations.Strikeout { });
            markup("Underline", 50, 150, new Hyland.DocumentFilters.Annotations.Underline { });
            markup("Squiggly", 50, 225, new Hyland.DocumentFilters.Annotations.Squiggly { });
        }

        static void RenderStickyNotes(int blankWidth, int blankHeight, Canvas canvas, Action<string> printTitle)
        {
            canvas.BlankPage(blankWidth, blankHeight);
            printTitle("Notes");
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.StickyNote
            {
                Rect = new System.Drawing.Rectangle(400, 100, 80, 80),
                Text = "Another Note",
                Color = System.Drawing.Color.Red,
                Author = "John Smith",
                DateModified = new DateTime(2019, 11, 17),
                Replies =
                        {
                            new Hyland.DocumentFilters.Annotations.Note
                            {
                                Text = "Second Reply",
                                Author = "Bill Gates"
                            },
                            new Hyland.DocumentFilters.Annotations.Note
                            {
                                Text = "Third Reply",
                                Author = "Steve Jobs"
                            },
                            new Hyland.DocumentFilters.Annotations.Note
                            {
                                Text = "Forth Reply"
                            }
                        }
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.StickyNote
            {
                Rect = new System.Drawing.Rectangle(10, 100, 30, 30),
                Text = "Smaller Note",
                Color = System.Drawing.Color.Yellow
            });
        }

        static void RenderLineEndings(int blankWidth, int blankHeight, Canvas canvas, Action<string> printTitle)
        {
            int w = 100, h = 100, s = 50;
            int x = s, y = s;
            int i = 0;
            for (Hyland.DocumentFilters.Annotations.LineEndingType b = Hyland.DocumentFilters.Annotations.LineEndingType.None; b != Hyland.DocumentFilters.Annotations.LineEndingType.Slash; b++)
            {
                for (Hyland.DocumentFilters.Annotations.LineEndingType e = Hyland.DocumentFilters.Annotations.LineEndingType.None; e != Hyland.DocumentFilters.Annotations.LineEndingType.Slash; e++)
                {
                    if (i == 0 || i % 30 == 0)
                    {
                        canvas.BlankPage(blankWidth, blankHeight);
                        printTitle("Lines and Arrows");
                        x = s;
                        y = 75;
                    }

                    if (x + w > blankWidth)
                    {
                        x = s;
                        y += h + s;
                    }

                    canvas.Annotate(new Hyland.DocumentFilters.Annotations.Line
                    {
                        Points =
                                {
                                    new System.Drawing.Point(x, (int)(y + (h * 0.3))),
                                    new System.Drawing.Point(x + w, (int)(y + (h * 0.6))),
                                },
                        LineEndings =
                                {
                                    Begin = b,
                                    End = e
                                },
                        Color = System.Drawing.Color.Black,
                        Border = new Hyland.DocumentFilters.Annotations.BorderStyle
                        {
                            Width = 3
                        },
                        Text = $"Line Ending Begin = {b.ToString()}\nLine Ending End={e.ToString()}",
                        InteriorColor = System.Drawing.Color.Red
                    });

                    canvas.SetFont("Arial", 8, 0);
                    canvas.SetBrush(0, 0);
                    canvas.TextOut(x, y - (s / 2), $"{b.ToString()}/{e.ToString()}");

                    x += w + s;
                    i++;
                }
            }
        }

        static void RenderPolygons(int blankWidth, int blankHeight, Canvas canvas, Action<string> printTitle)
        {
            canvas.BlankPage(blankWidth, blankHeight);
            printTitle("Polygon and Polyline");

            // Poly Line
            List<System.Drawing.Point> polylinePoints = new();
            List<System.Drawing.Point> polygonPoints = new();
            int y = 100;
            for (int i = 0; i < 1000 && y < blankHeight - 40; ++i)
            {
                polylinePoints.Add(new System.Drawing.Point(i % 2 == 0 ? 50 : 250, y));
                polygonPoints.Add(new System.Drawing.Point(i % 2 == 0 ? 450 : 650, y));
                y += 50;
            }
            polygonPoints.Add(new System.Drawing.Point(750, y));
            polygonPoints.Add(new System.Drawing.Point(750, 100));

            canvas.Annotate(new Hyland.DocumentFilters.Annotations.PolyLine
            {
                Points = polylinePoints,
                Color = System.Drawing.Color.Red,
                Border = new Hyland.DocumentFilters.Annotations.BorderStyle { Width = 3 },
                LineEndings = new Hyland.DocumentFilters.Annotations.LineEndings
                {
                    Begin = Hyland.DocumentFilters.Annotations.LineEndingType.OpenArrow,
                    End = Hyland.DocumentFilters.Annotations.LineEndingType.Square
                },
                Text = "Polyline"
            });

            // Polygon
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Polygon
            {
                Points = polygonPoints,
                InteriorColor = System.Drawing.Color.Yellow,
                Text = "Polygon"
            });
        }

        void RenderShapesAndStamps(Stream? cdStream, Stream? flowerStream, int blankWidth, int blankHeight, Canvas canvas, Action<string> printTitle)
        {
            canvas.BlankPage(blankWidth, blankHeight);

            printTitle("Shapes and Stamps");

            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Rectangle
            {
                Color = System.Drawing.Color.Purple,
                InteriorColor = System.Drawing.Color.LightCoral,
                Border = new Hyland.DocumentFilters.Annotations.BorderStyle
                {
                    Width = 3
                },
                Rect = new System.Drawing.Rectangle(50, 75, 100, 100),
                Text = "Square"
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Ellipse
            {
                Color = System.Drawing.Color.Purple,
                InteriorColor = System.Drawing.Color.LightCoral,
                Border = new Hyland.DocumentFilters.Annotations.BorderStyle
                {
                    Width = 3
                },
                Rect = new System.Drawing.Rectangle(200, 75, 100, 100),
                Text = "Circle"
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Ellipse
            {
                Color = System.Drawing.Color.Purple,
                InteriorColor = System.Drawing.Color.LightCoral,
                Border = new Hyland.DocumentFilters.Annotations.BorderStyle
                {
                    Width = 3
                },
                Rect = new System.Drawing.Rectangle(350, 75, 300, 100),
                Text = "Circle"
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Stamp
            {
                Subject = "Approved",
                Color = System.Drawing.Color.Green,
                Rect = new System.Drawing.Rectangle(50, 200, 100, 50),
                DefaultAppearance = new Hyland.DocumentFilters.Annotations.DefaultAppearance
                {
                    TextColor = System.Drawing.Color.White
                },
                Text = "I approve this stamp"
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Stamp
            {
                Subject = "Denied",
                Color = System.Drawing.Color.Red,
                Rect = new System.Drawing.Rectangle(200, 200, 100, 50),
                DefaultAppearance = new Hyland.DocumentFilters.Annotations.DefaultAppearance
                {
                    TextColor = System.Drawing.Color.White
                },
                Text = "Request has been rejected"
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Stamp
            {
                Subject = "Icon",
                Rect = new System.Drawing.Rectangle(350, 200, 100, 100),
                Appearance = new Hyland.DocumentFilters.Annotations.AppearanceStreams
                {
                    Normal = new Hyland.DocumentFilters.Annotations.AppearanceStream(cdStream)
                },
                Text = "SVG Stamp"
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Stamp
            {
                Subject = "Image Icon",
                Rect = new System.Drawing.Rectangle(500, 200, 100, 100),
                Appearance = new Hyland.DocumentFilters.Annotations.AppearanceStreams
                {
                    Normal = new Hyland.DocumentFilters.Annotations.AppearanceStream(flowerStream)
                },
                Text = "Image Stamp"
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Stamp
            {
                Subject = "Stretch Icon",
                Rect = new System.Drawing.Rectangle(50, 350, blankWidth - 100, 75),
                Appearance = new Hyland.DocumentFilters.Annotations.AppearanceStreams
                {
                    Normal = new Hyland.DocumentFilters.Annotations.AppearanceStream(flowerStream)
                },
                Text = "Stretched Image Stamp"
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Stamp
            {
                Subject = "Icon",
                Rect = new System.Drawing.Rectangle(50, 450, 64, 64),
                Appearance = new Hyland.DocumentFilters.Annotations.AppearanceStreams
                {
                    Normal = new Hyland.DocumentFilters.Annotations.AppearanceStream(cdStream)
                },
                Text = "SVG Stamp",
                Opacity = 0.5
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Stamp
            {
                Subject = "Image Icon",
                Rect = new System.Drawing.Rectangle(150, 550, 100, 100),
                Appearance = new Hyland.DocumentFilters.Annotations.AppearanceStreams
                {
                    Normal = new Hyland.DocumentFilters.Annotations.AppearanceStream(flowerStream)
                },
                Text = "Image Stamp",
                Opacity = 0.5
            });

            canvas.DrawScaleImage(100, 450, 400, 750, ReadAllBytes(flowerStream), "image/png");

            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Rectangle
            {
                Color = System.Drawing.Color.Black,
                InteriorColor = System.Drawing.Color.Red,
                Border = new Hyland.DocumentFilters.Annotations.BorderStyle
                {
                    Width = 3
                },
                Rect = new System.Drawing.Rectangle(50, 550, 100, 100),
                Text = "Square with 0.5 opacity",
                Opacity = 0.5
            });
        }

        static void RenderText(int blankWidth, int blankHeight, Canvas canvas, Action<string> printTitle, Action<string, int, int> printDescription)
        {
            canvas.BlankPage(blankWidth, blankHeight);

            printTitle("Text");

            int y = 75;
            printDescription("Single Line", 10, y - 15);
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.FreeText
            {
                DefaultAppearance = new Hyland.DocumentFilters.Annotations.DefaultAppearance
                {
                    FontSize = 12
                },
                Text = "Single line, not too long.",
                Rect = new System.Drawing.Rectangle(50, y, blankWidth - 100, 75),
            });
            y += 100;

            printDescription("Multi-line with wrapping", 10, y - 15);
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.FreeText
            {
                DefaultAppearance = new Hyland.DocumentFilters.Annotations.DefaultAppearance
                {
                    FontSize = 12
                },
                Rect = new System.Drawing.Rectangle(50, y, blankWidth - 100, 250),
                Text = "Word Wrap: Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed at neque pharetra, luctus turpis eget, euismod augue. Aliquam at libero iaculis, vestibulum tellus eget, accumsan ligula. Donec eget massa urna. Proin ornare leo mi, non molestie leo auctor vitae. Proin blandit erat massa, eget laoreet eros iaculis ut. Pellentesque vulputate fermentum lorem, id rutrum nibh aliquam eget. Nullam cursus ac sem quis ultrices. In sagittis, turpis vitae aliquet fringilla, nisl tortor bibendum tellus, et pharetra enim tellus ac dui. Donec varius purus ut fringilla laoreet. Nullam auctor sapien eu accumsan gravida. In efficitur convallis enim quis viverra. Nunc maximus mauris sem, a dapibus erat pellentesque vitae. Proin ut cursus libero. Vivamus neque augue, commodo eu vestibulum sed, fermentum quis urna. Donec a neque quis orci bibendum mattis. Nulla nec pellentesque sapien. Nullam vel ante mattis, feugiat quam ut, mattis libero. Nunc pellentesque ut erat eu porttitor. Sed suscipit nec urna vel consectetur. Maecenas ac libero interdum, ullamcorper lectus in, vehicula augue. In vel metus vestibulum, scelerisque ex ut, eleifend diam. Nulla gravida mollis orci, eget condimentum magna posuere non. Donec id est metus. Cras nunc nibh, pulvinar sit amet elementum vel, hendrerit ut sem. Aenean luctus elit vel egestas pretium. Sed non erat quis velit scelerisque volutpat. Nullam venenatis erat a malesuada vehicula. Integer sed neque eu tellus placerat ullamcorper. Donec eleifend metus nec mattis porttitor. Vestibulum mattis magna sit amet urna iaculis bibendum."
            });
            y += 200;

            printDescription("Multi-line with line breaks", 10, y - 15);
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.FreeText
            {
                DefaultAppearance = new Hyland.DocumentFilters.Annotations.DefaultAppearance
                {
                    FontSize = 12
                },
                Text = String.Join("\n", Enumerable.Range(1, 10).Select(e => $"This is line {e}")),
                Rect = new System.Drawing.Rectangle(50, y, blankWidth - 100, 250),
            });
            y += 175;

            printDescription("Right Aligned", 10, y - 15);
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.FreeText
            {
                DefaultAppearance = new Hyland.DocumentFilters.Annotations.DefaultAppearance
                {
                    FontSize = 12
                },
                Alignment = Hyland.DocumentFilters.Annotations.AlignmentType.Right,
                Text = "This line should be right aligned",
                Rect = new System.Drawing.Rectangle(50, y, blankWidth - 100, 50),
            });
            y += 75;

            printDescription("Center Aligned", 10, y - 15);
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.FreeText
            {
                DefaultAppearance = new Hyland.DocumentFilters.Annotations.DefaultAppearance
                {
                    FontSize = 12
                },
                Alignment = Hyland.DocumentFilters.Annotations.AlignmentType.Center,
                Text = "This line should be center aligned",
                Rect = new System.Drawing.Rectangle(50, y, blankWidth - 100, 50),
            });
            y += 50;

            printDescription("Long Text, Right Aligned", 10, y - 15);
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.FreeText
            {
                DefaultAppearance = new Hyland.DocumentFilters.Annotations.DefaultAppearance
                {
                    FontSize = 12
                },
                Alignment = Hyland.DocumentFilters.Annotations.AlignmentType.Right,
                Text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse accumsan rhoncus eros, sit amet egestas ligula dictum at. Nam ut metus a eros pretium convallis. Quisque consequat facilisis metus, non ornare nibh dignissim et. Nulla mauris velit, placerat quis vestibulum sed, dictum vitae turpis. Vestibulum ut mauris quis mi dapibus lacinia a at massa. Maecenas vel eros scelerisque, ultricies lacus et, pellentesque sapien. Cras consectetur sollicitudin dui eget venenatis. Aliquam erat volutpat. Mauris tempor sapien et fringilla ornare.",
                Rect = new System.Drawing.Rectangle(50, y, blankWidth - 100, 70),
            });
            y += 75;
        }

        static void Render2DBarcodes(int blankWidth, int blankHeight, Canvas canvas, Action<string> printTitle)
        {
            canvas.BlankPage(blankWidth, blankHeight);

            printTitle("Barcodes");
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.QrCode
            {
                Caption = "QR Code",
                Content = "http://www.hyland.com",
                Rect = System.Drawing.Rectangle.FromLTRB(50, 50, (blankWidth / 2) - 25, (blankHeight / 2) - 25),
                Margin = 8,
                ErrorCorrectionLevel = 3
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.DataMatrix
            {
                Caption = "Data Matrix",
                Content = "http://www.hyland.com",
                Rect = System.Drawing.Rectangle.FromLTRB((blankWidth / 2) + 25, 50, blankWidth - 50, (blankHeight / 2) - 25),
                Margin = 8,
                ErrorCorrectionLevel = 3
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.PDF417
            {
                Caption = "PDF417",
                Content = "http://www.hyland.com",
                Rect = System.Drawing.Rectangle.FromLTRB(50, (blankHeight / 2) + 25, blankWidth - 50, (int)(blankHeight * 0.75) - 50),
                Margin = 8,
                ErrorCorrectionLevel = 3
            });

        }

        static void Render1DBarcodes(int blankWidth, int blankHeight, Canvas canvas)
        {
            canvas.BlankPage(blankWidth, blankHeight);

            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Code39
            {
                Caption = "Code39: Hello World",
                Content = "HELLO WORLD",
                Rect = System.Drawing.Rectangle.FromLTRB(50, 50, (blankWidth / 2) - 25, 250),
                Margin = 8
            });
            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Code93
            {
                Caption = "Code93: Hello World",
                Content = "HELLO WORLD",
                Rect = System.Drawing.Rectangle.FromLTRB((blankWidth / 2) + 25, 50, blankWidth - 50, 250),
                Margin = 8
            });

            canvas.Annotate(new Hyland.DocumentFilters.Annotations.Code128
            {
                Caption = "Code128: Hello World",
                Content = "HELLO WORLD",
                Rect = System.Drawing.Rectangle.FromLTRB(50, 300, (blankWidth / 2) - 25, 500),
                Margin = 8
            });

            canvas.Annotate(new Hyland.DocumentFilters.Annotations.GS1_128
            {
                Parts =
                        {
                            { "11", "200101"
                            },
                            { "17", "250101"
                            },
                            { "241", "987654321"
                            },
                        },
                AutoCaption = true,
                Rect = System.Drawing.Rectangle.FromLTRB((blankWidth / 2) + 25, 300, blankWidth - 50, 500),
                Margin = 8
            });
        }
    }

    private Stream? GetTestResource(string fileName)
        => Assembly.GetExecutingAssembly().GetManifestResourceStream($"MarkupAnnotationsDemo.Resources.{fileName}");

    private byte[] ReadAllBytes(Stream? stream)
    {
        if (stream == null)
            return new byte[] { };
        else
        {
            stream.Position = 0;
            byte[] res = new byte[stream.Length];
            _ = stream.Read(res, 0, res.Length);
            return res;
        }
    }

    /// <summary>
    /// Utility function that maps a file extension to a Canvas type.
    /// </summary>
    private CanvasType ExtToCanvasType(string? ext)
    {
        CanvasType res;
        if (!Enum.TryParse<CanvasType>(ext?.Trim('.'), true, out res))
            throw new ArgumentException($"Unknown format {ext}");
        return res;
    }

    static int Main(string[] args) => CommandLineApplication.Execute<Program>(args);
}

