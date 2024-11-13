/*
(c) 2023 Hyland Software, Inc. and its affiliates. All rights reserved.

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

/****************************************************************************
* Document Filters 11 C++ objects & helper functions
****************************************************************************/

#ifndef PERCEPTIVE_DOCFILTERSOBJECTS_H_DEF
#define PERCEPTIVE_DOCFILTERSOBJECTS_H_DEF

#include "DocumentFilters.h"

#include <map>
#include <string>
#include <vector>
#include <exception>
#include <vector>
#include <memory>
#include <ostream>
#include <stdint.h>

#define _UCS2(S) (WCHAR *)Perceptive::ToWCHARStr(S).c_str()

#define PS_CLASS_NO_COPY(_class)		\
	private: _class(const _class& ref) {} \
	private: _class& operator=(const _class& ref) { return *this; }

namespace Perceptive
{
	typedef std::map<std::string,std::string> Metadata;
	typedef std::map<std::wstring,std::wstring> MetadataW;

	class DocumentFilters;
	class SubFile;
	class Page;
	class Word;
	class Stream;


	/** \brief
		The Extractor interface allows you to extract the content of a document and/or enumerate its sub-documents, 
		such as email attachments and ZIP archives.

		To obtain this interface, call the DocumentFilters.GetExtractor method. The Extractor interface contains the 
		following methods and properties.
	*/
	class Extractor
	{
		PS_CLASS_NO_COPY(Extractor)

		friend class DocumentFilters;
		friend class SubFile;
	private:
		Extractor(Stream *stream, bool own_stream);
	public:
		enum FileTypeWhat { kLongName = 0, kShortName = 1, kConfigName = 2, kClass = 3, kLegacy = 4 };

		virtual ~Extractor();

		/** \brief
			The Open method opens a document for processing.

			\param flags specify the type content required. 
			\param options contains a semicolon delimitered set of NAME=VALUE 
		*/
		void Open(const int flags = IGR_BODY_AND_META, const std::string &options = "");

		/** \brief 
			The FileType property is the document format code, as listed in Document Format Codes chart in the 
			implementation guide. 
		*/
		int getFileType();

		/** \brief
			Returns the fileype, as a string.
		*/
		std::string getFileType(FileTypeWhat what);

		/** \brief
			Returns the Document Capabilities as a bitfield, which can contain zero or more of the following:
			- IGR_FILE_SUPPORTS_TEXT          1
			- IGR_FILE_SUPPORTS_SUBFILES      2
			- IGR_FILE_SUPPORTS_HDHTML        4
		*/
		LONG getCapabilities();

		/** \brief
			SupportsText property is TRUE if text content can be extracted from the document. This property must be 
			TRUE to be able to call to the Extractor::SaveTo and Extractor::GetText methods.
		*/
		bool getSupportsText();

		/** \brief
			The SupportsSubFiles property is TRUE if the document is a compound or archive document, potentially with 
			sub-documents.
			
			This property must be TRUE to call the GetFirstSubFile and GetNextSubFile methods.
		*/
		bool getSupportsSubFiles();

		/** \brief
			The SupportsHD property is TRUE if the document can be converted to a high-definition rendition.
		*/
		bool getSupportsHD();

		/** \brief
			The EOF property is only valid for documents where the SupportsText property is TRUE. The EOF property will be 
			set to TRUE when no more text can be extracted from the document with calls to GetText. If the document needs 
			to be re-read, call Close and Open first.
		*/
		bool getEOF();

		/** \brief
			The GetText method extracts the next portion of text content from the document as UTF-8 encoded string.
		*/
		std::string GetText(const int max_length = 4096);

		/** \brief
		The GetText method extracts the next portion of text content from the document as a std::wstring.
		*/
		std::wstring GetTextW(const int max_length = 4096);


		std::vector<WCHAR> GetTextUTF16(const int max_length = 4096);

