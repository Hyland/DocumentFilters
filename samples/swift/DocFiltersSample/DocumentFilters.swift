// (C) 2017 Hyland Software, Inc. and its affiliates. All rights reserved.

import Foundation
import ISYS11df
import UIKit

struct DocumentFilters {
    typealias Exception = DocumentFiltersAPI.DocFiltersException

    enum FormatType: Int32 {
        case kLongName = 0
        case kShortName = 1
        case kConfigName = 2
        case kClassName = 3
        case kLegacy = 4
    }

    enum CanvasType: Int32 {
        case kPNG = 0
        case kJPG = 1
        case kPDF = 2
        case kTIF = 3
        case kBMP = 4
        case kPBM = 7
        case kPGM = 8
        case kPPM = 9
        case kWEBP = 10
        case kXPS = 11
        case kSVG = 12
        case kEPS = 13
        case kPS = 14
        case kXML = 5
        case kHTML = 6
    }

    struct FileOptions: OptionSet {
        let rawValue: Int32
        static let kBodyOnly = FileOptions(rawValue: ISYS11df.IGR_BODY_ONLY)
        static let kMetaOnly = FileOptions(rawValue: ISYS11df.IGR_META_ONLY)
        static let kBodyAndMeta = FileOptions(rawValue: ISYS11df.IGR_BODY_AND_META)
        static let kFormatText = FileOptions(rawValue: Int32(ISYS11df.IGR_FORMAT_TEXT))
        static let kFormatHTML = FileOptions(rawValue: Int32(ISYS11df.IGR_FORMAT_HTML))
        static let kFormatXML = FileOptions(rawValue: Int32(ISYS11df.IGR_FORMAT_XML))
        static let kFormatImage = FileOptions(rawValue: Int32(ISYS11df.IGR_FORMAT_IMAGE))
    }

    class DocumentFiltersObject {
        internal init(_ api: DocumentFiltersAPI) {
            self.api = api
        }

        internal init(_ parent: DocumentFiltersObject) {
            api = parent.api
        }

        internal init() {
            api = DocumentFiltersAPI()
        }

        internal var api: DocumentFiltersAPI
    }

    class Instance: DocumentFiltersObject {
        override init() {
            super.init()
        }

        init(_ license: String, path: String = "") throws {
            super.init()
            try Initialize(license, path: path)
        }

        func Initialize(_ license: String, path: String = "") throws {
            var apppath = path
            if apppath == "" || apppath == "." {
                apppath = Bundle.main.bundlePath
            }

            try _handle = api.Initialize(path: apppath, license: license)
        }

        func GetExtractor(filename: String) throws -> Extractor {
            return try Extractor(self, filename: filename)
        }

        func GetExtractor(bytes: inout [UInt8]) throws -> Extractor {
            return try Extractor(self, bytes: &bytes)
        }

        func GetExtractor(data: Data) throws -> Extractor {
            return try Extractor(self, from: data)
        }

        func MakeOutputCanvas(filename: String, type: CanvasType, options: String) throws -> Canvas {
            return Canvas(self, canvas_handle: try api.MakeOutputCanvas(type: type.rawValue, filename: filename, options: options))
        }

        func MakeOutputCanvas(stream: PIGR_Writable_Stream, type: CanvasType, options: String) throws -> Canvas {
            return Canvas(self, canvas_handle: try api.MakeOutputCanvas(type: type.rawValue, stream: stream, options: options))
        }

        func MakeOutputCanvas(data: Data, type: CanvasType, options: String) throws -> Canvas {
            return try Canvas(self, data: data, type: type.rawValue, options: options)
        }

        var handle: Int16 { return _handle }
        private var _handle: Int16 = 0
    }

    class Extractor: DocumentFiltersObject {
        init(_ parent: DocumentFiltersObject, filename: String) throws {
            super.init(parent)
            _stream = try api.MakeStreamFromFile(filename)
        }

        init(_ parent: DocumentFiltersObject, bytes: inout [UInt8]) throws {
            super.init(parent)
            _stream = try api.MakeStreamFromBytes(bytes: &bytes)
        }

        init(_ parent: DocumentFiltersObject, from: Data) throws {
            super.init(parent)
            _bridge = DocumentFiltersAPI.StreamDataBridge(with: from)
            _stream = _bridge?.IGRStream()
        }

