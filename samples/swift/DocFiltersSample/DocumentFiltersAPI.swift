// (C) 2017 Hyland Software, Inc. and its affiliates. All rights reserved.
//
// Provides utility functions around the "C" functions, marshalling data to and from swift types
//

import Foundation
import ISYS11df

internal struct PayloadStreamBridge {
    var method_open: Int = 0
    var method_read: Int = 0
    var method_seek: Int = 0
    var method_close: Int = 0
    var method_write: Int = 0
    var offset: Int = 0
    var data: Data

    init(_ data: Data) {
        self.data = data
    }
}

internal func GetStreamPayload<T>(_ stream: UnsafeMutablePointer<T>, _ run: @escaping (_: UnsafeMutablePointer<PayloadStreamBridge>) -> Void) {

    _ = stream
        .withMemoryRebound(to: PayloadStreamBridge.self, capacity: 1) { ptr -> PayloadStreamBridge in
            run(UnsafeMutablePointer(ptr))
            return ptr.pointee
        }
}

class DocumentFiltersAPI {
    enum DocFiltersException: Error {
        case Unknown(message: String)
    }

    class StreamDataBridge {
        private var _bridge: PayloadStreamBridge

        convenience init() {
            self.init(with: Data())
        }

        init(with: Data) {
            _bridge = PayloadStreamBridge(with)

            let igr = withUnsafeMutablePointer(to: &_bridge.method_open) { ptr -> UnsafeMutablePointer<IGR_Writable_Stream> in
                ptr.withMemoryRebound(to: ISYS11df.IGR_Writable_Stream.self, capacity: 1) {
                    return UnsafeMutablePointer<IGR_Writable_Stream>($0)
                }
            }
            igr.pointee.Write = { (stream, buffer, buffer_length) -> UInt32 in
                GetStreamPayload(stream!, { payload in
                    // Make sure there is enough space
                    if payload.pointee.data.count < payload.pointee.offset + Int(buffer_length) {
                        payload.pointee.data.count = payload.pointee.offset + Int(buffer_length)
                    }
                    // Write the data
                    payload.pointee.data.withUnsafeMutableBytes() { (bytes: UnsafeMutablePointer<UInt8>) -> Void in
                        let pos = bytes.advanced(by: payload.pointee.offset)
                        memcpy(UnsafeMutablePointer(pos), buffer, Int(buffer_length))
                    }
                    // Advance the seek pointer
                    payload.pointee.offset += Int(buffer_length)
                })
                return buffer_length
            }
            igr.pointee.base.Close = { _ in
            }
            igr.pointee.base.Open = { _ in
                0
            }
            igr.pointee.base.Read = { (stream, buffer, buffer_length) -> UInt32 in
                var to_read: Int = 0
                GetStreamPayload(stream!, { payload in
                    let available = payload.pointee.data.count - payload.pointee.offset

                    to_read = min(available, Int(buffer_length))

                    payload.pointee.data.withUnsafeMutableBytes() { (bytes: UnsafeMutablePointer<UInt8>) -> Void in
                        let pos = bytes.advanced(by: payload.pointee.offset)
                        memcpy(buffer, UnsafeMutablePointer(pos), to_read)
                    }
                })
                return UInt32(to_read)
            }
            igr.pointee.base.Seek = { (stream, offset, whense) -> Int64 in
                var retval: Int = 0
                GetStreamPayload(stream!, { payload in
                    retval = payload.pointee.offset
                    if whense == 0 {
                        retval = Int(offset)
                    } else if whense == 1 {
                        retval += Int(offset)
                    } else if whense == 2 {
                        retval = payload.pointee.data.count + Int(offset)
                    }
                    payload.pointee.offset = max(retval, 0)
                })
                return Int64(retval)
            }
        }

        deinit {
        }

        func data() -> Data? {
            return _bridge.data
        }

        func IGRStream() -> UnsafeMutablePointer<ISYS11df.IGR_Stream> {
            return withUnsafeMutablePointer(to: &_bridge.method_open) { ptr -> UnsafeMutablePointer<IGR_Stream> in
                ptr.withMemoryRebound(to: ISYS11df.IGR_Stream.self, capacity: 1) {
                    return UnsafeMutablePointer<IGR_Stream>($0)
                }
            }
        }