		/** \brief
			The GetFirstSubFile and GetFirstImage methods obtains a SubFile object representing the first sub-document or 
			attached image (if using HTML conversion) of the current document.
		*/
		SubFile *GetFirstSubFile();
		SubFile *GetNextSubFile();
		SubFile *GetFirstImage();
		SubFile *GetNextImage();

		/** \brief
			The GetSubFile method obtains a SubFile object representing the nominated sub-file of the current document.
		*/
		SubFile *GetSubFile(const std::string &ID);

		/** \brief
			The CopyTo method extracts the binary content of the sub-document to a file.
		*/
		void CopyTo(const std::string &filename);

		/** \brief
			The SaveTo method extracts the entire text content of the document in a single call. The text may be saved to a 
			file with the given name.
		*/
		void SaveTo(const std::string &filename);

		/** \brief
			The getHashMD5 and getHashSHA1 methods obtain a string representing the calculated hash of the current document 
			for unique identification.
		*/
		std::string getHashMD5();
		std::string getHashSHA1();

		/** \brief
			The Close method releases the document resources referenced by this Extractor object.
		*/
		void Close();

		/** \brief
			The GetFirstPage/GetNextPage methods enumerate over the pages of an opened document. 
		*/
		Page *GetFirstPage();
		Page *GetNextPage();

		/** \brief
			The GetPage method returns the page at the given index, where the page index is 0-based. An exception is raised if 
			the index is invalid.
		*/
		Page *GetPage(const int PageIndex);

		/** \brief
			Returns the number of pages in the current document, the document must be opened in image mode for the page count to 
			be populated.
		*/
		int getPageCount();

		/** \brief
			Returns the underlying document handle.
		*/
		LONG getHandle() const;
		
		void DumpStatistics(std::ostream &Stream);
		static std::string PrepareText(const std::string &text);
		static std::wstring PrepareTextW(const std::wstring &text);
		static std::vector<WCHAR> PrepareText(const std::vector<WCHAR> &text);
		Metadata ParseMetadata(const std::string &S);
		MetadataW ParseMetadataW(const std::wstring &S);


		typedef Stream* (*COnGetSubStream)(Extractor* extractor, IGR_Stream* stream, bool image);
	protected:
		class Impl;
		Impl* m_impl;

		virtual LONG needHandle();
		virtual IGR_Stream* needStream();
		void setStream(Stream* stream, bool own_stream);
	};

	/** \brief
		The SubFile interface is a descendant of Extractor, allowing work with sub-documents, extracted from a parent document, 
		by calling the parent�s Extractor::GetFirstSubFile and Extractor::GetNextSubFile methods.

		Open the sub-document associated with an instance of SubFile, in the same way as described for Extractor, allowing 
		processing of sub-documents to any depth. This means that text can be extracted and/or sub-documents contained in this 
		SubFile maybe enumerated.
	*/
	class SubFile
		: public Extractor
	{
		friend class Extractor;
		friend class Page;

		typedef LONG (IGR_EXPORT *CMethod)(LONG handle, LPCWSTR id, PIGR_Stream* stream, Error_Control_Block* ecb);

		SubFile(LONG parent_handle, const std::string &id, const std::string &name, LONGLONG file_time, LONGLONG file_size, CMethod method);
	public:
		virtual ~SubFile();

		/** \brief 
			The ID property contains the unique ID of the sub-document.
		*/
		const std::string& getID() const;

		/** \brief
			The Name property displays name of the sub-document, if available.
		*/
		const std::string& getName() const;

		/** \brief
			The FileDate property contains last-modified date and time of the sub-document as a double-precision number (DATE).
			If the date information is not available, the value is 0.
		*/
		LONGLONG getFileDate() const;

		/** \brief
			The FileSize property contains the size, in bytes, of the sub-document as a 64-bit number (INT64).
			If the size information is not available, the value is 0.
		*/
		LONGLONG getFileSize() const;
	private:
		class Impl;
		Impl* m_impl;

		SubFile(const SubFile&);			// no-op 
		SubFile& operator=(const SubFile&); // no-op
	protected:
		IGR_Stream* needStream();
	};

