'(c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
'
'THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
'ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
'WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
'DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
'ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
'(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
'LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
'ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
'(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
'SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
'

'****************************************************************************
'* Perceptive Document Filters Example - Convert a document to Interactive HD HTML
'*
'* This example creates a html view of a document which can be manipulated by
'* javascript to provide more functionality than a standard static page.
'*
'* There are two parts to the sample, the page generate logic use standard
'* Document Filters HDHTML exports to create the page data. The sample then
'* injects CSS and Javascript to provide extra viewing capabilities, such as
'* Zoom, Rotate, Highlight, Redact and Metadata Preview.
'*
'****************************************************************************

Imports System
Imports System.Collections.Generic
Imports System.Text
Imports System.IO
Imports Hyland.DocumentFilters

Namespace DocumentFiltersSamples

    Class Program
        Private m_filters As DocumentFilters
        Private m_outputFolder As String
        Private m_stdout As TextWriter
        Private m_stderr As TextWriter
        Private m_async As Boolean
        Private m_inlineImages As Boolean
        Private m_options As String

        Public Sub New()
            m_stdout = System.Console.Out
            m_stderr = System.Console.[Error]
            m_outputFolder = "."
            m_async = False
            m_inlineImages = False
        End Sub

        Public Sub Run(args As String())
            If args.Length = 0 Then
                ShowHelp()
                Return
            End If

            m_filters = New DocumentFilters()
            m_filters.Initialize("{ENTER LICENSE KEY}", ".")

            Dim fileList As New List(Of String)()
            For i As Integer = 0 To args.Length - 1
                Dim arg As [String] = args(i)

                If [String].Compare(arg, "--output", True) = 0 OrElse [String].Compare(arg, "-o", True) = 0 Then
                    m_outputFolder = args(System.Threading.Interlocked.Increment(i))
                ElseIf (String.Compare(arg, "--async", True) = 0 Or String.Compare(arg, "-a", True) = 0) Then
                    m_async = True
                ElseIf (String.Compare(arg, "--inline-images", True) = 0) Then
                    m_inlineImages = True
                ElseIf (String.Compare(arg, "--options", True) = 0) Then
                    m_options = args(System.Threading.Interlocked.Increment(i))
                ElseIf [String].Compare(arg, "-h", True) = 0 OrElse [String].Compare(arg, "--help", True) = 0 Then
                    ShowHelp()
                    Return
                Else
                    fileList.Add(arg)
                End If
            Next

            For Each filename As String In fileList
                ProcessFile(filename, m_filters.GetExtractor(filename))
            Next

            m_stdout.Close()
        End Sub

        Private Sub ProcessFile(filename As String, item As Extractor)
            Dim destination As String = System.IO.Path.Combine(m_outputFolder, System.IO.Path.GetFileNameWithoutExtension(filename) + ".html")

            m_stderr.WriteLine("Processing " + filename)
            Try
                Dim OptionsMain As String
                Dim OptionsPage As String

                OptionsMain = OptionsPage = m_options & ";" &
                 "HDHTML_INCLUDE_WORD_INDEXES=on;"
                ' HDHTML_INCLUDE_WORD_INDEXES will include information about each word in the run data for a page. This is 
                ' used by the javascript to map between the web browsers co-ordinate system and the word co-ordinate system
                ' used by Document Filters. In this example, it's used by the redaction function.

                If m_inlineImages Then
                    OptionsMain += "HTML_INLINE_IMAGES=on;"
                    OptionsPage += "HTML_INLINE_IMAGES=on;"
                    ' HTML_INLINE_IMAGES forces any images to be inlined directly into the HTML using url(data:image/png;base64,
                    ' rather than having external files. 
                End If


                OptionsMain += "HDHTML_OUTPUT_INJECT_HEAD=@perceptive-viewer-inject.html;"
                ' HDHTML_OUTPUT_INJECT_HEAD will load the contents of the perceptive-viewer-inject.html file and place it
                ' at the bottom of the <HEAD> section of the main HTML page. This allows us to inject stylesheets, javascript
                ' and extra metadata that will be loaded when the page is viewed.

                OptionsPage += "HDHTML_OUTPUT_BOILERPLATE=off"
                ' HDHTML_OUTPUT_BOILERPLATE disables the surrounding <html>...<body> tags that typically get generated into
                ' the HTML output.  This is used when generateing ASYNC pages, which must only contain the actual page data
                ' and not the surrounding html.

                item.Open(isys_docfilters.IGR_BODY_AND_META Or isys_docfilters.IGR_FORMAT_IMAGE)

                Dim outputStream As System.IO.FileStream = New System.IO.FileStream(destination, FileMode.Create, FileAccess.ReadWrite)
                Dim canvas As Hyland.DocumentFilters.Canvas = m_filters.MakeOutputCanvas(outputStream, isys_docfilters.IGR_DEVICE_HTML, OptionsMain)

                Try
                    For pageIndex As Integer = 0 To item.GetPageCount() - 1
                        Dim page As Hyland.DocumentFilters.Page = item.GetPage(pageIndex)
                        Try
                            m_stderr.Write(" - Page " & (pageIndex + 1) & " => ")

                            ' When in Async mode, always render the first page directly into the main output
                            If Not m_async Or pageIndex = 0 Then
                                m_stderr.WriteLine(destination)
                                canvas.RenderPage(page)
                            Else
                                Dim pageFilename As String = System.IO.Path.ChangeExtension(destination, Nothing) + String.Format("_page_{0}.html", pageIndex + 1)

                                m_stderr.WriteLine(pageFilename)

                                ' Render page stubs instead of the actual page content
                                Dim w As Integer = page.Width
                                Dim h As Integer = page.Height


                                Dim html As String = "<div class=""idf-page"" style=""width: " & w & "px; height: " & h & "px; border: 1px solid black; overflow:hidden;"" title=""" & System.IO.Path.GetFileName(pageFilename) & """></div>"

                                Dim chars As Byte() = UTF8Encoding.Default.GetBytes(html)
                                outputStream.Write(chars, 0, chars.Length)

                                ' Render page to it's own file
                                Dim pageCanvas As Hyland.DocumentFilters.Canvas = m_filters.MakeOutputCanvas(pageFilename, isys_docfilters.IGR_DEVICE_HTML, OptionsPage)
                                pageCanvas.RenderPage(page)
                                pageCanvas.Close()
                            End If

                            If Not m_inlineImages Then
                                ' Embedded Images
                                Dim image As SubFile = page.GetFirstImage()
                                While image IsNot Nothing
                                    image.CopyTo(System.IO.Path.Combine(m_outputFolder, image.getName()))
                                    image = page.GetNextImage()
                                End While
                            End If
                        Finally
                            page.Close()
                        End Try
                    Next
                Finally
                    canvas.Close()
                End Try
                Dim outDir As String = System.IO.Path.GetDirectoryName(destination)
                If Not System.IO.File.Exists(outDir + "\\perceptive-viewer-utils.js") Then
                    System.IO.File.Copy("perceptive-viewer-utils.js", outDir + "\perceptive-viewer-utils.js", False)
                End If
                If Not System.IO.File.Exists(outDir + "\\perceptive-viewer-utils.css") Then
                    System.IO.File.Copy("perceptive-viewer-utils.css", outDir + "\perceptive-viewer-utils.css", False)
                End If

            Catch e As Exception
                m_stderr.WriteLine("Error Processing " + filename)
                m_stderr.WriteLine("   - " + e.ToString())
            Finally
                item.Close()
            End Try
        End Sub

        Private Sub ShowHelp()
            System.Console.WriteLine("Document Filters 11: ConvertDocumentToInteractiveHDHTML vb.net Example")
            System.Console.WriteLine("(c) 2019 Hyland Software, Inc.")
            System.Console.WriteLine("")
            System.Console.WriteLine("ConvertDocumentToInteractiveHDHTML [options] filename")
            System.Console.WriteLine("")
            System.Console.WriteLine("options")
            System.Console.WriteLine(" -h, --help                this help")
            System.Console.WriteLine("     --inline-images       indicates that image data should be stored directly into the generated html")
            System.Console.WriteLine(" -o, --output [folder]     the folder to save the output files, defaults to current directory")
            System.Console.WriteLine(" -a, --async               generate HTML that loads page data on demand. The generated output ")
            System.Console.WriteLine("                           MUST be loaded into a web server.")
            System.Console.WriteLine("")
            System.Console.WriteLine("                           The page will fail to load using the file:// protocol")

        End Sub

        Public Shared Sub Main(args As String())
            Dim prog As New Program()
            prog.Run(args)
        End Sub
    End Class

End Namespace