        override init(_ parent: DocumentFiltersObject) {
            super.init(parent)
        }

        deinit {
            do { try Close() } catch {}
        }

        func Close() throws {
            if _handle > 0 {
                try api.CloseFile(_handle)
                _handle = 0
            }
            if _stream != nil {
                _stream?.pointee.Close(_stream)
                _stream = nil
            }
            if _bridge != nil {
                _bridge = nil
            }
        }

        func Open(flags: FileOptions, options: String = "") throws {
            if _handle == 0 {
                _ = try need_handle(flags: flags.rawValue, options: options)
            }
        }

        func getFileType() throws -> Int32 {
            if _handle > 0 {
                return _type
            } else if _stream != nil {
                return try api.GetFileType(_stream!)
            } else {
                return try api.GetFileType((try need_stream())!)
            }
        }

        func getCapabilities() throws -> Int32 {
            if _handle > 0 {
                return _caps
            } else if _stream != nil {
                return try api.GetFileTypeAndCapabilities(_stream!).caps
            } else {
                return try api.GetFileTypeAndCapabilities((try need_stream())!).caps
            }
        }

        func getFileType(what: FormatType) throws -> String {
            return try api.GetFileTypeAttribute(getFileType(), attr: what.rawValue)
        }

        func getSupportsText() throws -> Bool {
            return (try getCapabilities() & ISYS11df.IGR_FILE_SUPPORTS_TEXT) != 0
        }

        func getSupportsSubFiles() throws -> Bool {
            return (try getCapabilities() & ISYS11df.IGR_FILE_SUPPORTS_SUBFILES) != 0
        }

        func getSupportsHD() throws -> Bool {
            return (try getCapabilities() & ISYS11df.IGR_FILE_SUPPORTS_HDHTML) != 0
        }

        func getEOF() -> Bool {
            return _eof
        }

        func GetText(_ length: Int) throws -> String {
            var retval: String = ""
            _eof = try !api.GetText(try need_handle(), &retval, length: length)
            return retval
        }

        func SaveTo(to: URL) throws {
            while !getEOF() {
                let text = try GetText(4096)

                try text.write(to: to, atomically: false, encoding: .utf8)
            }
        }

        func SaveTo(filename: String) throws {
            try SaveTo(to: URL(fileURLWithPath: filename))
        }

        func getHashMD5() throws -> String {
            return try api.GetHashMD5(stream: (try need_stream())!)
        }

        func getHashSHA1() throws -> String {
            return try api.GetHashSHA1(stream: (try need_stream())!)
        }

        func GetFirstSubFile() throws -> SubFile? {
            return try GetNextSubFile()
        }

        func GetNextSubFile() throws -> SubFile? {
            let val = try api.GetNextSubFile(try need_handle())
            if val != nil {
                return SubFile(self, doc_handle: try need_handle(), entry: val!)
            } else {
                return nil
            }
        }

        func GetSubFile(id: String) throws -> SubFile? {
            let val = DocumentFiltersAPI.SubFileEntry(id, "", 0, nil)
            return SubFile(self, doc_handle: try need_handle(), entry: val)
        }

        func GetFirstImage() throws -> SubFile? {
            return try GetNextImage()
        }

        func GetNextImage() throws -> SubFile? {
            let val = try api.GetNextImage(try need_handle())
            if val != nil {
                let doc_handle = try need_handle()

                return SubFile(self, doc_handle: doc_handle, entry: val!, extractor: { () -> PIGR_Stream? in
                    (try self.api.ExtractImage(doc_handle, id: val!.id))!
                })
            } else {
                return nil
            }
        }

        func CopyTo(fileURL: URL) throws {
            let stream = try need_stream()!

            var buffer = [UInt8](repeating: 0, count: 1024)

            // Get the current seek position
            let pos = stream.pointee.Seek(stream, 0, 1)
            // Seek to the end
            _ = stream.pointee.Seek(stream, 0, 2)

            var read: UInt32 = stream.pointee.Read(stream, &buffer, UInt32(buffer.count))
            while read > 0 {
                try Data(bytes: buffer, count: Int(read)).write(to: fileURL)

                read = stream.pointee.Read(stream, &buffer, UInt32(buffer.count))
            }
            // Seek back to original position
            _ = stream.pointee.Seek(stream, pos, 0)
        }