        func IGRWritableStream() -> UnsafeMutablePointer<ISYS11df.IGR_Writable_Stream> {
            return withUnsafeMutablePointer(to: &_bridge.method_open) { ptr -> UnsafeMutablePointer<IGR_Writable_Stream> in
                ptr.withMemoryRebound(to: ISYS11df.IGR_Writable_Stream.self, capacity: 1) {
                    return UnsafeMutablePointer<IGR_Writable_Stream>($0)
                }
            }
        }
    }

    /**
     Checks the error code from an IGR call, throwing an exception if needed
     @param code contains the result value as returned from an IGR call
     @param ecb contains the error structured populated by an IGR call
     @return the code passed in
     */
    @discardableResult
    private func check(_ code: Int32, _ ecb: inout Error_Control_Block) throws -> Int32 {
        if code != 0 && code != ISYS11df.IGR_NO_MORE {
            try check(&ecb)

            throw DocFiltersException.Unknown(message: "Unknown Exception")
        }
        return code
    }

    /**
     Checks the error control struct, throwing an exception if needed
     @param ecb contains the error structured populated by an IGR call
     */
    private func check(_ ecb: inout Error_Control_Block) throws {
        if ecb.Msg.0 != 0 {

            let msg = withUnsafePointer(to: &ecb.Msg.0) { ptr -> String in
                return String(cString: ptr)
            }

            throw DocFiltersException.Unknown(message: msg)
        }
    }

    /**
     Initializes the Document Filters engine and authenticates your license.
     - Parameters:
     - path: [in] Path to DocFilters executables
     - license: [in] Populate with your application License Code.
     */
    func Initialize(path: String, license: String) throws -> Int16 {
        var ecb = Error_Control_Block()
        var isb = Instance_Status_Block()
        var retval: Int16 = 0

        withUnsafeMutablePointer(to: &isb.Licensee_ID1.0) { ptr -> Void in
            strlcpy(UnsafeMutablePointer(ptr), license, Int(MemoryLayout.size(ofValue: isb.Licensee_ID1)))
        }

        return try path.withCString { str_path -> Int16 in
            ISYS11df.Init_Instance(0, str_path, &isb, &retval, &ecb)
            try check(&ecb)
            return retval
        }
    }

    /**
     MakeStreamFromBytes creates a DocFilters stream based on a byte array for use with the stream functions.
     - Parameters:
     - bytes: Byte array to construct the stream from, the caller must ensure the byte array remains in memory, unmodified for the lifetime of the stream
     - Returns: a new IGR_Stream, it must be freed by calling stream.Close
     */
    func MakeStreamFromBytes(bytes: inout [UInt8]) throws -> PIGR_Stream {
        var retval: PIGR_Stream?
        var ecb = Error_Control_Block()
        try check(IGR_Make_Stream_From_Memory(&bytes, bytes.count, nil, &retval, &ecb), &ecb)
        return retval!
    }

    /**
     MakeStreamFromFile creates a DocFilters stream based on a file for use with the stream functions.
     - Parameters:
     - filename: The name of the file to load
     - Returns: a new IGR_Stream, it must be freed by calling stream.Close
     */
    func MakeStreamFromFile(_ filename: String) throws -> PIGR_Stream {
        return try filename.withCString(encodedAs: UTF16.self) { ptr -> PIGR_Stream in
            var retval: PIGR_Stream?
            var ecb = Error_Control_Block()
            try check(IGR_Make_Stream_From_File(ptr, 0, &retval, &ecb), &ecb)
            return retval!
        }
    }