	/** \brief
		The Page interface represents a single page in an image laid-out document. The page allows access to the words on a page, 
		as well as the ability to render it onto a canvas such as TIFF, PNG, or PDF.

		To obtain this interface, call the Extractor::GetPage. The Page interface contains the following methods and properties.
	*/
	class Page
	{
		PS_CLASS_NO_COPY(Page)

		friend class Extractor;
		friend class Canvas;
		Page(HPAGE P);
	public:
		virtual ~Page();

		/** \brief
			The Close method releases any resources associated with the page. 
		*/
		void Close();

		/** \brief
			The WordCount property returns the number of "Word"s that are on a page. The words can be enumerated using the GetFirstWord 
			and GetNextWord methods. 
		*/
		int getWordCount();

		/** \brief
			Returns the width of the page in pixels.
		*/
		int getWidth();

		/** \brief
			Returns the height of the page in pixels.
		*/
		int getHeight();

		/** \brief
			The Text property returns all the text contained on the page as a UTF-8 encoded string
		*/
		std::string GetText(const int MaxLength = 4096);

		/** \brief
			The Text property returns all the text contained on the page as a std::wstring
		*/
		std::wstring GetTextW(const int MaxLength = 4096);

		/** \brief
			IGR_Get_Page_Attribute returns style or properties of an open page; see <page> under Structured XML for a full list of options.
		*/
		std::string GetAttribute(const std::string &Name);

		/** \brief
			The FirstWord and NextWord properties enumerate all the words on the current page. FirstWord resets the enumeration back to 
			the beginning. The property will return a reference to a Word object, or NULL if there are no more words. 

			It is the callers responsibility to free the returned Word object.
		*/
		Word *GetFirstWord();
		Word *GetNextWord();
		Word *GetWord(const int WordIndex);

		/** \brief 
			The FirstImage and NextImage enumerate the embedded images that are on the page. This method is useful if the page images are to 
			be extracted and stored in separate files. These properties are not needed if the page is to be rendered into an image output 
			canvas such as PNG, TIFF, or PDF.
		*/
		SubFile *GetFirstImage();
		SubFile *GetNextImage();

		/** \brief
			Redact removes a range of words and blacks out the location for the specified range from the page. 
		*/
		void Redact(int FirstWord, int LastWord);
		void Redact(Word *FirstWord, Word *LastWord);

		HPAGE getHandle() const;
	private:
		class Impl;
		Impl* m_impl;


		void getDimensions(LONG &Width, LONG &Height);
	};

	/** \brief
		The Word interface allows extraction of words and their bounding boxes when in image / HD mode.
		
		To obtain this interface, call the Page.GetFirstWord() / Page.GetNextWord methods. 
	*/
	class Word
	{
		PS_CLASS_NO_COPY(Word);

		friend class Page;
		Word(IGR_Page_Word *W, int WordIndex);
	public:
		virtual ~Word();

		/** \brief
			Returns the text of this word as a UTF-8 encoded string.
		*/
		std::string GetText() const;

		/** \brief
			Returns the text of this word as a std::wstring.
		*/
		std::wstring GetTextW() const;

		/** \brief
			The X and Y properties return the position of the word in pixels. The position information is based on the DPI 
			used when loading the page.
		*/
		int getX() const;
		int getY() const;

		/** \brief
			The Width and Height properties return the dimensions of the word in pixels. The dimension information is based 
			on the DPI used when loading the page. 
		*/
		int getWidth() const;
		int getHeight() const;

		int getCharacterOffset() const;
		int getWordIndex() const;

		int getStyle() const;
	private:
		IGR_Page_Word *m_page_word;
		int m_word_index;
	};

	/** \brief
		The Canvas interface allows rendering of pages to a variety of output devices, including HD HTML, PNG, and PDF.
		
		The Canvas object also allows post-processing / image manipulation of output such as annotations, redaction, 
		bates stamping, or general drawing.
		
		To obtain this interface, call the DocumentFilters.MakeOutputCanvas or DocumentFilters.MakeOutputCanvasOnStream 
		methods. The Canvas interface contains the following methods and properties:
		
		Note: The drawing API is available for bitmap and PDF outputs only. Drawing onto an HTML5 output is not supported.
	*/
	class Canvas
	{
		PS_CLASS_NO_COPY(Canvas)