        func CopyTo(filename: String) throws {
            try CopyTo(fileURL: URL(fileURLWithPath: filename))
        }

        func GetFirstPage() throws -> Page? {
            _page_index = 0
            return try GetNextPage()
        }

        func GetNextPage() throws -> Page? {
            if try _page_index < getPageCount() {
                let retval = try GetPage(_page_index)
                _page_index = _page_index + 1
                return retval
            } else {
                return nil
            }
        }

        func GetPage(_ page: Int) throws -> Page {
            return Page(self, doc_handle: try need_image_handle(), page: page)
        }

        func getPageCount() throws -> Int {
            return try api.PageCount(handle: need_image_handle())
        }

        var FileType: Int32 { do { return try getFileType() } catch { return 0 } }
        var SupportsText: Bool { do { return try getSupportsText() } catch { return false } }
        var SupportsSubFiles: Bool { do { return try getSupportsSubFiles() } catch { return false } }
        var SupportsHD: Bool { do { return try getSupportsHD() } catch { return false } }
        var EOF: Bool { return getEOF() }
        var PageCount: Int { do { return try getPageCount() } catch { return 0 } }

        internal func need_handle(flags: Int32 = 0, options: String = "") throws -> Int32 {
            if _handle == 0 {
                _handle = try api.OpenFile(need_stream()!, flags: flags, options: options, caps: &_caps, type: &_type)
                _eof = false
            }

            if _handle == 0 {
                throw DocumentFiltersAPI.DocFiltersException.Unknown(message: "Could not open file")
            }
            return _handle
        }
        internal func need_image_handle() throws -> Int32 {
            return try need_handle(flags: Int32(ISYS11df.IGR_FORMAT_IMAGE))
        }
        internal func need_stream() throws -> PIGR_Stream? {
            return _stream
        }
        internal var _stream: PIGR_Stream?
        internal var _bridge: DocumentFiltersAPI.StreamDataBridge?

        private var _handle: Int32 = 0
        private var _caps: Int32 = 0
        private var _type: Int32 = 0
        private var _eof: Bool = true
        private var _page_index: Int = 0
    }

    class SubFile: Extractor {
        init(_ parent: DocumentFiltersObject, doc_handle: Int32, entry: DocumentFiltersAPI.SubFileEntry) {
            _entry = entry
            _doc_handle = doc_handle
            _extractor = { () throws -> PIGR_Stream? in
                return try parent.api.ExtractSubFile(doc_handle, id: entry.id)
            }
            super.init(parent)
        }

        init(_ parent: DocumentFiltersObject, doc_handle: Int32, entry: DocumentFiltersAPI.SubFileEntry, extractor: @escaping () throws -> PIGR_Stream?) {
            _entry = entry
            _doc_handle = doc_handle
            _extractor = extractor
            super.init(parent)
        }

        func getFileTime() -> Date? {
            return _entry.date
        }

        func getID() -> String {
            return _entry.id
        }

        func getName() -> String {
            return _entry.name
        }

        func getFileSize() -> Int64 {
            return _entry.size
        }

        internal override func need_stream() throws -> PIGR_Stream? {
            if _stream == nil {
                _stream = try _extractor()
            }
            return _stream
        }

        var FileTime: Date? { return getFileTime() }
        var ID: String { return getID() }
        var Name: String { return getName() }
        var FileSize: Int64 { return getFileSize() }

        private var _entry: DocumentFiltersAPI.SubFileEntry
        private var _doc_handle: Int32
        private var _extractor: () throws -> PIGR_Stream?
    }

    class Page: DocumentFiltersObject {
        init(_ parent: DocumentFiltersObject, doc_handle: Int32, page: Int) {
            _doc_handle = doc_handle
            _page = page
            super.init(parent)
        }

        deinit {
            do { try Close() } catch {}
        }

        func Close() throws {
            if _page_handle > 0 {
                try api.ClosePage(_page_handle)
            }
            _page_handle = 0
        }

        func GetWordCount() throws -> Int {
            _ = try need_handle()
            return Int(_word_count)
        }

        func GetWidth() throws -> Int {
            _ = try need_handle()
            return Int(_dimensions.width)
        }

        func GetHeight() throws -> Int {
            _ = try need_handle()
            return Int(_dimensions.height)
        }

        func GetText(_ length: Int32 = 4096) throws -> String {
            var retval: String = ""
            if try api.GetPageText(try need_handle(), &retval, length: Int(length)) {
                return retval
            }
            return ""
        }