    /**
     Opens a document for text extraction or enumeration of sub-documents and also controls the output format, including converting the source document to HTML.
     - Parameters:
     - filename: Path to the document to be opened.
     - flags: Specifies what type of data is returned from subsequent calls to the IGR_Get_Text function. These Open Document Flags affect the verbosity or the format of the extracted data.
     - options: Extended processing options, used when converting the document to HTML. The Open Document Options are expressed as Name=Value with a semicolon delimiter.
     - caps: Returns the Document Capabilities as a bitfield.
     - type Returns the Document Format Code of the document.
     - Returns: a handle to be used in subsequent calls.
     */
    func OpenFile(_ filename: String, flags: Int32, options: String, caps: inout Int32, type: inout Int32) throws -> Int32 {
        var ecb = Error_Control_Block()
        var retval = Int32(0)

        try filename.withCString(encodedAs: UTF16.self) { ptr -> Void in
            try options.withCString(encodedAs: UTF16.self) { opts -> Void in
                try check(ISYS11df.IGR_Open_File_Ex(ptr, flags, opts, &caps, &type, &retval, &ecb), &ecb)
            }
        }
        return retval
    }

    /**
     Opens a document for text extraction or enumeration of sub-documents and also controls the output format, including converting the source document to HTML.
     - Parameters:
     - filename: Path to the document to be opened.
     - flags: Specifies what type of data is returned from subsequent calls to the IGR_Get_Text function. These Open Document Flags affect the verbosity or the format of the extracted data.
     - options: Extended processing options, used when converting the document to HTML. The Open Document Options are expressed as Name=Value with a semicolon delimiter.
     - caps: Returns the Document Capabilities as a bitfield.
     - type Returns the Document Format Code of the document.
     - Returns: a handle to be used in subsequent calls.
     */
    func OpenFile(_ stream: PIGR_Stream, flags: Int32, options: String, caps: inout Int32, type: inout Int32) throws -> Int32 {
        var ecb = Error_Control_Block()
        var retval = Int32(0)

        try options.withCString(encodedAs: UTF16.self) { opts -> Void in
            try check(ISYS11df.IGR_Open_Stream_Ex(stream, flags, opts, &caps, &type, &retval, &ecb), &ecb)
        }
        return retval
    }

    /** Releases the resources associated with the file handle. It must be called for every document opened by IGR_Open_File, IGR_Open_File_Ex, IGR_Open_Stream or IGR_Open_Stream_Ex. */
    func CloseFile(_ handle: Int32) throws {
        var ecb = Error_Control_Block()
        try check(ISYS11df.IGR_Close_File(handle, &ecb), &ecb)
    }

    /** Extracts the text of previously opened document */
    func GetText(_ handle: Int32, _ text: inout String, length: Int) throws -> Bool {
        var buffer = [UInt16](repeating: 0, count: length)
        var ecb = Error_Control_Block()
        var bufferSize: Int32 = Int32(length)

        try check(ISYS11df.IGR_Get_Text(handle, &buffer, &bufferSize, &ecb), &ecb)

        text = (String.decodeCString(buffer, as: UTF16.self)?.result)!

        return bufferSize > 0
    }

    /** Returns a new canvas, used to render paginated output (pages) */
    func MakeOutputCanvas(type: Int32, filename: String, options: String) throws -> Int32 {
        var ecb = Error_Control_Block()
        var retval = Int32(0)

        try filename.withCString(encodedAs: UTF16.self) { pfilename -> Void in

            try options.withCString(encodedAs: UTF16.self) { poptions -> Void in
                try check(ISYS11df.IGR_Make_Output_Canvas(type, pfilename, poptions, &retval, &ecb), &ecb)
            }
        }
        return retval
    }

    /** Returns a new canvas, used to render paginated output (pages) */
    func MakeOutputCanvas(type: Int32, stream: PIGR_Writable_Stream, options: String) throws -> Int32 {
        var ecb = Error_Control_Block()
        var retval = Int32(0)

        try options.withCString(encodedAs: UTF16.self) { poptions -> Void in
            try check(ISYS11df.IGR_Make_Output_Canvas_On(type, stream, poptions, &retval, &ecb), &ecb)
        }
        return retval
    }

    /** Close the given canvas handle */
    func CloseCanvas(_ handle: Int32) throws {
        var ecb = Error_Control_Block()
        try check(ISYS11df.IGR_Close_Canvas(handle, &ecb), &ecb)
    }

    /** Renders the given page onto the canvas, this will create a new page for multi-page canvases */
    func RenderPage(page: Int32, canvas: Int32) throws {
        var ecb = Error_Control_Block()
        try check(ISYS11df.IGR_Render_Page(page, canvas, &ecb), &ecb)
    }

