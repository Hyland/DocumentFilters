'(c) 2022 Hyland Software, Inc. and its affiliates. All rights reserved.

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
Imports System.IO
Imports Hyland.DocumentFilters

Namespace DocumentFiltersSamples

    Public Class CustomStream
        Inherits IGRStream

        Private m_stream As System.IO.Stream

        Public Sub New(stream As System.IO.Stream)
            m_stream = stream
        End Sub

        Public Overrides Function Read(Size As UInteger, Dest As IGRStream_Data) As UInteger
            Dim data(Size) As Byte
            Dim res As Integer = m_stream.Read(data, 0, Size)
            Dest.write(data, res)
            Return res
        End Function

        Public Overrides Function Seek(Offset As Long, Origin As Integer) As UInteger
            Return m_stream.Seek(Offset, Origin)
        End Function

        Public Overrides Function Write(bytes As Byte(), size As UInteger) As UInteger
            m_stream.Write(bytes, 0, size)
            Return size
        End Function

        Public Function writeTo(outputStream As BinaryWriter) As Long
            Dim buffer(4096) As Byte
            Dim bytesRead As Integer
            Dim totalBytesWritten As Long
            m_stream.Position = 0

            Do
                bytesRead = m_stream.Read(buffer, 0, buffer.Length)
                If (bytesRead > 0) Then
                    outputStream.Write(buffer, 0, bytesRead)
                    totalBytesWritten += bytesRead
                End If
            Loop While bytesRead > 0
            Return totalBytesWritten
        End Function
    End Class

End Namespace