        func GetFirstWord() throws -> Word? {
            _word_index = 0
            return try GetNextWord()
        }

        func GetNextWord() throws -> Word? {
            if try _word_index < GetWordCount() {
                try need_words()
                let retval = Word(self, word: _words[_word_index], word_index: _word_index)
                _word_index = _word_index + 1
                return retval
            }
            return nil
        }

        func GetFirstImage() throws -> SubFile? {
            return try GetNextImage()
        }

        func GetNextImage() throws -> SubFile? {
            let page_handle = try need_handle()

            let val = try api.GetNextPageImage(page_handle)
            if val != nil {
                return SubFile(self, doc_handle: 0, entry: val!, extractor: { () -> PIGR_Stream? in
                    (try self.api.ExtractPageImage(page_handle, id: val!.id))!
                })
            } else {
                return nil
            }
        }

        func Redact(first: Word, last: Word) throws {
            try Redact(first: first.GetWordIndex(), last: last.GetWordIndex())
        }

        func Redact(first: Int, last: Int) throws {
            try api.PageRedact(try need_handle(), first_word: Int32(first), last_word: Int32(last))
        }

        func GetAttribute(name: String) throws -> String {
            return try api.PageAttribute(try need_handle(), attribute_name: name)
        }

        func GetImage(_ options: String = "") throws -> UIImage? {
            let stream = DocumentFiltersAPI.StreamDataBridge()

            let canvas = try api.MakeOutputCanvas(type: CanvasType.kPNG.rawValue, stream: stream.IGRWritableStream(), options: options)
            try api.RenderPage(page: need_handle(), canvas: canvas)
            try api.CloseCanvas(canvas)

            return UIImage(data: stream.data()!)
        }

        internal func need_handle() throws -> Int32 {
            if _page_handle == 0 {
                _page_handle = try api.OpenPage(dochandle: _doc_handle, page: Int32(_page))
                _dimensions = try api.PageDimensions(page_handle: _page_handle)
                _word_count = try api.WordCount(page_handle: _page_handle)
            }
            return _page_handle
        }

        internal func need_words() throws {
            _words = try api.PageWords(try need_handle(), word_count: try GetWordCount())
        }

        private var _page_handle: Int32 = 0
        private var _doc_handle: Int32 = 0
        private var _page: Int = 0
        private var _dimensions: DocumentFiltersAPI.PageSize = (0, 0)
        private var _word_count: Int = 0
        private var _word_index: Int = 0
        private var _words: [ISYS11df.IGR_Page_Word] = []
    }

    class Word: DocumentFiltersObject {
        init(_ parent: DocumentFiltersObject, word: ISYS11df.IGR_Page_Word, word_index: Int) {
            _word = word
            _index = word_index
            super.init(parent)
        }

        func GetText() -> String {
            var tmp = [UInt16](repeating: 0, count: Int(_word.wordLength))
            memcpy(&tmp, _word.word, Int(_word.wordLength * 2))
            return (String.decodeCString(tmp, as: UTF16.self)?.result)!
        }

        func GetX() -> Int {
            return Int(_word.x)
        }

        func GetY() -> Int {
            return Int(_word.y)
        }

        func GetWidth() -> Int {
            return Int(_word.width)
        }

        func GetHeight() -> Int {
            return Int(_word.height)
        }

        func GetCharacterOffset() -> Int {
            return Int(_word.charoffset)
        }

        func GetWordIndex() -> Int {
            return Int(_index)
        }

        private var _word: ISYS11df.IGR_Page_Word = ISYS11df.IGR_Page_Word()
        private var _index: Int = 0
    }

    class Canvas: DocumentFiltersObject {
        init(_ parent: DocumentFiltersObject, canvas_handle: Int32) {
            super.init(parent)
            _handle = canvas_handle
        }

        init(_ parent: DocumentFiltersObject, data: Data, type: Int32, options: String) throws {
            super.init(parent)

            _bridge = DocumentFiltersAPI.StreamDataBridge(with: data)
            _handle = try api.MakeOutputCanvas(type: type, stream: (_bridge?.IGRWritableStream())!, options: options)
        }

        init(_ parent: DocumentFiltersObject, stream: PIGR_Writable_Stream, type: Int32, options: String) throws {
            super.init(parent)

            _handle = try api.MakeOutputCanvas(type: type, stream: stream, options: options)
        }