		friend class DocumentFilters;
		Canvas(HCANVAS C);
	public:
		virtual ~Canvas();

		/** \brief
			The Close method releases any resources associated with the canvas and flushes any pending data to the output 
			device. 
		*/
		void Close();

		/** \brief
			RenderPage draws the page content onto the specified output canvas. 
		*/
		void RenderPage(Page* page);

		/** \brief
			The Arc method draws an arc on the image along the perimeter of the ellipse, bounded by the specified 
			rectangle. It uses the current Pen.
		*/
		void Arc(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4);
		void AngleArc(int x, int y, int Radius, int StartAngle, int SweepAngle);

		/** \brief
			The Chord method draws a closed figure represented by the intersection of a line and an ellipse. The ellipse is 
			bisected by a line that runs between X3,Y3 and X4,Y4 coordinates
		*/
		void Chord(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4);

		/** \brief
			The Ellipse method draws the ellipse defined by a bounding rectangle on the canvas, outlined with the current 
			pen and filled with the current brush.
		*/
		void Ellipse(int x, int y, int x2, int y2);

		/** \brief
			The Rect method draws a rectangle using the Brush and Pen of the canvas to fill and draw the border.
		*/
		void Rect(int x, int y, int x2, int y2);

		/** \brief
			LineTo draws a line on the canvas from current pen position to the point specified by X and Y, and sets the pen 
			position to (X, Y) coordinates.
		*/
		void LineTo(int x, int y);
		
		/** \brief
			MoveTo changes the current drawing position to the point (X,Y).
		*/
		void MoveTo(int x, int y);

		/** \brief
			The Pie method draws a pie-shaped section of the ellipse on the canvas, bounded by the rectangle (X, Y) and 
			(X2, Y2).
		*/
		void Pie(int x, int y, int x2, int y2, int x3, int y3, int x4, int y4);

		/** \brief
			RoundRect draws a rectangle with rounded corners on the canvas, outlined with the current pen and filled with 
			the current brush.
		*/
		void RoundRect(int x, int y, int x2, int y2, int Radius);

		/** \brief
			TextOut writes a string on the canvas, starting at (X, Y). It updates the pen position to the end of the string 
			and uses the current font and brush.
		*/
		void TextOut(int x, int y, const std::string& text);
		void TextOut(int x, int y, const std::wstring& text);

		/** \brief 
			Writes a string inside a clipping rectangle, using the current brush and font. 
		*/
		void TextRect(int x, int y, int x2, int y2, const std::string& text, int Flags);
		void TextRect(int x, int y, int x2, int y2, const std::wstring& text, int Flags);
		
		/** \brief
			Returns the width and height in pixels, of a string if rendered with the current font. 
		*/
		int TextWidth(const std::string& text);
		int TextHeight(const std::string& text);
		int TextWidth(const std::wstring& text);
		int TextHeight(const std::wstring& text);

		/** \brief
			SetPen updates the canvas pen with the specific color, width, and style.
		*/
		void SetPen (int Color, int Width, int Style);

		/** \brief
			SetBrush updates the current brush on the canvas with the given color and style. Brushes are used when drawing 
			rectangles and text.
		*/
		void SetBrush(int Color, int Style);

		/** \brief
			SetFont specifies the font to be used when drawing text to the canvas. All subsequent calls to TextOut and 
			MeasureText will use this font.
		*/
		void SetFont(const std::string& Name, int Size, int Style);

		/** \brief
			SetOpacity sets the opacity or transparency for future drawing routines.
		*/
		void SetOpacity(BYTE Opacity);

		/** \brief
			DrawImage renders an image from a buffer onto the Canvas.
		*/
		void DrawImage(int x, int y, void *ImageData, size_t ImageSize, const std::string &MimeType);