    /** Return a handle to the given page */
    func OpenPage(dochandle: Int32, page: Int32) throws -> Int32 {
        var ecb = Error_Control_Block()
        var retval = Int32(0)

        try check(ISYS11df.IGR_Open_Page(dochandle, page, &retval, &ecb), &ecb)
        return retval
    }

    /** Close the given page handle */
    func ClosePage(_ pagehandle: Int32) throws {
        var ecb = Error_Control_Block()

        try check(ISYS11df.IGR_Close_Page(pagehandle, &ecb), &ecb)
    }

    func GetFileTypeAndCapabilities(_ filename: String) throws -> (type: Int32, caps: Int32) {
        var ecb = Error_Control_Block()
        var caps: Int32 = 0
        var type: Int32 = 0

        try filename.withCString(encodedAs: UTF16.self) { ptr -> Void in
            try check(ISYS11df.IGR_Get_File_Type(ptr, &caps, &type, &ecb), &ecb)
        }
        return (type, caps)
    }

    func GetFileTypeAndCapabilities(_ stream: PIGR_Stream) throws -> (type: Int32, caps: Int32) {
        var ecb = Error_Control_Block()
        var caps: Int32 = 0
        var type: Int32 = 0

        try check(ISYS11df.IGR_Get_Stream_Type(stream, &caps, &type, &ecb), &ecb)
        return (type, caps)
    }

    func GetFileType(_ filename: String) throws -> Int32 {
        return try GetFileTypeAndCapabilities(filename).type
    }

    func GetFileType(_ stream: PIGR_Stream) throws -> Int32 {
        return try GetFileTypeAndCapabilities(stream).type
    }

    func GetFileCapabilities(_ filename: String) throws -> Int32 {
        return try GetFileTypeAndCapabilities(filename).caps
    }

    func GetFileTypeAttribute(_ format_code: Int32, attr: Int32) throws -> String {
        var ecb = Error_Control_Block()
        var buffer = [Int8](repeating: 0, count: 255)

        try check(IGR_Get_Format_Attribute(format_code, attr, &buffer, &ecb), &ecb)
        return String(describing: buffer)
    }

    func GetHashMD5(stream: PIGR_Stream) throws -> String {
        var buffer = [UInt16](repeating: 0, count: Int(PERCEPTIVE_SZ_MD5_HEX_LEN))
        var ecb = Error_Control_Block()

        try check(IGR_Calculate_MD5(stream, &buffer, &ecb), &ecb)
        return (String.decodeCString(buffer, as: UTF16.self)?.result)!
    }

    func GetHashSHA1(stream: PIGR_Stream) throws -> String {
        var buffer = [UInt16](repeating: 0, count: Int(PERCEPTIVE_SZ_SHA1_HEX_LEN))
        var ecb = Error_Control_Block()

        try check(IGR_Calculate_SHA1(stream, &buffer, &ecb), &ecb)
        return (String.decodeCString(buffer, as: UTF16.self)?.result)!
    }

    func MakeDate(_ val: Int64) -> Date? {
        if val <= 0 {
            return nil
        } else {
            var base = DateComponents()
            base.year = 1601
            base.month = 1
            base.day = 1

            return Date(timeInterval: TimeInterval(val / 10_000_000), since: Calendar.current.date(from: base)!)
        }
    }

    typealias SubFileEntry = (id: String, name: String, size: Int64, date: Date?)
    typealias PageSize = (width: Int32, height: Int32)

    func GetNextSubFile(_ handle: Int32) throws -> SubFileEntry? {
        var id = [UInt16](repeating: 0, count: 4096)
        var name = [UInt16](repeating: 0, count: 1024)
        var date: Int64 = 0
        var size: Int64 = 0
        var ecb = Error_Control_Block()

        if try check(ISYS11df.IGR_Get_Subfile_Entry(handle, &id, &name, &date, &size, &ecb), &ecb) == ISYS11df.IGR_NO_MORE {
            return nil
        } else {
            return (
                (String.decodeCString(id, as: UTF16.self)?.result)!,
                (String.decodeCString(name, as: UTF16.self)?.result)!,
                size,
                MakeDate(date)
            )
        }
    }

