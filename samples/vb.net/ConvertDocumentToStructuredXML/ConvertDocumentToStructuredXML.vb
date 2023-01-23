'(c) 2022 Hyland Software, Inc. and its affiliates. All rights reserved.
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

        Public Sub New()
            m_stdout = System.Console.Out
            m_stderr = System.Console.Error
            m_outputFolder = "."
        End Sub

        Public Sub Run(args As String())
            If args.Length = 0 Then
                ShowHelp()
                Return
            End If

            m_filters = New DocumentFilters()
            m_filters.Initialize(DfSamplesCommon.DocumentFiltersLicense.LICENSE_KEY, ".")

            Dim fileList As New List(Of String)()
            For i As Integer = 0 To args.Length - 1
                Dim arg As String = args(i)

                If String.Compare(arg, "--output", True) = 0 OrElse String.Compare(arg, "-o", True) = 0 Then
                    m_outputFolder = args(System.Threading.Interlocked.Increment(i))
                ElseIf String.Compare(arg, "-h", True) = 0 OrElse String.Compare(arg, "--help", True) = 0 Then
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
            Dim destination As String = System.IO.Path.Combine(m_outputFolder, System.IO.Path.GetFileNameWithoutExtension(filename) + ".xml")

            m_stderr.WriteLine("Processing " + filename)
            Try
                item.Open(isys_docfilters.IGR_BODY_AND_META Or isys_docfilters.IGR_FORMAT_IMAGE)

                Dim canvas As Hyland.DocumentFilters.Canvas = m_filters.MakeOutputCanvas(destination, isys_docfilters.IGR_DEVICE_XML, "")
                Try
                    For pageIndex As Integer = 0 To item.GetPageCount()-1
                        Dim page As Hyland.DocumentFilters.Page = item.GetPage(pageIndex)
                        Try
                            canvas.RenderPage(page)
                        Finally
                            page.Close()
                        End Try
                    Next
                Finally
                    canvas.Close()
                End Try
            Catch e As Exception
                m_stderr.WriteLine("Error Processing " + filename)
                m_stderr.WriteLine("   - " + e.ToString())
            Finally
                item.Close()
            End Try
        End Sub

        Private Sub ShowHelp()
            System.Console.WriteLine("Document Filters 11: ConvertDocumentToStructuredXML vb.net Example")
            System.Console.WriteLine("(c) 2022 Hyland Software, Inc.")
            System.Console.WriteLine("")
            System.Console.WriteLine("ConvertDocumentToStructuredXML [options] filename")
            System.Console.WriteLine("")
            System.Console.WriteLine("options")
            System.Console.WriteLine(" -h, --help                this help")
            System.Console.WriteLine(" -o, --output [folder]     the folder to save the output files, defaults to current directory")
        End Sub

        Shared Sub Main(args As String())
            Dim prog As New Program()
            prog.Run(args)
        End Sub
    End Class

End Namespace