		/** \brief 
			DrawScaleImage renders an image from a buffer onto the Canvas. The image is scaled to a specified size.
		*/
		void DrawScaleImage(int x, int y, int x2, int y2, void *ImageData, size_t ImageSize, const std::string &MimeType);
		void Rotation(int Degrees);

		/** \brief
			Reset the canvas back to default values
		*/
		void Reset();

		/** \brief
			Returns the underlying canvas handle
		*/
		HCANVAS getHandle() const;
	private:
		HCANVAS m_canvas_handle;
	};

	/** \brief
		DocumentFilters is the primary factory object in the Document Filters Object Library. You will need to 
		create and initialize an instance of this object to start using the API. It is recommended to define the object in 
		the application scope and create and initialize it only once.
	*/
	class DocumentFilters
	{
		PS_CLASS_NO_COPY(DocumentFilters)

	public:
		DocumentFilters();
		virtual ~DocumentFilters();

		/** \brief
			The Initialize method initializes and authorizes the Document Filters API. It is the first method that 
			your application must call.
		*/
		void Initialize(const std::string &License, const std::string &Path);

		/** \brief
			Unloaded Document Filters and releases any allocated resources. Note, you must release all extractors and subfiles
			before calling Uninitialze.
		*/
		void Uninitialize();

		/** \brief
			Creates a new canvas that is used for rendering page content. The output data will be written to the file specified 
			in filename. 

			\param filename path to output file to create
			\param type enum containing the canvas type, i.e. IGR_DEVICE_IMAGE_PNG, IGR_DEVICE_IMAGE_JPG, IGR_DEVICE_IMAGE_PDF...
			\param options string containing name=value semicolon separated list of options
			\return a new canvas object, it is the callers responsibility to free
		*/
		Canvas* MakeOutputCanvas(const std::string& filename, int type, const std::string& options);

		/** \brief
			Creates a new canvas that is used for rendering page content, writing to the stream object.
			
			\param stream where content is to be written
			\param type enum containing the canvas type, i.e. IGR_DEVICE_IMAGE_PNG, IGR_DEVICE_IMAGE_JPG, IGR_DEVICE_IMAGE_PDF...
			\param options string containing name=value semicolon separated list of options
			\return a new canvas object, it is the callers responsibility to free
		*/
		Canvas* MakeOutputCanvas(Stream *stream, int type, const std::string& Options);

		/** \brief
			The GetExtractor method obtains an Extractor object to process a document. An Extractor allows you to:
			-	Identify a document's type.
			-	Extract its text.
			-	Extract its metadata.
			-	Enumerate and extract sub-documents.
			-	Convert a document to HTML.
			-	Convert a document to an image or series of images.

			\param filename contains path to the document to be opened.
			\return a new extractor object, it is the callers responsibility to free
		*/
		Extractor *GetExtractor(const std::string &filename);

		/** \brief
			The GetExtractor method obtains an Extractor object to process a document.
				
			\param stream of document to be opened
			\return a new extractor object, it is the callers responsibility to free
		*/
		Extractor *GetExtractor(Stream *stream, bool own_stream = true);
		Extractor *GetExtractor(IGR_Stream *stream);

		/** \brief
			Sets the default encoding to use for documents that are not unicode, or do not store a code page.
		*/
		void SetDefaultInputEncoding(const std::string &encoding);

		/** \brief
		Sets the default encoding to use for documents that are not unicode, or do not store a code page.
		*/
		void SetDefaultInputCodePage(const int code_page);

		/** \brief 
			Sets a directory where temporary files way be written.
		*/
		void SetTempPath(const std::string &path);


		/** \brief
			Returns the current memory state of the process. Windows only. 
		*/
		std::string GetMemoryStatus();

		void DumpStatistics(std::ostream &S);
	private:
		SHORT m_instance;
	};

	class Exception
		: public std::exception
	{
	public:
		Exception(const std::string &Message)
			: m_message(Message)
		{
		}
		~Exception() throw() {} 
		const char * what() const throw()
		{
			return m_message.c_str();
		}
	private:
		std::string m_message;
	};