    func GetNextImage(_ handle: Int32) throws -> SubFileEntry? {
        var id = [UInt16](repeating: 0, count: 4096)
        var name = [UInt16](repeating: 0, count: 1024)
        var date: Int64 = 0
        var size: Int64 = 0
        var ecb = Error_Control_Block()

        if try check(ISYS11df.IGR_Get_Image_Entry(handle, &id, &name, &date, &size, &ecb), &ecb) == ISYS11df.IGR_NO_MORE {
            return nil
        } else {
            return (
                (String.decodeCString(id, as: UTF16.self)?.result)!,
                (String.decodeCString(name, as: UTF16.self)?.result)!,
                size,
                MakeDate(date)
            )
        }
    }

    func GetNextPageImage(_ page_handle: Int32) throws -> SubFileEntry? {
        var id = [UInt16](repeating: 0, count: 4096)
        var name = [UInt16](repeating: 0, count: 1024)
        var date: Int64 = 0
        var size: Int64 = 0
        var ecb = Error_Control_Block()

        if try check(ISYS11df.IGR_Get_Page_Image_Entry(page_handle, &id, &name, &date, &size, &ecb), &ecb) == ISYS11df.IGR_NO_MORE {
            return nil
        } else {
            return (
                (String.decodeCString(id, as: UTF16.self)?.result)!,
                (String.decodeCString(name, as: UTF16.self)?.result)!,
                size,
                MakeDate(date)
            )
        }
    }

    func ExtractImage(_ doc_handle: Int32, id: String) throws -> PIGR_Stream? {
        var retval: PIGR_Stream?
        var ecb = Error_Control_Block()

        return try id.withCString(encodedAs: UTF16.self) { str -> PIGR_Stream? in
            try check(ISYS11df.IGR_Extract_Image_Stream(doc_handle, str, &retval, &ecb), &ecb)
            return retval
        }
    }

    func ExtractSubFile(_ doc_handle: Int32, id: String) throws -> PIGR_Stream? {
        var retval: PIGR_Stream?
        var ecb = Error_Control_Block()

        return try id.withCString(encodedAs: UTF16.self) { str -> PIGR_Stream? in
            try check(ISYS11df.IGR_Extract_Subfile_Stream(doc_handle, str, &retval, &ecb), &ecb)
            return retval
        }
    }

    func ExtractPageImage(_ page_handle: Int32, id: String) throws -> PIGR_Stream? {
        var retval: PIGR_Stream?
        var ecb = Error_Control_Block()

        return try id.withCString(encodedAs: UTF16.self) { str -> PIGR_Stream? in
            try check(ISYS11df.IGR_Extract_Page_Image_Stream(page_handle, str, &retval, &ecb), &ecb)
            return retval
        }
    }

    func PageCount(handle: Int32) throws -> Int {
        var retval: Int32 = 0
        var ecb = Error_Control_Block()

        try check(IGR_Get_Page_Count(handle, &retval, &ecb), &ecb)
        return Int(retval)
    }

    func WordCount(page_handle: Int32) throws -> Int {
        var retval: Int32 = 0
        var ecb = Error_Control_Block()

        try check(IGR_Get_Page_Word_Count(page_handle, &retval, &ecb), &ecb)
        return Int(retval)
    }

    func PageDimensions(page_handle: Int32) throws -> PageSize {
        var width: Int32 = 0
        var height: Int32 = 0
        var ecb = Error_Control_Block()
        try check(IGR_Get_Page_Dimensions(page_handle, &width, &height, &ecb), &ecb)
        return (width, height)
    }

    func GetPageText(_ page_handle: Int32, _ text: inout String, length: Int) throws -> Bool {
        var buffer = [UInt16](repeating: 0, count: length)
        var ecb = Error_Control_Block()
        var bufferSize: Int32 = Int32(length)

        try check(ISYS11df.IGR_Get_Page_Text(page_handle, &buffer, &bufferSize, &ecb), &ecb)

        text = (String.decodeCString(buffer, as: UTF16.self)?.result)!

        return bufferSize > 0
    }

