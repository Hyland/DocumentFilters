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

Imports System.IO
Imports Hyland.DocumentFilters

Module ConvertDocument
    Private m_filters As DocumentFilters
    Private m_html As Boolean
    Private m_subFiles As Boolean
    Private m_outputFile As String = ""
    Private m_stdout As System.IO.TextWriter
    Private m_stderr As System.IO.TextWriter

    Sub Main()
        m_stdout = System.Console.Out
        m_stderr = System.Console.Error

        Dim Args() As String = System.Environment.GetCommandLineArgs()

        If Args.Length = 0 Then
            ShowHelp()
            Exit Sub
        End If

        m_filters = New DocumentFilters
        m_filters.Initialize(DfSamplesCommon.DocumentFiltersLicense.LICENSE_KEY, ".")

        Dim fileList As List(Of String) = New List(Of String)
        For I As Integer = 1 To Args.Length - 1
            Dim arg As String = Args(I)

            If (String.Compare(arg, "--html", True) = 0) Then
                m_html = True
            ElseIf (String.Compare(arg, "--text", True) = 0 Or String.Compare(arg, "-t", True) = 0) Then
                m_html = False
            ElseIf (String.Compare(arg, "--subfiles", True) = 0 Or String.Compare(arg, "-s", True) = 0) Then
                m_subFiles = True
            ElseIf (String.Compare(arg, "--output", True) = 0 Or String.Compare(arg, "-o", True) = 0) Then
                I = I + 1
                m_outputFile = Args(I)
            ElseIf (String.Compare(arg, "-h", True) = 0 Or String.Compare(arg, "--help", True) = 0) Then
                ShowHelp()
                Exit Sub
            Else
                fileList.Add(arg)
            End If
        Next

        If m_outputFile <> "" Then
            m_stdout = New StreamWriter(File.Open(m_outputFile, FileMode.Create), System.Text.Encoding.UTF8)
        End If

        For Each filename As String In fileList
            ProcessFile(filename, m_filters.GetExtractor(filename))
        Next

        m_stdout.Close()
    End Sub

    Private Sub ProcessFile(ByVal filename As String, ByVal item As Extractor)
        m_stderr.WriteLine("Processing " + filename)
        Try
            Dim flags As Integer = isys_docfilters.IGR_BODY_AND_META
            If m_html Then
                flags = flags Or isys_docfilters.IGR_FORMAT_HTML
            End If

            item.Open(flags)

            ' Extract the text and return it to stdout
            While Not item.getEOF
                Dim Text As String = item.GetText(4096)
                ' Cleanup the text
                Text = Text.Replace(Chr(&HE), vbLf)
                Text = Text.Replace(vbCr, vbLf)
                m_stdout.Write(Text)
            End While
            m_stdout.WriteLine("")

            ' Extract the HTML generated images into the current folder
            If m_html Then
                Dim image As SubFile = item.GetFirstImage()
                While image IsNot Nothing
                    m_stderr.WriteLine("Extracting image " + image.getName())
                    image.CopyTo(image.getName())
                    image.Close()
                    ' Move onto the next image
                    image = item.GetNextImage()
                End While
            End If

            ' Extract the sub files (if any) and process recursively
            If m_subFiles Then
                Dim child As SubFile = item.GetFirstSubFile
                While child IsNot Nothing
                    ProcessFile(filename + ">" + child.getName(), child)
                    ' Move onto the next sub file
                    child = item.GetNextSubFile()
                End While
            End If

        Catch e As Exception
            m_stderr.WriteLine("Error Processing " + filename)
            m_stderr.WriteLine("   - " + e.ToString())
        Finally
            item.Close()
        End Try
    End Sub

    Private Sub ShowHelp()
        System.Console.WriteLine("Document Filters 11: ConvertDocument vb.net Example")
        System.Console.WriteLine("(c) 2022 Hyland Software, Inc.")
        System.Console.WriteLine("")
        System.Console.WriteLine("ConvertDocument [options] filename")
        System.Console.WriteLine("")
        System.Console.WriteLine("options")
        System.Console.WriteLine(" -h, --help                this help")
        System.Console.WriteLine(" -t, --text                output as plain text")
        System.Console.WriteLine("     --html                output as HTML text, images stored in current directory")
        System.Console.WriteLine(" -o, --output [filename]   write the output to the specified file")
        System.Console.WriteLine(" -s, --subfiles            process subfiles")
    End Sub

End Module