        deinit {
            do { try Close() } catch {}
        }

        func Close() throws {
            if _handle > 0 {
                try api.CloseCanvas(_handle)
                _handle = 0
            }
        }

        func RenderPage(page: Page) throws {
            try api.RenderPage(page: page.need_handle(), canvas: need_handle())
        }

        func AddCustomMetaData(name: String, value: String) throws {
            if !_meta.isEmpty {
                _meta += ";"
            }
            _meta += "META(\"\(name)\", \"\(value)\")"
        }

        func Arc(x: Int, y: Int, x2: Int, y2: Int, x3: Int, y3: Int, x4: Int, y4: Int) throws {
            try api.CanvasArc(need_handle(), x, y, x2, y2, x3, y3, x4, y4)
        }

        func AngleArc(x1: Int, y1: Int, radius: Int, startAngle: Int, sweepAngle: Int) throws {
            try api.CanvasAngleArc(need_handle(), x1, y1, radius, startAngle, sweepAngle)
        }

        func Chord(x: Int, y: Int, x2: Int, y2: Int, x3: Int, y3: Int, x4: Int, y4: Int) throws {
            try api.CanvasChord(need_handle(), x, y, x2, y2, x3, y3, x4, y4)
        }

        func Ellipse(x: Int, y: Int, x2: Int, y2: Int) throws {
            try api.CanvasEllipse(need_handle(), x, y, x2, y2)
        }

        func Rect(x: Int, y: Int, x2: Int, y2: Int) throws {
            try api.CanvasRect(need_handle(), x, y, x2, y2)
        }

        func LineTo(x: Int, y: Int) throws {
            try api.CanvasLineTo(need_handle(), x, y)
        }

        func MoveTo(x: Int, y: Int) throws {
            try api.CanvasMoveTo(need_handle(), x, y)
        }

        func Pie(x: Int, y: Int, x2: Int, y2: Int, x3: Int, y3: Int, x4: Int, y4: Int) throws {
            try api.CanvasPie(need_handle(), x, y, x2, y2, x3, y3, x4, y4)
        }

        func RoundRect(x: Int, y: Int, x2: Int, y2: Int, radius: Int) throws {
            try api.CanvasRoundRect(need_handle(), x, y, x2, y2, radius)
        }

        func TextOut(x: Int, y: Int, text: String) throws {
            try api.CanvasTextOut(need_handle(), x, y, text)
        }

        func TextRect(x _: Int, y _: Int, x2 _: Int, y2 _: Int, text _: String, flags _: Int) throws {
        }

        func TextWidth(text: String) throws -> Int {
            return try api.CanvasTextWidth(need_handle(), text)
        }

        func TextHeight(text: String) throws -> Int {
            return try api.CanvasTextHeight(need_handle(), text)
        }

        func SetPen(color: Int, width: Int, style: Int) throws {
            try api.CanvasSetPen(need_handle(), color, width, style)
        }

        func SetBrush(color: Int, style: Int) throws {
            try api.CanvasSetBrush(need_handle(), color, style)
        }

        func SetFont(text: String, size: Int, style: Int) throws {
            try api.CanvasSetFont(need_handle(), text, size, style)
        }

        func SetOpacity(opacity: UInt8) throws {
            try api.CanvasSetOpacity(need_handle(), opacity)
        }

        func DrawImage(x: Int, y: Int, imagedata: inout [UInt8], imagesize: Int, mimetype: String) throws {
            try api.CanvasDrawImage(need_handle(), x, y, &imagedata, imagesize, mimetype)
        }

        func DrawScaleImage(x: Int, y: Int, x2: Int, y2: Int, imagedata: inout [UInt8], imagesize: Int, mimetype: String) throws {
            try api.CanvasDrawScaleImage(need_handle(), x, y, x2, y2, &imagedata, imagesize, mimetype)
        }

        func Rotation(degrees: Int) throws {
            try api.CanvasRotation(need_handle(), degrees)
        }

        func Reset() throws {
            try api.CanvasReset(need_handle())
        }

        internal func need_handle() -> Int32 {
            return _handle
        }

        private var _handle: Int32 = 0
        private var _meta: String = ""
        private var _bridge: DocumentFiltersAPI.StreamDataBridge?
    }
}