    func PageRedact(_ page_handle: Int32, first_word: Int32, last_word: Int32) throws {
        var ecb = Error_Control_Block()

        try check(IGR_Redact_Page_Text(page_handle, first_word, last_word, 0, &ecb), &ecb)
    }

    func PageAttribute(_ page_handle: Int32, attribute_name: String) throws -> String {
        var ecb = Error_Control_Block()
        var buffer_length: Int32 = 1024
        var buffer = [UInt16](repeating: 0, count: Int(buffer_length))

        return try attribute_name.withCString(encodedAs: UTF16.self) { ptr -> String in
            try check(IGR_Get_Page_Attribute(page_handle, ptr, &buffer, &buffer_length, &ecb), &ecb)
            return (String.decodeCString(buffer, as: UTF16.self)?.result)!
        }
    }

    func PageWords(_ page_handle: Int32, word_count: Int) throws -> [ISYS11df.IGR_Page_Word] {
        var retval = [ISYS11df.IGR_Page_Word](repeating: ISYS11df.IGR_Page_Word(), count: Int(word_count))
        var fetch: Int32 = Int32(word_count)
        var ecb = Error_Control_Block()

        try check(IGR_Get_Page_Words(page_handle, 0, &fetch, &retval, &ecb), &ecb)
        return retval
    }