	//////////
	struct StreamBridge;
	class Stream
	{
	public:
		Stream();
		virtual ~Stream();

		virtual ULONG Open() = 0;
		virtual ULONG Read(void* buffer, ULONG bufferSize) = 0;
		virtual ULONG Write(void* buffer, ULONG bufferSize);
		virtual LONGLONG Seek(LONGLONG offset, ULONG whence) = 0;
		virtual void Close();

		virtual IGR_Stream* getIGRStream() const;
	private:
		StreamBridge* m_proxy;
	};

	class IGRStream : public Stream
	{
		PS_CLASS_NO_COPY(IGRStream)
	public:
		IGRStream();
		IGRStream(IGR_Stream* stream);

		ULONG Open();
		ULONG Read(void* buffer, ULONG bufferSize);
		ULONG Write(void* buffer, ULONG bufferSize);
		LONGLONG Seek(LONGLONG offset, ULONG whence);
		void Close();

		IGR_Stream* getIGRStream() const;
	protected:
		IGR_Stream* m_stream;
	};

	class FileStream : public Stream
	{
		PS_CLASS_NO_COPY(FileStream)
	public:
		FileStream(const std::string &filename, const std::string& Mode = "rb");
		~FileStream();

		ULONG Open();
		ULONG Read(void* buffer, ULONG bufferSize);
		ULONG Write(void* buffer, ULONG bufferSize);
		LONGLONG Seek(LONGLONG offset, ULONG whence);
		void Close();
	private:
		FILE* m_file;
	};

	class MemStream : public IGRStream
	{
		PS_CLASS_NO_COPY(MemStream)
	public:
		MemStream(void *Data, LONG DataSize);
	};

	class MemOutputStream : public Stream
	{
		PS_CLASS_NO_COPY(MemOutputStream)
	public:
		MemOutputStream();
		virtual ~MemOutputStream();
		ULONG Open();
		ULONG Read(void* buffer, ULONG bufferSize);
		ULONG Write(void* buffer, ULONG bufferSize);
		LONGLONG Seek(LONGLONG offset, ULONG whence);
		void Close();
		void Reset();
		ULONG WriteToFile(FILE *f);
		ULONG SaveToFile(const std::string &filename);
	private:
		size_t m_data_pos;
		std::vector<BYTE> m_data;
	};


	std::wstring ToWideStr(const WCHAR *S, const int NumChars);
	std::string ToUTF8Str(const WCHAR *S, const int NumChars);
	std::string ToWCHARStr(const std::string &S);
	std::string ToWCHARStr(const std::wstring &S);

	std::string GetErrorStr(const int ReturnCode);
	std::string GetFileTypeName(const int FileType);
	std::string HTMLEncode(const std::string &S);
	std::string ReplaceAll(const std::string &S, const std::string& Search, const std::string& Replace);

	void PrepareFileForBinaryOutput(FILE *F);
	void PrepareFileForWideOutput(FILE *F, const bool IncludeBOM);
	void PrepareFileForUTF8Output(FILE *F, const bool IncludeBOM);
	std::string ReadTextFile(const std::string &filename);

	bool SameText(const std::string &S1, const std::string &S2);
	std::string IncludeTrailingPathDelimiter(const std::string &Path);
	std::string RemoveTrailingPathDelimiter(const std::string &Path);
	std::string JoinFilename(const std::string &Path, const std::string &filename);
	std::string MakeSafeFilename(const std::string &filename);
	bool FileExists(const std::string &filename);
	bool DirExists(const std::string &Path);
	bool CreateFolder(const std::string &Path);
	bool CopyFile(const std::string &Source, const std::string &Dest, bool overwrite);
	std::string ExtractFilePath(const std::string &Path);
	std::string ExtractFileName(const std::string &Path);
	std::string ChangeFileExt(const std::string &filename, const std::string &Ext);

}; // namespace Perceptive

#endif // PERCEPTIVE_DOCFILTERSOBJECTS_H_DEF