    func CanvasArc(_ canvas_handle: Int32, _ x: Int, _ y: Int, _ x2: Int, _ y2: Int, _ x3: Int, _ y3: Int, _ x4: Int, _ y4: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_Arc(canvas_handle, Int32(x), Int32(y), Int32(x2), Int32(y2), Int32(x3), Int32(y3), Int32(x4), Int32(y4), &ecb), &ecb)
    }

    func CanvasAngleArc(_ canvas_handle: Int32, _ x1: Int, _ y1: Int, _ radius: Int, _ startAngle: Int, _ sweepAngle: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_AngleArc(canvas_handle, Int32(x1), Int32(y1), Int32(radius), Int32(startAngle), Int32(sweepAngle), &ecb), &ecb)
    }

    func CanvasChord(_ canvas_handle: Int32, _ x: Int, _ y: Int, _ x2: Int, _ y2: Int, _ x3: Int, _ y3: Int, _ x4: Int, _ y4: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_Chord(canvas_handle, Int32(x), Int32(y), Int32(x2), Int32(y2), Int32(x3), Int32(y3), Int32(x4), Int32(y4), &ecb), &ecb)
    }

    func CanvasEllipse(_ canvas_handle: Int32, _ x: Int, _ y: Int, _ x2: Int, _ y2: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_Ellipse(canvas_handle, Int32(x), Int32(y), Int32(x2), Int32(y2), &ecb), &ecb)
    }

    func CanvasRect(_ canvas_handle: Int32, _ x: Int, _ y: Int, _ x2: Int, _ y2: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_Rect(canvas_handle, Int32(x), Int32(y), Int32(x2), Int32(y2), &ecb), &ecb)
    }

    func CanvasLineTo(_ canvas_handle: Int32, _ x: Int, _ y: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_LineTo(canvas_handle, Int32(x), Int32(y), &ecb), &ecb)
    }

    func CanvasMoveTo(_ canvas_handle: Int32, _ x: Int, _ y: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_MoveTo(canvas_handle, Int32(x), Int32(y), &ecb), &ecb)
    }

    func CanvasPie(_ canvas_handle: Int32, _ x: Int, _ y: Int, _ x2: Int, _ y2: Int, _ x3: Int, _ y3: Int, _ x4: Int, _ y4: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_Arc(canvas_handle, Int32(x), Int32(y), Int32(x2), Int32(y2), Int32(x3), Int32(y3), Int32(x4), Int32(y4), &ecb), &ecb)
    }

    func CanvasRoundRect(_ canvas_handle: Int32, _ x: Int, _ y: Int, _ x2: Int, _ y2: Int, _ radius: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_RoundRect(canvas_handle, Int32(x), Int32(y), Int32(x2), Int32(y2), Int32(radius), &ecb), &ecb)
    }

    func CanvasTextOut(_ canvas_handle: Int32, _ x: Int, _ y: Int, _ text: String) throws {
        var ecb = Error_Control_Block()
        _ = try text.withCString(encodedAs: UTF16.self) { str in
            try check(IGR_Canvas_TextOut(canvas_handle, Int32(x), Int32(y), str, &ecb), &ecb)
        }
    }

    //	func CanvasTextRect(canvas_handle: Int32, x: Int, y: Int, x2: Int, y2: Int, text: String, flags: Int) throws {
    //		var ecb = Error_Control_Block();
    //		_ = try text.withCString(encodedAs: UTF16.self) { str in
    //			try check(IGR_Canvas_TextRect(canvas_handle, Int32(x), Int32(y), Int32(x2), Int32(y2), str, flags, &ecb), &ecb)
    //		}
    //	}
    func CanvasTextWidth(_ canvas_handle: Int32, _ text: String) throws -> Int {
        var ecb = Error_Control_Block()
        return try text.withCString(encodedAs: UTF16.self) { str -> Int in
            var width: Int32 = 0
            var height: Int32 = 0
            try check(IGR_Canvas_MeasureText(canvas_handle, str, &width, &height, &ecb), &ecb)
            return Int(width)
        }
    }

    func CanvasTextHeight(_ canvas_handle: Int32, _ text: String) throws -> Int {
        var ecb = Error_Control_Block()
        return try text.withCString(encodedAs: UTF16.self) { str -> Int in
            var width: Int32 = 0
            var height: Int32 = 0
            try check(IGR_Canvas_MeasureText(canvas_handle, str, &width, &height, &ecb), &ecb)
            return Int(height)
        }
    }

    func CanvasSetPen(_ canvas_handle: Int32, _ color: Int, _ width: Int, _ style: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_SetPen(canvas_handle, Int32(color), Int32(width), Int32(style), &ecb), &ecb)
    }

    func CanvasSetBrush(_ canvas_handle: Int32, _ color: Int, _ style: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_SetBrush(canvas_handle, Int32(color), Int32(style), &ecb), &ecb)
    }

    func CanvasSetFont(_ canvas_handle: Int32, _ text: String, _ size: Int, _ style: Int) throws {
        var ecb = Error_Control_Block()
        _ = try text.withCString(encodedAs: UTF16.self) { str in
            try check(IGR_Canvas_SetFont(canvas_handle, str, Int32(size), Int32(style), &ecb), &ecb)
        }
    }

    func CanvasSetOpacity(_ canvas_handle: Int32, _ opacity: UInt8) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_SetOpacity(canvas_handle, opacity, &ecb), &ecb)
    }

    func CanvasDrawImage(_ canvas_handle: Int32, _ x: Int, _ y: Int, _ imagedata: inout [UInt8], _ imagesize: Int, _ mimetype: String) throws {
        var ecb = Error_Control_Block()
        _ = try mimetype.withCString(encodedAs: UTF16.self) { str in
            try check(IGR_Canvas_DrawImage(canvas_handle, Int32(x), Int32(y), &imagedata, imagesize, str, &ecb), &ecb)
        }
    }

    func CanvasDrawScaleImage(_ canvas_handle: Int32, _ x: Int, _ y: Int, _ x2: Int, _ y2: Int, _ imagedata: inout [UInt8], _ imagesize: Int, _ mimetype: String) throws {
        var ecb = Error_Control_Block()
        _ = try mimetype.withCString(encodedAs: UTF16.self) { str in
            try check(IGR_Canvas_DrawScaleImage(canvas_handle, Int32(x), Int32(y), Int32(x2), Int32(y2), &imagedata, imagesize, str, &ecb), &ecb)
        }
    }

    func CanvasRotation(_ canvas_handle: Int32, _ degrees: Int) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_Rotation(canvas_handle, Int32(degrees), &ecb), &ecb)
    }

    func CanvasReset(_ canvas_handle: Int32) throws {
        var ecb = Error_Control_Block()
        try check(IGR_Canvas_Reset(canvas_handle, &ecb), &ecb)
    }
}
