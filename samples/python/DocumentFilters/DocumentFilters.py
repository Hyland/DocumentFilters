# (c) 2022 Hyland Software, Inc. and its affiliates. All rights reserved.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import base64
import datetime, re, json, io, ctypes
from enum import Enum
from typing import Callable
from . DocumentFiltersAPI import *


def _IsPythonStream(obj):
    try:
        return obj.seek != None
    except AttributeError:
        return False

def _IsIgrStream(obj):
    return isinstance(obj, IGRStream)

def __StreamBridge_Read(stream, buffer, length):
    try:
        s, _1, _2 = _stream_manager._ResolveStream(stream)
        bytes = s.read(length)
        ctypes.memmove(buffer, bytes, len(bytes))
        return len(bytes)
    except Exception as e:
        print("Stream_Read Error: " + str(e))
        return -1

def __StreamBridge_Seek(stream, offset, whence):
    try:
        s, _1, _2 = _stream_manager._ResolveStream(stream)
        s.seek(offset, whence)
        return s.tell()
    except Exception as e:
        print("Stream_Seek Error: " + str(e))
        return -1

def __StreamBridge_Write(stream, buffer, length):
    try:
        s, _1, _2 = _stream_manager._ResolveStream(stream)

        source = (length * ctypes.c_ubyte).from_address(buffer)[:length]
        data = bytearray(length)
        data[0:length] = source
        return s.write(data)

    except Exception as e:
        print("Stream_Write Error: " + str(e))
        return -1

def __StreamBridge_Destruct(stream):
    _1, bridge, _2 = _stream_manager._ResolveStream(stream)
    _stream_manager._UnregisterStream(bridge.Context)

def __StreamBridge_Action(stream, action, actionData):
    try:
        s, _1, _2 = _stream_manager._ResolveStream(stream)

        if action == IGR_ACTION_GET_STREAM_PART:
            nfo = ctypes.cast(actionData, ctypes.POINTER(IGR_T_ACTION_GET_STREAM_PART))
            res = s.GetStreamPart(nfo.contents.partName, nfo.contents.partFullName, nfo.contents.partIndex)
            if res != None:
                nfo.contents.istr = _stream_manager.MakeStreamBridge(res)
                return IGR_OK

        return IGR_E_BAD_ERROR
    except Exception as e:
        print("Stream_Action Error: " + str(e))
        return IGR_E_BAD_ERROR

def _igrstream_open(stream):
    return 0

def _igrstream_action(actionId, actionData, stream):
    try:
        return __StreamBridge_Action(stream, actionId, actionData)
    except Exception as e:
        return IGR_E_BAD_ERROR

FPtr_IGR_Stream_Seek = IGR_Make_Stream_Seek(__StreamBridge_Seek)
FPtr_IGR_Stream_Read = IGR_Make_Stream_Read(__StreamBridge_Read)
FPtr_IGR_Stream_Write = IGR_Make_Stream_Write(__StreamBridge_Write)
FPtr_IGR_Stream_Action = IGR_Make_Stream_Action(__StreamBridge_Action)
FPtr_IGR_Stream_Destruct = IGR_Make_Stream_Destruct(__StreamBridge_Destruct)
FPtr_IGR_Stream_Action2 = IGR_CALLBACK(_igrstream_action)

class _StreamBridge_Manager:

    def __init__(self):
        self._streams = []
        self._igr_streams = {}

    def _RegisterStream(self, stream):
        for i in range(len(self._streams)):
            if self._streams[i] == None:
                self._streams[i] = stream
                return i
        self._streams.append(stream)
        return len(self._streams) - 1

    def _UnregisterStream(self, index):
        if index >= 0 and index < len(self._streams):
            result = self._streams[index]
            self._streams[index] = None
            return result
        return None

    def _ResolveStream(self, stream):
        bridge = ctypes.cast(stream, ctypes.POINTER(IGR_Stream))
        return self._streams[bridge.contents.Context]

    def MakeStreamBridge(self, stream, allowWrites = False):
        api = ISYS11dfAPI.Singleton(None)

        if api.IGR_Make_Stream_From_Functions != None:
            # Internal state that is kept alive by _Register_Stream
            bridge = IGR_Stream()
            bridge.Context = self._RegisterStream((stream, bridge, None))

            # The final IGR_Stream populated by IGR_Make_Stream_From_Functions
            result = ctypes.POINTER(IGR_Stream)()

            # There is a defect in libffi on win32 where it incorrectly handles
            # stdcall callbacks. We force the callbacks to be cdecl. Win64
            # has a single calling convention which makes this a no-op.
            if _IsIgrStream(stream) and not allowWrites:
                action = FPtr_IGR_Stream_Action
            else:
                action = ctypes.cast(None, IGR_Make_Stream_Action)

            DocumentFiltersBase._Call(api.IGR_Make_Stream_From_Functions, \
                ctypes.byref(bridge), \
                IGR_MAKE_STREAM_FROM_FUNCTIONS_FLAGS_USECDECL, \
                FPtr_IGR_Stream_Seek, \
                FPtr_IGR_Stream_Read, \
                FPtr_IGR_Stream_Write, \
                action, \
                FPtr_IGR_Stream_Destruct, \
                ctypes.byref(result))

            return result
        else:
            payload = (ctypes.c_byte * ctypes.sizeof(IGR_Stream))()
            payload_p = ctypes.pointer(payload)
            stream_p = ctypes.cast(payload_p, ctypes.POINTER(IGR_Stream))

            # Internal state that is kept alive by _Register_Stream
            stream_p.contents.Open = IGR_Stream_Open(_igrstream_open)
            stream_p.contents.Close = IGR_Stream_Close(FPtr_IGR_Stream_Destruct)
            stream_p.contents.Seek = IGR_Stream_Seek(FPtr_IGR_Stream_Seek)
            stream_p.contents.Read = IGR_Stream_Read(FPtr_IGR_Stream_Read)
            stream_p.contents.Write = IGR_Stream_Write(FPtr_IGR_Stream_Write)
            stream_p.contents.Context = self._RegisterStream((stream, stream_p.contents, payload))

            if _IsIgrStream(stream) and not allowWrites:
                result = ctypes.POINTER(IGR_Stream)()

                DocumentFiltersBase._Call(api.IGR_Extend_Stream, \
                    stream_p,
                    FPtr_IGR_Stream_Action2, \
                    stream_p, \
                    ctypes.byref(result))
                return result
            else:
                return stream_p

_stream_manager = _StreamBridge_Manager()

class IGRException(RuntimeError):
    def __init__(self, message, id = 4):
        RuntimeError.__init__(self, { 'errorCode' : id, 'errorMsg' : message})

class DocumentFiltersBase(object):
    __api = None

    def _APIPrepare(self, dll_path):
        if self.__api == None:
            self.__api = ISYS11dfAPI.Singleton(dll_path)
        return self.__api

    @property
    def API(self):
        return self._APIPrepare(None)

    @staticmethod
    def _Call(method, *args):
        ecb = Error_Control_Block()
        a1 = args[:] + ( ctypes.byref(ecb), )
        res = method(*a1)
        return DocumentFiltersBase._Check(res, ecb)

    @staticmethod
    def _Check(errorCode, ecb):
        msg = ecb.Msg.decode('utf8')
        if errorCode != IGR_OK and errorCode != IGR_NO_MORE:
            if msg:
                raise IGRException(msg, errorCode)
            else:
                raise IGRException("Unknown Error: " + str(errorCode), errorCode)
        if ecb.Msg.decode('utf8') != "":
            raise IGRException(msg)
        return errorCode

    @staticmethod
    def _ToUTF16(value):
        # return the bytes minus the byte-order-marker
        result = value.encode('utf-16')[2:]
        # pad to include a null terminator
        result += b'\x00\x00\x00\x00'
        return result

    @staticmethod
    def _UTF16FindNull(data, size = None):
        j = 0
        for i in range(0, size):
            if data[j] == b'\x00' and data[j+1] == b'\x00':
                return i
            j += 2
        return size

    @staticmethod
    def _FromUTF16(data, size = None):
        if size == None:
            size = len(data) // 2
            data = ctypes.cast(data, ctypes.POINTER(ctypes.c_char))
        s = DocumentFiltersBase._UTF16FindNull(data, size)
        return data[:s*2].decode('utf-16')

    @staticmethod
    def _ToUTF16Array(data, value):
        v = value.encode('utf-16')[2:]

        d = ctypes.cast(data, ctypes.POINTER(ctypes.c_ubyte))
        d_len = len(data) * 2

        l = min(d_len-2, len(v))
        for i in range(0, l-1):
            d[i] = v[i]
        # Null Terminate
        d[l+1] = 0
        d[l+2] = 0

    @staticmethod
    def _UTF16Buffer(size):
        return ctypes.create_string_buffer(size * 2)

    @staticmethod
    def _FileTimeToDateTime(val):
        if val > 0:
            epoch = datetime.datetime(1601, 1, 1)
            return epoch + datetime.timedelta(microseconds=val // 10)
        return None

    class _EnumeratorOf:
        def __init__(self, first, next):
            self._first = first
            self._next = next

        def __iter__(self):
            return self.Iterator(self)

        def ToList(self):
            result = []
            for i in self:
                result.append(i)
            return result

        class Iterator:
            def __init__(self, parent):
                self._first = parent._first
                self._next = parent._next
                self._started = False
                self._current = None
            def __next__(self):
                if not self._started:
                    self._current = self._first()
                    self._started = True
                else:
                    self._current = self._next(self._current)
                if self._current != None:
                    return self._current
                raise StopIteration

            def next(self):
                return self.__next__()

    class _ListOf:
        def __init__(self, count, getter):
            self._count = count
            self._getter = getter

        def ToList(self):
            return self

        @property
        def Count(self): return self._count()

        def Item(self, index): return self._getter(index)

        def __getitem__(self, key): return self.Item(key)

        def __iter__(self): return self.Iterator(self)

        class Iterator:
            def __init__(self, parent):
                self._parent = parent
                self._index = 0
            def __next__(self):
                if (self._index < self._parent.Count):
                    result = self._parent.Item(self._index)
                    self._index += 1
                    return result
                else:
                    raise StopIteration
            def next(self):
                return self.__next__()


class StreamBridge:
    @staticmethod
    def Make(stream, forWrite = False):
        return _stream_manager.MakeStreamBridge(stream, forWrite)

class IGRStream:
    class IGRStream_Data:
        def __init__(self):
            self.data = io.BytesIO()

        def write(self, data):
            self.data.write(data)
            return len(data)

        def to_bytes(self):
            self.data.seek(0, 0)
            return self.data.read()

    def read(self, size):
        if size == None or size == -1:
            size = self._get_size() - self._get_pos()

        data = IGRStream.IGRStream_Data()
        self.Read(size, data)
        return data.to_bytes()

    def write(self, bytes):
        return self.Write(bytes)

    def seek(self, offset, whence):
        self.Seek(offset, whence)

    def tell(self):
        return self._get_pos()

    def _get_pos(self):
        return self.Seek(0, 1)

    def _get_size(self):
        c = self.Seek(0, 1)
        res = self.Seek(0, 2)
        self.Seek(c, 0)
        return res

    def Read(self, size, dest):
        raise NotImplementedError()
    def Write(self, bytes):
        raise NotImplementedError()
    def Seek(self, offset, origin):
        raise NotImplementedError()
    def GetStreamPart(self, partName, partFullName, partIndex):
        raise NotImplementedError()

class DocumentFilters(DocumentFiltersBase):
    class Format(DocumentFiltersBase):
        @property
        def DisplayName(self): return self._name

        @property
        def ShortName(self): return self._shortname

        @property
        def ConfigName(self): return self._configname

        @property
        def MimeType(self): return self._mimetype

        @property
        def Class(self): return self._class

        @property
        def Id(self): return self._id

        @property
        def FileTypeCategory(self): return self._fileTypeCategory

        def __init__(self, index):
            self._name = DocumentFilters.Format.Fetch(self.API, index, 0)
            self._shortname = DocumentFilters.Format.Fetch(self.API, index, 1)
            self._configname = DocumentFilters.Format.Fetch(self.API, index, 2)
            self._class = DocumentFilters.Format.Fetch(self.API, index, 3)
            self._mimetype = DocumentFilters.Format.Fetch(self.API, index, 5)
            self._id = index
            try:
                self._fileTypeCategory = DocumentFilters.Format.Fetch(self.API, index, 6)
            except ValueError:
                self._fileTypeCategory = 0

        def __repr__(self):
            return str(self.__dict__)

        @staticmethod
        def Fetch(API, index, what, maxLength = 4096):
            try:
                result = ctypes.create_string_buffer(maxLength)
                if DocumentFiltersBase._Call(API.IGR_Get_Format_Attribute, index, what, result) == 0:
                    return result.value.decode('ascii')
            except Exception:
                return None

        @staticmethod
        def GetAll():
            result = []
            for i in range(1024):
                item = DocumentFilters.Format(i)
                if item.DisplayName != None:
                    result.append(item)
            return result

    class Option(DocumentFiltersBase):

        @property
        def DisplayName(self): return self._name

        @property
        def Description(self): return self._description

        def __init__(self, index):
            self._name = DocumentFilters.Option.Fetch(self.API, index, 0)
            self._description = DocumentFilters.Option.Fetch(self.API, index, 1)

        def __repr__(self):
            d = dict()
            d['Name'] = self._name
            d['Description'] = self.Description
            return str(d)

        @staticmethod
        def Fetch(API, index, what, maxLength = 4096):
            try:
                result = ctypes.create_string_buffer(maxLength)
                if DocumentFiltersBase._Call(API.IGR_Get_Option_Attribute, index, what, result, maxLength) == 0:
                    return result.value.decode('ascii')
            except:
                return None

        @staticmethod
        def GetAll():
            result = []
            for i in range(1024*64):
                item = DocumentFilters.Option(i)
                if item.DisplayName != None:
                    result.append(item)
                else:
                    break
            return result

    class Hyperlink(DocumentFiltersBase):
        def __init__(self, item):
            self._item = item

        def GetHyperlinkType(self): return self._item.type
        def GetRef(self): return self._getstr(IGR_HYPERLINK_GET_VALUE_REF)
        def GetUri(self): return self._getstr(IGR_HYPERLINK_GET_VALUE_URI)
        def GetX(self): return self._item.x
        def GetY(self): return self._item.y
        def GetWidth(self): return self._item.width
        def GetHeight(self): return self._item.height
        def GetPageNumber(self): return self._item.page_number
        def GetDestFit(self): return self._item.dest_fit
        def GetDestLeft(self): return self._item.dest_left
        def GetDestTop(self): return self._item.dest_top
        def GetDestRight(self): return self._item.dest_right
        def GetDestBottom(self): return self._item.dest_bottom
        def GetDestZoom(self): return self._item.dest_zoom
        def GetFlags(self): return self._item.flags
        def GetActionType(self): return self._item.type

        @property
        def X(self): return self._item.x

        @property
        def Y(self): return self._item.y

        @property
        def Width(self): return self._item.width

        @property
        def Height(self): return self._item.height

        @property
        def PageNumber(self): return self._item.page_number

        @property
        def DestFit(self): return self._item.dest_fit

        @property
        def DestLeft(self): return self._item.dest_left

        @property
        def DestTop(self): return self._item.dest_top

        @property
        def DestRight(self): return self._item.dest_right

        @property
        def DestBottom(self): return self._item.dest_bottom

        @property
        def DestZoom(self): return self._item.dest_zoom

        @property
        def Flags(self): return self._item.flags

        @property
        def Ref(self): return self._getstr(IGR_HYPERLINK_GET_VALUE_REF)

        @property
        def Uri(self): return self._getstr(IGR_HYPERLINK_GET_VALUE_URI)

        def _getstr(self, id, max_length = 4096):
            data = ctypes.create_string_buffer(max_length * 2)
            DocumentFiltersBase._Call(self.API.IGR_Get_Page_Hyperlink_Str, ctypes.byref(self._item), id, max_length, ctypes.byref(data))
            return DocumentFiltersBase._FromUTF16(data, max_length)

        @staticmethod
        def Make(pageHandle):
            API = ISYS11dfAPI.Singleton()
            result = []
            count = IGR_LONG(0)
            DocumentFiltersBase._Call(API.IGR_Get_Page_Hyperlink_Count, pageHandle, ctypes.byref(count))
            for i in range(count.value):
                c = IGR_LONG(1)
                e = IGR_Hyperlink()
                DocumentFiltersBase._Call(API.IGR_Get_Page_Hyperlinks, pageHandle, i, ctypes.byref(c), ctypes.byref(e))
                result.append(DocumentFilters.Hyperlink(e))
            return result

    class FormElement(DocumentFiltersBase):

        class FormElementOption:
            def __init__(self, name, value, selected):
                self._name = name
                self._value = value
                self._selected = selected

            @property
            def Name(self): return self._name

            @property
            def Value(self): return self._value

            @property
            def Selected(self): return self._selected

        def __init__(self, item):
            self._item = item
            self._optionIndex = 0
            self._options = []

            for i in range(item.option_count):
                name = self._getOptionStr(IGR_PAGE_FORM_ELEMENT_GET_NAME, i)
                value = self._getOptionStr(IGR_PAGE_FORM_ELEMENT_GET_VALUE, i)
                selected = self._getOptionStr(IGR_PAGE_FORM_ELEMENT_GET_SELECTED, i) == '1'

                self._options.append(DocumentFilters.FormElement.FormElementOption(name, value, selected))


        def GetValue(self, maxLength = 4096): return self._getStr(IGR_PAGE_FORM_ELEMENT_GET_VALUE, maxLength)

        def GetName(self): return DocumentFiltersBase._FromUTF16(self._item.name)
        def GetFlags(self): return self._item.flags
        def GetCaption(self): return self._getStr(IGR_PAGE_FORM_ELEMENT_GET_CAPTION)
        def GetFontName(self): return self._getStr(IGR_PAGE_FORM_ELEMENT_GET_FONT_NAME)
        def GetAction(self): return self._getStr(IGR_PAGE_FORM_ELEMENT_GET_ACTION)
        def GetActionDest(self): return self._getStr(IGR_PAGE_FORM_ELEMENT_GET_ACTION_DEST)
        def GetX(self): return self._item.x
        def GetY(self): return self._item.y
        def GetWidth(self): return self._item.width
        def GetHeight(self): return self._item.height
        def GetFontSize(self): return self._item.fontSize
        def GetSelected(self): return self._item.selectedItem
        def GetChecked(self): return (self.Flags & IGR_PAGE_FORM_ELEMENT_FLAG_CHECKED) != 0
        def GetElementType(self): return self._item.type
        def GetAlignment(self): return self._item.textAlignment
        def GetRotation(self): return self._item.rotation

        def GetOption(self, index): return self._options[index] if index < len(self._options) else None

        def GetFirstOption(self):
            self._optionIndex = 0
            return self.GetNextOption()

        def GetNextOption(self):
            result = self.GetOption(self._optionIndex)
            self._optionIndex += 1
            return result

        @property
        def Name(self): return self.GetName()

        @property
        def Value(self): return self.GetValue()

        @property
        def Caption(self): return self.GetCaption()

        @property
        def FontName(self): return self.GetFontName()

        @property
        def Action(self): return self.GetAction()

        @property
        def ActionDest(self): return self.GetActionDest()

        @property
        def X(self): return self.GetX()

        @property
        def Y(self): return self.GetY()

        @property
        def Width(self): return self.GetWidth()

        @property
        def Height(self): return self.GetHeight()

        @property
        def Flags(self): return self.GetFlags()

        @property
        def FontSize(self): return self.GetFontSize()

        @property
        def Selected(self): return self.GetSelected()

        @property
        def Checked(self): return self.GetChecked()

        @property
        def FontSize(self): return self.GetFontSize()

        @property
        def ElementType(self): return self.GetElementType()

        @property
        def FirstOption(self): return self.GetFirstOption()

        @property
        def NextOption(self): return self.GetNextOption()

        @property
        def Alignment(self): return self.GetAlignment()

        @property
        def Rotation(self): return self.GetRotation()

        @property
        def OptionCount(self): return len(self._options)

        @property
        def Options(self): return self._options

        def _getStr(self, v, maxLength = 4096):
            buffer = DocumentFiltersBase._UTF16Buffer(maxLength)
            DocumentFiltersBase._Call(self.API.IGR_Get_Page_Form_Element_Str, ctypes.byref(self._item), v, maxLength, ctypes.byref(buffer))
            return DocumentFiltersBase._FromUTF16(buffer, maxLength)

        def _getOptionStr(self, type, index, maxLength = 4096):
            buffer = DocumentFiltersBase._UTF16Buffer(maxLength)
            DocumentFiltersBase._Call(self.API.IGR_Get_Page_Form_Element_Option_Str, ctypes.byref(self._item), type, index, maxLength, ctypes.byref(buffer))
            return DocumentFiltersBase._FromUTF16(buffer, maxLength)


        @staticmethod
        def Make(pageHandle):
            API = ISYS11dfAPI.Singleton()
            result = []
            count = IGR_LONG(0)
            DocumentFiltersBase._Call(API.IGR_Get_Page_Form_Element_Count, pageHandle, ctypes.byref(count))
            for i in range(count.value):
                c = IGR_LONG(1)
                e = IGR_Page_Form_Element()
                DocumentFiltersBase._Call(API.IGR_Get_Page_Form_Elements, pageHandle, i, ctypes.byref(c), ctypes.byref(e))
                result.append(DocumentFilters.FormElement(e))
            return result

    class Word(DocumentFiltersBase):
        def __init__(self, item, index):
            self._item = item
            self._index = index

        def GetText(self): return DocumentFiltersBase._FromUTF16(ctypes.cast(self._item.word, ctypes.POINTER(ctypes.c_char)), self.Length)
        def GetIndex(self): return self._index
        def GetWordIndex(self): return self.GetIndex()
        def GetX(self): return self._item.x
        def GetY(self): return self._item.y
        def GetWidth(self):  return self._item.width
        def GetHeight(self): return self._item.height
        def GetStyle(self): return self._item.style
        def GetLength(self): return self._item.wordLength
        def GetCharacterOffset(self): return self._item.charoffset

        @property
        def Index(self): return self._index

        @property
        def X(self): return self._item.x

        @property
        def Y(self): return self._item.y

        @property
        def Width(self): return self._item.width

        @property
        def Height(self): return self._item.height

        @property
        def Style(self): return self._item.style

        @property
        def Length(self): return self._item.wordLength

        @property
        def CharacterOffset(self): return self._item.charoffset

        @property
        def Text(self): return self.GetText()

    class Bookmark(DocumentFiltersBase):
        def __init__(self, docHandle = None, item = None, title=None,
            destination=None, fit=None, action=None, color = None, level = None,
            zoom = None, x = None, y = None, width = None, height = None,
            textStyle = None, pageIndex = None, children = None):
            self._children = []
            self._docHandle = docHandle
            self._item = item
            if self._item == None:
                self._item = IGR_Bookmark()

                if title is not None: self.Title = title
                if destination is not None: self.Destination = destination
                if fit is not None: self.Fit = fit
                if action is not None: self.Action = action
                if color is not None: self.Color = color
                if level is not None: self.Level = level
                if zoom is not None: self.Zoom = zoom
                if x is not None: self.X = x
                if y is not None: self.Y = y
                if width is not None: self.Width = width
                if height is not None: self.Height = height
                if textStyle is not None: self.TextStyle = textStyle
                if pageIndex is not None: self.PageIndex = pageIndex
                if children is not None and len(children) > 0: self._children.extend(children)

        def __iter__(self):
            return self.Children.__iter__()

        def GetTitle(self):  return DocumentFiltersBase._FromUTF16(self._item.title)
        def SetTitle(self, value): DocumentFiltersBase._ToUTF16Array(self._item.title, value)
        def GetDestination(self): return DocumentFiltersBase._FromUTF16(self._item.dest)
        def SetDestination(self, value):  DocumentFiltersBase._ToUTF16Array(self._item.dest, value)
        def GetFit(self): return self._item.fit
        def SetFit(self, value): self._item.fit = value
        def GetAction(self): return self._item.action
        def SetAction(self, value): self._item.action = value
        def GetColor(self): return self._item.color
        def SetColor(self, value): self._item.color = value
        def GetLevel(self): return self._item.level
        def SetLevel(self, value): self._item.level = value
        def GetZoom(self): return self._item.zoom
        def SetZoom(self, value): self._item.zoom = value
        def GetRect(self): return self._item.x, self._item.y, self._item.width, self._item.height
        def GetWidth(self): return self._item.width
        def SetWidth(self, value): self._item.width = value
        def GetHeight(self): return self._item.height
        def SetHeight(self, value): self._item.height = value
        def GetX(self): return self._item.x
        def SetX(self, value): self._item.x = value
        def GetY(self): return self._item.y
        def SetY(self, value): self._item.y = value
        def GetTextStyle(self): return self._item.text_style
        def SetTextStyle(self, value): self._item.text_style = value
        def GetPageIndex(self):
            try:
                return int(self.Destination)
            except:
                return -1
        def SetPageIndex(self, value): self.SetDestination(str(value))

        def GetFirstChild(self):
            if self._docHandle == None: return None
            res = IGR_Bookmark()
            if DocumentFiltersBase._Call(self.API.IGR_Get_Bookmark_First_Child, self._docHandle, ctypes.byref(self._item), ctypes.byref(res)) == 0:
                return DocumentFilters.Bookmark(self._docHandle, res)
            else:
                return None

        def GetNextSibling(self):
            if self._docHandle == None: return None
            res = IGR_Bookmark()
            if DocumentFiltersBase._Call(self.API.IGR_Get_Bookmark_Next_Sibling, self._docHandle, ctypes.byref(self._item), ctypes.byref(res)) == 0:
                return DocumentFilters.Bookmark(self._docHandle, res)
            else:
                return None

        def GetChildren(self):
            if (self._docHandle == None): return self._children

            first = lambda : self.FirstChild
            next = lambda current : current.NextSibling
            return DocumentFiltersBase._EnumeratorOf(first, next)

        def AddChild(self, child):
            child.Level = self.Level + 1
            self._children.append(child)

        @property
        def Title(self): return self.GetTitle()

        @Title.setter
        def Title(self, value): self.SetTitle(value)

        @property
        def Destination(self): return self.GetDestination()

        @Destination.setter
        def Destination(self, value): self.SetDestination(value)

        @property
        def Fit(self): return self.GetFit()

        @Fit.setter
        def Fit(self, value): self.SetFit(value)

        @property
        def Action(self): return self.GetAction()

        @Action.setter
        def Action(self, value): self.SetAction(value)

        @property
        def Color(self): return self.GetColor()

        @Color.setter
        def Color(self, value): self.SetColor(value)

        @property
        def Level(self): return self.GetLevel()

        @Level.setter
        def Level(self, value): self.SetLevel(value)

        @property
        def Zoom(self): return self.GetZoom()

        @Zoom.setter
        def Zoom(self, value): self.SetZoom(value)

        @property
        def PageIndex(self): return self.GetPageIndex()

        @PageIndex.setter
        def PageIndex(self, value): self.SetDestination(str(value))

        @property
        def Rect(self): return self.GetRect()

        @property
        def FirstChild(self): return self.GetFirstChild()

        @property
        def NextSibling(self): return self.GetNextSibling()

        @property
        def Children(self): return self.GetChildren()

        @property
        def Width(self): return self._item.width

        @Width.setter
        def Width(self, value): self.SetWidth(value)

        @property
        def Height(self): return self._item.height

        @Height.setter
        def Height(self, value): self.SetHeight(value)

        @property
        def X(self): return self._item.x

        @X.setter
        def X(self, value): self.SetX(value)

        @property
        def Y(self): return self._item.y

        @Y.setter
        def Y(self, value): self.SetY(value)

        @property
        def TextStyle(self): return self._item.text_style

        @TextStyle.setter
        def TextStyle(self, value): self.SetTextStyle(value)

        @property
        def Children(self): return self.GetChildren()

        def _packBookmark(self):
            return self._item


    class Page(DocumentFiltersBase):
        def __init__(self, docHandle, pageIndex):
            self._docHandle = docHandle
            self._pageIndex = pageIndex
            self._pageHandle = IGR_LONG(0)
            self._words = None
            self._pageText = None
            self._wordIndex = 0
            self._formElements = None
            self._imageEnumerator = None
            self._hyperlinks = None
            self._hyperlinkIter = None
            self._annotations = None
            self._annotationIndex = 0

            DocumentFiltersBase._Call(self.API.IGR_Open_Page, docHandle, pageIndex, ctypes.byref(self._pageHandle))

        def __enter__(self):
            return self

        def __exit__(self, exception_type, exception_value, traceback):
            self.Close()

        def Close(self):
            if (self._pageHandle.value > 0):
                DocumentFiltersBase._Call(self.API.IGR_Close_Page, self._pageHandle)
                self._pageHandle = IGR_LONG(0)

        def GetWordCount(self):
            result = IGR_LONG(0)
            DocumentFiltersBase._Call(self.API.IGR_Get_Page_Word_Count, self._pageHandle, ctypes.byref(result))
            return result.value

        def GetFirstWord(self):
            self._wordIndex = 0
            return self.GetNextWord()

        def GetNextWord(self):
            if self._wordIndex < self.WordCount:
                result = self.GetWord(self._wordIndex)
                self._wordIndex += 1
                return result
            return None

        def GetWord(self, index):
            words = self._needWords()
            if index < 0 or index > self.WordCount:
                raise IndexError('page index out of bounds')
            return DocumentFilters.Word(words[index], index)

        def GetDimensions(self):
            width = IGR_LONG(0)
            height = IGR_LONG(0)
            DocumentFiltersBase._Call(self.API.IGR_Get_Page_Dimensions, self._pageHandle, ctypes.byref(width), ctypes.byref(height))
            return width.value, height.value

        def GetWidth(self):
            width, _1 = self.GetDimensions()
            return width

        def GetHeight(self):
            _1, height = self.GetDimensions()
            return height

        def GetText(self):
            if self._pageText == None:
                maxLength = 4096
                data = ctypes.create_string_buffer(maxLength * 2)
                size = IGR_LONG(maxLength)
                self._pageText = ""
                while DocumentFiltersBase._Call(self.API.IGR_Get_Page_Text, self._pageHandle, ctypes.byref(data), ctypes.byref(size)) == 0:
                    self._pageText += DocumentFiltersBase._FromUTF16(data, size.value)

            return self._pageText

        def _needWords(self):
            if self._words == None:
                count = self.GetWordCount()
                self._words = []
                for i in range(count):
                    item = IGR_Page_Word()
                    c = IGR_LONG(1)
                    if DocumentFiltersBase._Call(self.API.IGR_Get_Page_Words, self._pageHandle, i, ctypes.byref(c), ctypes.byref(item)) == 0:
                        self._words.append(item)
            return self._words

        def _needFormElements(self):
            if self._formElements == None:
                self._formElements = DocumentFilters.FormElement.Make(self.Handle)
            return self._formElements

        def _needHyperlinks(self):
            if self._hyperlinks == None:
                self._hyperlinks = DocumentFilters.Hyperlink.Make(self.Handle)

            return self._hyperlinks

        def _needAnnotations(self):
            if self._annotations == None:
                self._annotations = DocumentFilters.Annotations.Fetch(self._pageHandle)

            return self._annotations

        def Redact(self, firstWord, lastWord):
            if isinstance(firstWord, DocumentFilters.Word):
                firstWord = firstWord.Index
            if isinstance(lastWord, DocumentFilters.Word):
                lastWord = lastWord.Index

            DocumentFiltersBase._Call(self.API.IGR_Redact_Page_Text, self._pageHandle, firstWord, lastWord, 0)

        def GetFirstImage(self):
            if self._imageEnumerator == None:
                self._imageEnumerator = HSUBFILES()
                DocumentFiltersBase._Call(self.API.IGR_Get_Page_Images_Enumerator, self._pageHandle, ctypes.byref(self._imageEnumerator))
            else:
                DocumentFiltersBase._Call(self.API.IGR_Subfiles_Reset, self._imageEnumerator)
            return self.GetNextImage()

        def GetNextImage(self):
            if self._imageEnumerator == None:
                return self.GetFirstImage()
            return DocumentFilters.Extractor._NextSubFile(self._imageEnumerator, self._pageHandle, self.API.IGR_Subfiles_Next, self.API.IGR_Extract_Page_Image_Stream)

        def GetAttribute(self, name):
            size = IGR_LONG(1024)
            buffer = DocumentFiltersBase._UTF16Buffer(size.value)
            DocumentFiltersBase._Call(self.API.IGR_Get_Page_Attribute, self._pageHandle, DocumentFiltersBase._ToUTF16(name), ctypes.byref(buffer), ctypes.byref(size))
            return DocumentFiltersBase._FromUTF16(buffer, size.value)

        def GetFirstHyperlink(self):
            self._hyperlinkIter = iter(self.Hyperlinks)
            return self.GetNextHyperlink()

        def GetNextHyperlink(self):
            try:
                return next(self._hyperlinkIter)
            except:
                return None

        def GetFirstFormElement(self):
            self._formElementIter = iter(self.FormElements)
            return self.GetNextFormElement()

        def GetNextFormElement(self):
            try:
                return next(self._formElementIter)
            except:
                return None

        def GetAnnotationCount(self):
            res = IGR_LONG(0)
            DocumentFiltersBase._Call(self.API.IGR_Get_Page_Annotation_Count, self._pageHandle, ctypes.byref(res))
            return res.value

        def GetAnnotation(self, index): return self.Annotations[index] if index < len(self.Annotations) else None

        def GetFirstAnnotation(self):
            self._annotationIndex = 0
            return self.GetNextAnnotation()

        def GetNextAnnotation(self):
            result = self.GetAnnotation(self._annotationIndex)
            self._annotationIndex += 1
            return result

        def GetPageBitmap(self, sourceRect = None, destSize = None, options = ""):
            from PIL import Image, ImageFile, ImagePalette

            pixels = IGR_Page_Pixels()
            if sourceRect is None:
                sourceRect = IGR_Rect()
                sourceRect.right = self.Width
                sourceRect.bottom = self.Height
            if destSize is None:
                destSize = IGR_Size()
                destSize.width = self.Width
                destSize.height = self.Height

            DocumentFilters._Call(self.API.IGR_Get_Page_Pixels, 
                self._pageHandle, 
                ctypes.byref(sourceRect), 
                ctypes.byref(destSize), 
                IGR_ULONG(0),
                DocumentFiltersBase._ToUTF16(options),
                IGR_ULONG(IGR_OPEN_BITMAP_PIXEL_AUTO),
                ctypes.byref(pixels))
            try:
                imageSize = pixels.stride * pixels.height

                pix = ctypes.cast(pixels.scanline0, ctypes.POINTER(ctypes.c_char * imageSize)).contents
                pal = ctypes.cast(pixels.palette, ctypes.POINTER(IGR_ULONG * pixels.palette_count)).contents

                if pixels.pixel_format == IGR_OPEN_BITMAP_PIXEL_1BPP_INDEXED: 
                    res = Image.frombytes("1", (pixels.width, pixels.height), pix, "raw")
                elif pixels.pixel_format == IGR_OPEN_BITMAP_PIXEL_8BPP_INDEXED:
                    res = Image.frombytes("P", (pixels.width, pixels.height), pix, "raw")
                    newpal = []
                    for i in pal:
                        newpal.append((i & 0xff0000) >> 16)
                        newpal.append((i & 0x00ff00) >> 8)
                        newpal.append((i & 0x0000ff))

                    res.putpalette(newpal)
                else:
                    res = Image.frombytes("RGB", (pixels.width, pixels.height), pix, "raw", "BGRX")
            finally:
                DocumentFiltersBase._Call(self.API.IGR_Free_Page_Pixels, ctypes.byref(pixels))
            return res
        
        def Compare(self, otherPage, margins = None, leftMargins = None, rightMargins = None, compareSettings = None):
            if otherPage is None:
                raise IGRException("otherPage cannot be None", 4)
            
            res = IGR_HTEXTCOMPARE()
            leftHandle = self.Handle
            rightHandle = otherPage.Handle

            wrappedSettings = DocumentFilters.CompareSettings()
            if isinstance(compareSettings, DocumentFilters.CompareSettings):
                wrappedSettings = compareSettings;
            
            if not isinstance(leftMargins, IGR_FRect):
                leftMargins = margins
            if not isinstance(rightMargins, IGR_FRect):
                rightMargins = margins
            if leftMargins is None:
                leftMargins = IGR_FRect()
            if rightMargins is None:
                rightMargins = IGR_FRect()

            DocumentFiltersBase._Call(self.API.IGR_Text_Compare_Pages, leftHandle, ctypes.byref(leftMargins), rightHandle, ctypes.byref(rightMargins), ctypes.byref(wrappedSettings._item), ctypes.byref(res))
            return DocumentFilters.CompareResults(res)


        @property
        def PageIndex(self): return self._pageIndex

        @property
        def Width(self): return self.GetWidth()

        @property
        def Height(self): return self.GetHeight()

        @property
        def WordCount(self): return self.GetWordCount()

        @property
        def FirstWord(self): return self.GetFirstWord()

        @property
        def NextWord(self): return self.GetNextWord()

        @property
        def Words(self): return [ DocumentFilters.Word(x, i) for i,x in enumerate(self._needWords()) ]

        @property
        def Images(self): return DocumentFilters.Extractor.SubFileEnumerator(self._pageHandle, self.API.IGR_Get_Page_Images_Enumerator, self.API.IGR_Extract_Page_Image_Stream)

        @property
        def Text(self): return self.GetText()

        @property
        def Handle(self): return self._pageHandle

        @property
        def FormElements(self): return self._needFormElements()

        @property
        def Hyperlinks(self): return self._needHyperlinks()

        @property
        def Annotations(self): return self._needAnnotations()

        @property
        def AnnotationCount(self): return self.GetAnnotationCount()

    class Annotations:
        class _Serializer:
            class Months(Enum):
                Jan = 1
                Feb = 2
                Mar = 3
                Apr = 4
                May = 5
                Jun = 6
                Jul = 7
                Aug = 8
                Sep = 9
                Oct = 10
                Nov = 11
                Dec = 12
            class Days(Enum):
                Mon = 0
                Tue = 1
                Wed = 2
                Thu = 3
                Fri = 4
                Sat = 5
                Sun = 6

            @staticmethod
            def Get(dict, name, default = None):
                try:
                    return dict[name] if dict is not None else default
                except KeyError:
                    return default

            @staticmethod
            def GetDate(dict, name, default = None):
                # 'Wed, 18 Jan 2017 14:21:29 GMT'
                val = DocumentFilters.Annotations._Serializer.Get(dict, name, default)
                if val is not None:
                    parts = [s.strip() for s in re.split(",| |:", val)]

                    if len(parts) == 9:
                        return datetime.datetime(int(parts[4]), DocumentFilters.Annotations._Serializer.Months[parts[3]].value, int(parts[2]), int(parts[5]), int(parts[6]), int(parts[7]))
                    else:
                        return None

            @staticmethod
            def DateToStr(date: datetime.datetime):
                # 'Wed, 18 Jan 2017 14:21:29 GMT'
                months = DocumentFilters.Annotations._Serializer.Months
                days = DocumentFilters.Annotations._Serializer.Days
                dow = days(date.weekday()).name

                return f"{dow}, {date.day} {months(date.month).name} {date.year} {date.hour:02}:{date.minute:02}:{date.second:02} GMT"

            @staticmethod
            def Has(dict, name):
                return dict is not None and name in dict.keys()

            @staticmethod
            def GetEnum(dict, name, type, default = None):
                if dict is None: return default
                try:
                    val = str(dict[name])
                    try:
                        if val.isnumeric():
                            return type(int(val))
                        else:
                            return type[val]
                    except KeyError:
                        for k in type:
                            if k.name.lower() == val.lower():
                                return k
                        return default
                except KeyError:
                    return default

            @staticmethod
            def GetObj(dict, name: str, type, default = None):
                val = DocumentFilters.Annotations._Serializer.Get(dict, name, None)
                if val is not None:
                    return type(val)
                return default

        class Type(Enum):
            Unknown = IGR_ANNOTATION_UNKNOWN
            Text = IGR_ANNOTATION_TEXT
            Link = IGR_ANNOTATION_LINK
            Freetext = IGR_ANNOTATION_FREETEXT
            Line = IGR_ANNOTATION_LINE
            Rectangle = IGR_ANNOTATION_SQUARE
            Ellipse = IGR_ANNOTATION_CIRCLE
            Polygon = IGR_ANNOTATION_POLYGON
            Polyline = IGR_ANNOTATION_POLYLINE
            Highlight = IGR_ANNOTATION_HIGHLIGHT
            Underline = IGR_ANNOTATION_UNDERLINE
            Squiggly = IGR_ANNOTATION_SQUIGGLE
            Strikeout = IGR_ANNOTATION_STRIKEOUT
            Caret = IGR_ANNOTATION_CARET
            Ink = IGR_ANNOTATION_INK
            Stamp = IGR_ANNOTATION_STAMP
            Popup = IGR_ANNOTATION_POPUP
            Barcode = IGR_ANNOTATION_BARCODE
            NamedDestination = IGR_ANNOTATION_NAMED_DESTINATION
        class BorderStyleType(Enum):
            Unknown = IGR_ANNOTATION_BORDER_STYLE_UNKNOWN
            Solid = IGR_ANNOTATION_BORDER_STYLE_SOLID
            Dashed = IGR_ANNOTATION_BORDER_STYLE_DASHED
            Beveled = IGR_ANNOTATION_BORDER_STYLE_BEVELED
            Inset = IGR_ANNOTATION_BORDER_STYLE_INSET
            Underline = IGR_ANNOTATION_BORDER_STYLE_UNDERLINE
        class LineEndingType(Enum):
            None_ = IGR_ANNOTATION_LINE_ENDING_NONE
            Square = IGR_ANNOTATION_LINE_ENDING_SQUARE
            Circle = IGR_ANNOTATION_LINE_ENDING_CIRCLE
            Diamond = IGR_ANNOTATION_LINE_ENDING_DIAMOND
            OpenArrow = IGR_ANNOTATION_LINE_ENDING_OPENARROW
            ClosedArrow = IGR_ANNOTATION_LINE_ENDING_CLOSEDARROW
            Butt = IGR_ANNOTATION_LINE_ENDING_BUTT
            RightOpenArrow = IGR_ANNOTATION_LINE_ENDING_RIGHTOPENARROW
            RightClosedArrow = IGR_ANNOTATION_LINE_ENDING_RIGHTCLOSEDARROW
            Slash = IGR_ANNOTATION_LINE_ENDING_SLASH
        class AlignmentType(Enum):
            Left = IGR_ANNOTATION_ALIGN_LEFT
            Center = IGR_ANNOTATION_ALIGN_CENTER
            Right = IGR_ANNOTATION_ALIGN_RIGHT
        class Flags:
            Invisible = IGR_ANNOTATION_FLAGS_INVISIBLE
            Hidden = IGR_ANNOTATION_FLAGS_HIDDEN
            Print = IGR_ANNOTATION_FLAGS_PRINT
            NoZoom = IGR_ANNOTATION_FLAGS_NOZOOM
            NoRotate = IGR_ANNOTATION_FLAGS_NOROTATE
            NoView = IGR_ANNOTATION_FLAGS_NOVIEW
            ReadOnly = IGR_ANNOTATION_FLAGS_READONLY
            Locked = IGR_ANNOTATION_FLAGS_LOCKED
            ToggleNoView = IGR_ANNOTATION_FLAGS_TOGGLENOVIEW
            LockedContents = IGR_ANNOTATION_FLAGS_LOCKEDCONTENTS
        class HighlightType(Enum):
            Unknown = IGR_ANNOTATION_HIGHLIGHT_UNKNOWN
            Invert = IGR_ANNOTATION_HIGHLIGHT_INVERT
            Outline = IGR_ANNOTATION_HIGHLIGHT_OUTLINE
            Push = IGR_ANNOTATION_HIGHLIGHT_PUSH

        class Rect:
            def __init__(self, left: float = 0, top: float = 0, right: float = 0, bottom: float = 0, payload = None):
                self.Left: float = DocumentFilters.Annotations._Serializer.Get(payload, 'left', left)
                self.Top: float = DocumentFilters.Annotations._Serializer.Get(payload, 'top', top)
                self.Right: float = DocumentFilters.Annotations._Serializer.Get(payload, 'right', right)
                self.Bottom: float = DocumentFilters.Annotations._Serializer.Get(payload, 'bottom', bottom)

            def GetBottom(self): return self.Bottom
            def SetBottom(self, value): self.Bottom = value
            def GetLeft(self): return self.Left
            def SetLeft(self, value): self.Left = value
            def GetRight(self): return self.Right
            def SetRight(self, value): self.Right = value
            def GetTop(self): return self.Top
            def SetTop(self, value): self.Top = value

            @property
            def Width(self) -> float: return self.Right - self.Left

            @property
            def Height(self) -> float: return self.Bottom - self.Top

            @staticmethod
            def xywh(x: float, y: float, w: float, h: float): return DocumentFilters.Annotations.Rect(x, y, x + w, y + h)

            @staticmethod
            def ltrb(l: float, t: float, r: float, b: float): return DocumentFilters.Annotations.Rect(l, t, r, b)

            @staticmethod
            def Deserialize(payload):
                return DocumentFilters.Annotations.Rect(payload=payload)

            @staticmethod
            def FromXYWH(x: float, y: float, w: float, h: float): return DocumentFilters.Annotations.Rect.xywh(x, y, w, h)

            @staticmethod
            def FromLTRB(l: float, t: float, r: float, b: float): return DocumentFilters.Annotations.Rect.ltrb(l, t, r, b)


        class Point:
            def __init__(self, x = 0, y = 0, payload = None):
                self.X = DocumentFilters.Annotations._Serializer.Get(payload, 'x', x)
                self.Y = DocumentFilters.Annotations._Serializer.Get(payload, 'y', y)

            @staticmethod
            def FromXY(x: float, y: float):
                return DocumentFilters.Annotations.Point(x, y)

            def GetX(self): return self.X
            def SetX(self, value): self.X = value
            def GetY(self): return self.Y
            def SetY(self, value): self.Y = value

        class AppearanceStream:
            def __init__(self, payload = None):
                self.ContentType = DocumentFilters.Annotations._Serializer.Get(payload, 'contentType')
                self.Encoding = DocumentFilters.Annotations._Serializer.Get(payload, 'encoding', "Base64")
                self.Content = DocumentFilters.Annotations._Serializer.Get(payload, 'content')

            @staticmethod
            def FromFile(filename: str, contentType: str):
                ret = DocumentFilters.Annotations.AppearanceStream()
                ret.ContentType = contentType
                ret.Encoding = "base64"

                with open(filename, "rb") as image_file:
                    ret.Content = base64.b64encode(image_file.read()).decode('ascii')

                return ret

            def GetContent(self): return self.Content
            def SetContent(self, value): self.Content = value
            def GetContentType(self): return self.ContentType
            def SetContentType(self, value): self.ContentType = value
            def GetEncoding(self): return self.Encoding
            def SetEncoding(self, value): self.Encoding = value

        class AppearanceStreams:
            def __init__(self, payload = None):
                self.Normal = DocumentFilters.Annotations._Serializer.GetObj(payload, 'normal', DocumentFilters.Annotations.AppearanceStream, None)
                self.RollOver = DocumentFilters.Annotations._Serializer.GetObj(payload, 'rollOver', DocumentFilters.Annotations.AppearanceStream, None)
                self.Down = DocumentFilters.Annotations._Serializer.GetObj(payload, 'down', DocumentFilters.Annotations.AppearanceStream, None)

        class Action:
            def __init__(self, type, payload = None):
                self.Type = type
                self.NewWindow = DocumentFilters.Annotations._Serializer.Get(payload, 'newWindow')

            def GetActionType(self): return self.Type
            def SetActionType(self, value): self.Type = value
            def GetNewWindow(self): return self.NewWindow
            def SetNewWindow(self, value): self.NewWindow = value

            @property
            def ActionType(self): return self.type

        class ActionGoTo(Action):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Action.__init__(self, "GoTo", payload)
                self.Page = DocumentFilters.Annotations._Serializer.Get(payload, 'page')
                self.Rect = DocumentFilters.Annotations._Serializer.GetObj(payload, 'page', DocumentFilters.Annotations.Rect.Deserialize, None)
                self.Zoom = DocumentFilters.Annotations._Serializer.Get(payload, 'zoom') # Zoom

            def GetPage(self): return self.Page
            def SetPage(self, value): self.Page = value
            def GetRect(self): return self.Rect
            def SetRect(self, value): self.Rect = value
            def GetZoom(self): return self.Zoom
            def SetZoom(self, value): self.Zoom = value

        class ActionUri(Action):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Action.__init__(self, "URI", payload)
                self.Uri = DocumentFilters.Annotations._Serializer.Get(payload, 'uri')

            def GetUri(self): return self.Uri
            def SetUri(self, value): self.Uri = value

        class ActionGoToRemote(Action):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Action.__init__(self, "GoToR", payload)
                self.Filename = DocumentFilters.Annotations._Serializer.Get(payload, 'filename')
                self.Page = DocumentFilters.Annotations._Serializer.Get(payload, 'page')
                self.Rect = DocumentFilters.Annotations._Serializer.Get(payload, 'rect', DocumentFilters.Annotations.Rect.Deserialize, None)

            def GetFilename(self): return self.Filename
            def SetFilename(self, value): self.Filename = value
            def GetPage(self): return self.Page
            def SetPage(self, value): self.Page = value
            def GetRect(self): return self.Rect
            def SetRect(self, value): self.Rect = value

        class ActionNamed(Action):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Action.__init__(self, "Named", payload)
                self.Name = DocumentFilters.Annotations._Serializer.Get(payload, 'name')

            def GetName(self): return self.Name
            def SetName(self, value): self.Name = value

        class LineEndings:
            def __init__(self, payload = None):
                self.Begin = DocumentFilters.Annotations._Serializer.GetEnum(payload, 'begin', DocumentFilters.Annotations.LineEndingType, DocumentFilters.Annotations.LineEndingType.None_)
                self.End = DocumentFilters.Annotations._Serializer.GetEnum(payload, 'end', DocumentFilters.Annotations.LineEndingType, DocumentFilters.Annotations.LineEndingType.None_)

            def GetBegin(self): return self.Begin
            def SetBegin(self, value): self.Begin = value
            def GetEnd(self): return self.End
            def SetEnd(self, value): self.End = value

        class IHasPoints:
            def __init__(self, payload = None):
                self.Points = list()    # Point
                try:
                    self.Points = [DocumentFilters.Annotations.Point(item) for item in (payload or {}).get('points') or []]
                except KeyError:
                    pass

            def GetPoints(self): return self.Points

        class IHasLineEndings:
            def __init__(self, payload = None):
                self.LineEndings = DocumentFilters.Annotations._Serializer.GetObj(payload, 'lineEndings', DocumentFilters.Annotations.LineEndings, DocumentFilters.Annotations.LineEndings())

            def GetLineEndings(self): return self.LineEndings

        class DefaultAppearance:
            def __init__(self, payload = None):
                self.FontName = DocumentFilters.Annotations._Serializer.Get(payload, "fontName", "Helvetica")
                self.FontSize = DocumentFilters.Annotations._Serializer.Get(payload, "fontSize", 14)
                self.TextColor = DocumentFilters.Annotations._Serializer.Get(payload, "textColor", "#000000")

            def GetFontName(self): return self.FontName
            def SetFontName(self, value): self.FontName = value
            def GetFontSize(self): return self.FontSize
            def SetFontSize(self, value): self.FontSize = value
            def GetTextColor(self): return self.TextColor
            def SetTextColor(self, value): self.TextColor = value

        class BorderStyle:
            def __init__(self, payload = None):
                self.Type = DocumentFilters.Annotations._Serializer.GetEnum(payload, 'type', DocumentFilters.Annotations.BorderStyleType, DocumentFilters.Annotations.BorderStyleType.Unknown)
                self.Width = DocumentFilters.Annotations._Serializer.Get(payload, 'width', 0)
                self.Intensity = DocumentFilters.Annotations._Serializer.Get(payload, 'intensity', 0)
                self.Dash = [ item for item in (payload or {}).get('dash') or [] ]

            def GetBorderType(self): return self.Type
            def GetIntensity(self): return self.Intensity
            def SetIntensity(self, value): self.Intensity = value
            def GetWidth(self): return self.Width
            def SetWidth(self, value): self.Width = value

        class IHasDefaultAppearance:
            def __init__(self, payload = None):
                self.DefaultAppearance = DocumentFilters.Annotations._Serializer.GetObj(payload, 'defaultAppearance', DocumentFilters.Annotations.DefaultAppearance, DocumentFilters.Annotations.DefaultAppearance())

            def GetDefaultAppearance(self): return self.DefaultAppearance

        class IHasRectDifferences:
            def __init__(self, payload = None):
                self.RectDifferences = DocumentFilters.Annotations._Serializer.GetObj(payload, 'rectDifferences', DocumentFilters.Annotations.Rect.Deserialize)

            def GetRectDifferences(self): return self.RectDifferences

        class IHasInteriorColor:
            def __init__(self, payload = None):
                self.InteriorColor = DocumentFilters.Annotations._Serializer.Get(payload, 'interiorColor') # Color

            def GetInteriorColor(self): return self.InteriorColor
            def SetInteriorColor(self, value): self.InteriorColor = value

        class Encoder(json.JSONEncoder):

            def default(self, o): # pylint: disable=E0202
                def camelCase(name):
                    return str(name[0]).lower() + name[1:]
                def normalizeValue(val):
                    if isinstance(val, Enum):
                        return camelCase(val.name)
                    elif isinstance(val, datetime.datetime):
                        return DocumentFilters.Annotations._Serializer.DateToStr(val)
                    else:
                        return val

                res = { camelCase(k): normalizeValue(v) for k, v in o.__dict__.items() }
                return res
                # return o.__dict__

        class Annotation:
            def __init__(self, type, payload = None):
                self.Type = type
                self.Flags = DocumentFilters.Annotations._Serializer.Get(payload, "flags", DocumentFilters.Annotations.Flags.Print)
                self.Name = DocumentFilters.Annotations._Serializer.Get(payload, "name")
                self.Text = DocumentFilters.Annotations._Serializer.Get(payload, "text")
                self.DateModified: datetime = DocumentFilters.Annotations._Serializer.GetDate(payload, "dateModified")
                self.DateCreated: datetime = DocumentFilters.Annotations._Serializer.GetDate(payload, "dateCreated")
                self.Color = DocumentFilters.Annotations._Serializer.Get(payload, "color")
                self.Title = DocumentFilters.Annotations._Serializer.Get(payload, "title")
                self.Subject = DocumentFilters.Annotations._Serializer.Get(payload, "subject")
                self.Opacity = DocumentFilters.Annotations._Serializer.Get(payload, "opacity")
                self.Intent = DocumentFilters.Annotations._Serializer.Get(payload, "intent")
                self.Replies = [DocumentFilters.Annotations.Note(item) for item in (payload or {}).get('replies') or [] ]
                self.Appearance = DocumentFilters.Annotations._Serializer.GetObj(payload, "appearance", DocumentFilters.Annotations.AppearanceStreams, DocumentFilters.Annotations.AppearanceStreams())
                self.Popup = DocumentFilters.Annotations._Serializer.GetObj(payload, 'popup', DocumentFilters.Annotations.Popup)
                self.Border = DocumentFilters.Annotations._Serializer.GetObj(payload, 'border', DocumentFilters.Annotations.BorderStyle, DocumentFilters.Annotations.BorderStyle())
                self.Rect = DocumentFilters.Annotations._Serializer.GetObj(payload, 'rect', DocumentFilters.Annotations.Rect.Deserialize, DocumentFilters.Annotations.Rect(0, 0, 0, 0))

            def serialize(self):
                return json.dumps(self, indent=2, cls=DocumentFilters.Annotations.Encoder)

            def GetAnnotationType(self): return self.Type
            def GetAppearance(self): return self.Appearance
            def GetBorder(self): return self.Border
            def GetColor(self): return self.Color
            def GetDateCreated(self): return self.DateCreated
            def GetDateModified(self): return self.DateModified
            def GetFlags(self): return self.Flags
            def GetIntent(self): return self.Intent
            def GetName(self): return self.Name
            def GetOpacity(self): return self.Opacity
            def GetPopup(self): return self.Popup
            def GetRect(self): return self.Rect
            def GetReplies(self): return self.Replies
            def GetSubject(self): return self.Subject
            def GetText(self): return self.Text
            def GetTitle(self): return self.Title

            def SetAnnotationType(self, value): self.Type = value
            def SetAppearance(self, value): self.Appearance = value
            def SetBorder(self, value): self.Border = value
            def SetColor(self, value): self.Color = value
            def SetDateCreated(self, value): self.DateCreated = value
            def SetDateModified(self, value): self.DateModified = value
            def SetFlags(self, value): self.Flags = value
            def SetIntent(self, value): self.Intent = value
            def SetName(self, value): self.Name = value
            def SetOpacity(self, value): self.Opacity = value
            def SetPopup(self, value): self.Popup = value
            def SetRect(self, value): self.Rect = value
            def SetReplies(self, value): self.Replies = value
            def SetSubject(self, value): self.Subject = value
            def SetText(self, value): self.Text = value
            def SetTitle(self, value): self.Title = value

            @property
            def AnnotationType(self): return self.GetAnnotationType()

        class Popup(Annotation):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Popup, payload)
                self.Open = DocumentFilters.Annotations._Serializer.Get(payload, 'open')

        def GetOpen(self): return self.Open
        def SetOpen(self, value): self.Open = value

        class Note(Annotation):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Text, payload)
                self.State = DocumentFilters.Annotations._Serializer.Get(payload, 'state')
                self.StateModel = DocumentFilters.Annotations._Serializer.Get(payload, 'stateModel')
                self.Author = DocumentFilters.Annotations._Serializer.Get(payload, 'author')

            def GetAuthor(self): return self.Author
            def SetAuthor(self, value): self.Author = value
            def GetState(self): return self.State
            def SetState(self, value): self.State = value
            def GetStateModel(self): return self.StateModel
            def SetStateModel(self, value): self.StateModel = value

        class StickyNote(Note):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Note.__init__(self, payload)
                self.IconName = DocumentFilters.Annotations._Serializer.Get(payload, 'iconName')
                self.Open = DocumentFilters.Annotations._Serializer.Get(payload, 'open')

            def GetIconName(self): return self.IconName
            def SetIconName(self, value): self.IconName = value
            def GetOpen(self): return self.Open
            def SetOpen(self, value): self.Open = value

        class Link(Annotation):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Link, payload)
                self.Highlight = DocumentFilters.Annotations._Serializer.Get(payload, 'highlight')
                self.QuadPoints = [ DocumentFilters.Annotations.Point(item) for item in (payload or {}).get('quadPoints') or [] ]
                self.Action = DocumentFilters.Annotations._Serializer.GetObj(payload, 'action', DocumentFilters.Annotations.Action)

            def GetAction(self): return self.Action
            def SetAction(self, value): self.Action = value
            def GetHighlight(self): return self.Highlight
            def GetPoints(self): return self.QuadPoints
            def SetPoints(self, value): self.QuadPoints = value

        class FreeText(Annotation, IHasDefaultAppearance):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Freetext, payload)
                DocumentFilters.Annotations.IHasDefaultAppearance.__init__(self, payload)
                self.Alignment = DocumentFilters.Annotations._Serializer.Get(payload, 'alignment', DocumentFilters.Annotations.AlignmentType.Left)

            def GetAlignment(self): return self.Alignment
            def SetAlignment(self, value): self.Alignment = value

        class Line(Annotation, IHasLineEndings, IHasInteriorColor, IHasPoints):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Line, payload)
                DocumentFilters.Annotations.IHasLineEndings.__init__(self, payload)
                DocumentFilters.Annotations.IHasInteriorColor.__init__(self, payload)
                DocumentFilters.Annotations.IHasPoints.__init__(self, payload)

        class Rectangle(Annotation, IHasInteriorColor, IHasRectDifferences):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Rectangle, payload)
                DocumentFilters.Annotations.IHasInteriorColor.__init__(self, payload)
                DocumentFilters.Annotations.IHasRectDifferences.__init__(self, payload)

        class Ellipse(Annotation, IHasInteriorColor, IHasRectDifferences):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Ellipse, payload)
                DocumentFilters.Annotations.IHasInteriorColor.__init__(self, payload)
                DocumentFilters.Annotations.IHasRectDifferences.__init__(self, payload)

        class Polygon(Annotation, IHasPoints, IHasInteriorColor):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Polygon, payload)
                DocumentFilters.Annotations.IHasPoints.__init__(self, payload)
                DocumentFilters.Annotations.IHasInteriorColor.__init__(self, payload)

        class Polyline(Annotation, IHasPoints, IHasLineEndings):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Polyline, payload)
                DocumentFilters.Annotations.IHasPoints.__init__(self, payload)
                DocumentFilters.Annotations.IHasLineEndings.__init__(self, payload)

        class TextMarkup(Annotation, IHasPoints):
            def __init__(self, type, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, type, payload)
                DocumentFilters.Annotations.IHasPoints.__init__(self, payload)

        class Highlight(TextMarkup):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.TextMarkup.__init__(self, DocumentFilters.Annotations.Type.Highlight, payload)

        class Squiggly(TextMarkup):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.TextMarkup.__init__(self, DocumentFilters.Annotations.Type.Squiggly, payload)

        class Strikeout(TextMarkup):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.TextMarkup.__init__(self, DocumentFilters.Annotations.Type.Strikeout, payload)

        class Underline(TextMarkup):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.TextMarkup.__init__(self, DocumentFilters.Annotations.Type.Underline, payload)

        class Stamp(Annotation, IHasDefaultAppearance):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Stamp, payload)
                DocumentFilters.Annotations.IHasDefaultAppearance.__init__(self, payload)

        class Ink(Annotation, IHasPoints):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Ink, payload)
                DocumentFilters.Annotations.IHasPoints.__init__(self, payload)

        class Barcode(Annotation):
            def __init__(self, subtype, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.Barcode, payload)
                self.SubType = subtype
                self.Caption = DocumentFilters.Annotations._Serializer.Get(payload, "caption", None)
                self.ErrorCorrectionLevel = DocumentFilters.Annotations._Serializer.Get(payload, "errorCorrectionLevel", 3)
                self.Margin = DocumentFilters.Annotations._Serializer.Get(payload, "margin", None)
                self.BgColor = DocumentFilters.Annotations._Serializer.Get(payload, "bgColor", None)
                self.Content = DocumentFilters.Annotations._Serializer.Get(payload, "content", None)

            @staticmethod
            def From(obj):
                if obj.SubType == "qr": return DocumentFilters.Annotations.QrCode(obj)
                elif obj.SubType == "aztec": return DocumentFilters.Annotations.Aztec(obj)
                elif obj.SubType == "dm": return DocumentFilters.Annotations.DataMatrix(obj)
                elif obj.SubType == "pdf417": return DocumentFilters.Annotations.PDF417(obj)
                elif obj.SubType == "code39": return DocumentFilters.Annotations.Code39(obj)
                elif obj.SubType == "code93": return DocumentFilters.Annotations.Code93(obj)
                elif obj.subType == "code128": return DocumentFilters.Annotations.Code128(obj)
                elif obj.SubType == "gs1-128": return DocumentFilters.Annotations.GS1_128(obj)
                else: return DocumentFilters.Annotations.Barcode(obj)

            def GetBgColor(self): return self.BgColor
            def SetBgColor(self, value): self.BgColor = value
            def GetCaption(self): return self.Caption
            def SetCaption(self, value): self.Caption = value
            def GetContent(self): return self.Content
            def SetContent(self, value): self.Content = value
            def GetErrorCorrectionLevel(self): return self.ErrorCorrectionLevel
            def SetErrorCorrectionLevel(self, value): self.ErrorCorrectionLevel = value
            def GetMargin(self): return self.Margin
            def SetMargin(self, value): self.Margin = value
            def GetSubType(self): return self.SubType
            def SetSubType(self, value): self.SubType = value

        class QrCode(Barcode):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Barcode.__init__(self, "qr", payload)

        class Aztec(Barcode):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Barcode.__init__(self, "aztec", payload)

        class DataMatrix(Barcode):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Barcode.__init__(self, "dm", payload)

        class PDF417(Barcode):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Barcode.__init__(self, "pdf417", payload)

        class Code39(Barcode):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Barcode.__init__(self, "code39", payload)

        class Code93(Barcode):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Barcode.__init__(self, "code93", payload)

        class Code128(Barcode):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Barcode.__init__(self, "code128", payload)

        class GS1_128(Barcode):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Barcode.__init__(self, "gs1-128", payload)
                self.autoCaption = None
                self.parts = []

            def addPart(self, name, value):
                part = {
                    'code': name,
                    'content': value
                }
                self.parts.append(part)
            
            def AddPart(self, name, value): self.addPart(name, value)

            def GetAutoCaption(self): return self.autoCaption
            def SetAutoCaption(self, value): self.autoCaption = value

        class NamedDestination(Annotation):
            def __init__(self, payload = None):
                DocumentFilters.Annotations.Annotation.__init__(self, DocumentFilters.Annotations.Type.NamedDestination, payload)

        @staticmethod
        def From(obj):
            payloadStr = DocumentFilters.Annotations._get_str(obj, ".", 1024*32)
            payload = json.loads(payloadStr)

            if obj.type == IGR_ANNOTATION_TEXT: result = DocumentFilters.Annotations.StickyNote(payload)
            elif obj.type == IGR_ANNOTATION_LINK: result = DocumentFilters.Annotations.Link(payload)
            elif obj.type == IGR_ANNOTATION_FREETEXT: result = DocumentFilters.Annotations.FreeText(payload)
            elif obj.type == IGR_ANNOTATION_LINE: result = DocumentFilters.Annotations.Line(payload)
            elif obj.type == IGR_ANNOTATION_SQUARE: result = DocumentFilters.Annotations.Rectangle(payload)
            elif obj.type == IGR_ANNOTATION_CIRCLE: result = DocumentFilters.Annotations.Ellipse(payload)
            elif obj.type == IGR_ANNOTATION_POLYGON: result = DocumentFilters.Annotations.Polygon(payload)
            elif obj.type == IGR_ANNOTATION_POLYLINE: result = DocumentFilters.Annotations.Polyline(payload)
            elif obj.type == IGR_ANNOTATION_HIGHLIGHT: result = DocumentFilters.Annotations.Highlight(payload)
            elif obj.type == IGR_ANNOTATION_UNDERLINE: result = DocumentFilters.Annotations.Underline(payload)
            elif obj.type == IGR_ANNOTATION_SQUIGGLE: result = DocumentFilters.Annotations.Squiggly(payload)
            elif obj.type == IGR_ANNOTATION_STRIKEOUT: result = DocumentFilters.Annotations.Strikeout(payload)
            elif obj.type == IGR_ANNOTATION_STAMP: result = DocumentFilters.Annotations.Stamp(payload)
            elif obj.type == IGR_ANNOTATION_INK: result = DocumentFilters.Annotations.Ink(payload)
            elif obj.type == IGR_ANNOTATION_POPUP: result = DocumentFilters.Annotations.Popup(payload)
            elif obj.type == IGR_ANNOTATION_BARCODE: result = DocumentFilters.Annotations.Barcode.From(payload)
            else:
                result = DocumentFilters.Annotation(DocumentFilters.Annotations._Serializer.GetEnum(payload, "type", DocumentFilters.Annotations.Type, DocumentFilters.Annotations.Type.Unknown), payload)

            return result

        @staticmethod
        def Fetch(pageHandle):
            result = []
            count = IGR_LONG(0)

            API = ISYS11dfAPI.Singleton()
            DocumentFiltersBase._Call(API.IGR_Get_Page_Annotation_Count, pageHandle, ctypes.byref(count))

            for i in range(count.value):
                read = IGR_LONG(1)
                item = IGR_Annotation()
                DocumentFiltersBase._Call(API.IGR_Get_Page_Annotations, pageHandle, i, ctypes.byref(read), ctypes.byref(item))

                result.append(DocumentFilters.Annotations.From(item))

            return result

        @staticmethod
        def _get_str(anno, path, maxLength = 4096):
            API = ISYS11dfAPI.Singleton()

            data = ctypes.create_string_buffer(maxLength * 2)
            size = IGR_LONG(maxLength)

            DocumentFiltersBase._Call(API.IGR_Get_Page_Annotation_Str,
                ctypes.byref(anno),
                DocumentFiltersBase._ToUTF16(path),
                ctypes.byref(size),
                ctypes.byref(data))
            return DocumentFiltersBase._FromUTF16(data, size.value)


    class RenderPageProperties:
        class Rect:
            def __init__(self, left, top, right, bottom):
                self.left = left
                self.top = top
                self.right = right
                self.bottom = bottom

            @staticmethod
            def xywh(x, y, w, h):
                return DocumentFilters.RenderPageProperties.Rect(x, y, x + w, y + h)

            @staticmethod
            def ltrb(l, t, r, b):
                return DocumentFilters.RenderPageProperties.Rect(l, t, r, b)

        def __init__(self):
            self._values = dict()
            self._formValueIndex = 0
            self._strings = list()
            self._redactionFlags = 0
            self._redactions = list()
            self._sourceRect = DocumentFilters.RenderPageProperties.Rect(0,0,0,0)
            self._destRect = DocumentFilters.RenderPageProperties.Rect(0,0,0,0)
            self._payload = IGR_Render_Page_Properties()

        def AddFormValue(self, name, value, selected = 0):
            self._values[name] = ( value, selected )

        def GetFormValueCount(self): return len(self._values)

        def GetFormValue(self, index): return self._values.items()[index] if index < len(self._values) else None

        def GetFirstFormValue(self):
            self._formValueIndex = 0
            return self.GetNextFormValue(self._formValueIndex)

        def GetNextFormValue(self):
            result = self.GetFormValue(self._formValueIndex)
            self._formValueIndex += 1
            return result

        @property
        def FormValueCount(self): return len(self._values)

        @property
        def FormValue(self, index): return self.GetFormValue(index)

        @property
        def FirstFormValue(self): return self.GetFirstFormValue()

        @property
        def NextFormValue(self): return self.GetNextFormValue()

        @property
        def RedactionFlags(self): return self._redactionFlags

        @RedactionFlags.setter
        def RedactionFlags(self, value): self._redactionFlags = value

        def AddRedaction(self, left, top, right, bottom, color):
            item = IGR_Render_Page_Redactions()
            item.x = left
            item.y = top
            item.width = right - left
            item.height = bottom - top
            item.color = color
            self._redactions.append(item)

        def SetSourceRect(self, left, top, right, bottom): self._sourceRect = DocumentFilters.RenderPageProperties.Rect(left, top, right, bottom)

        def SetDestRect(self, left, top, right, bottom): self._destRect = DocumentFilters.RenderPageProperties.Rect(left, top, right, bottom)

        @property
        def SourceRect(self): return self._sourceRect

        @property
        def DestRect(self): return self._destRect

        def _str(self, value):
            result = ctypes.create_string_buffer(DocumentFiltersBase._ToUTF16(value))
            self._strings.append(result)
            return ctypes.cast(ctypes.byref(result), ctypes.POINTER(IGR_UCS2))

        def _get_payload(self):
            self._payload.struct_size = ctypes.sizeof(IGR_Render_Page_Properties)
            self._payload.form_value_count = 0
            self._payload.redaction_flags = self._redactionFlags
            self._payload.redaction_count = 0
            self._payload.source_rect.left = self._sourceRect.left
            self._payload.source_rect.top = self._sourceRect.top
            self._payload.source_rect.right = self._sourceRect.right
            self._payload.source_rect.bottom = self._sourceRect.bottom
            self._payload.dest_rect.left = self._destRect.left
            self._payload.dest_rect.top = self._destRect.top
            self._payload.dest_rect.right = self._destRect.right
            self._payload.dest_rect.bottom = self._destRect.bottom

            self._strings = list()
            count = len(self._values)
            index = 0
            self._payload.form_values = (IGR_Render_Page_Form_Values * count)()
            self._payload.form_value_count = count
            for k, v in self._values.items():
                self._payload.form_values[index].name = self._str(k)
                self._payload.form_values[index].value = self._str(v[0])
                self._payload.form_values[index].selected = v[1]
                index = index + 1

            redaction_count = len(self._redactions)
            self._payload.redactions = (IGR_Render_Page_Redactions * redaction_count)()
            self._payload.redaction_count = redaction_count
            for i, v in enumerate(self._redactions):
                self._payload.redactions[i] = v

            return self._payload

    class Canvas(DocumentFiltersBase):
        def __init__(self, type, options):
            self._filename = None
            self._stream = None
            self._ownStream = False
            self._handle = None
            self._type = type
            self._options = options

        def __enter__(self):
            return self

        def __exit__(self, exception_type, exception_value, traceback):
            self.Close()

        @staticmethod
        def Make(destination, type, options):
            if isinstance(destination, io.IOBase) \
                or isinstance(destination, StreamBridge):
                return DocumentFilters.Canvas.OnStream(destination, type, options)
            elif isinstance(destination, str):
                return DocumentFilters.Canvas.OnFile(destination, type, options)
            elif isinstance(destination, IGRStream):
                return DocumentFilters.Canvas.OnStream(destination, type, options)
            else:
                raise Exception('Cannot create canvas on ' + str(destination))

        @staticmethod
        def OnFile(filename, type, options = ""):
            result = DocumentFilters.Canvas(type, options)
            result._filename = filename
            return result

        @staticmethod
        def OnStream(stream, type, options = ""):
            result = DocumentFilters.Canvas(type, options)
            if isinstance(stream, io.IOBase) or isinstance(stream, IGRStream):
                result._stream = ctypes.cast(StreamBridge.Make(stream, True), ctypes.POINTER(IGR_Stream))
                result._ownStream = True
            else:
                result._stream = ctypes.cast(stream, ctypes.POINTER(IGR_Stream))
            return result

        def Close(self):
            if self._handle != None:
                DocumentFiltersBase._Call(self.API.IGR_Close_Canvas, self._handle)

            if self._stream != None and self._ownStream:
                self._stream.contents.Close(self._stream)

            self._handle = None
            self._stream = None

        def RenderPage(self, page, options = "", renderProps = None):
            if not isinstance(renderProps, DocumentFilters.RenderPageProperties):
                renderProps = DocumentFilters.RenderPageProperties()

            props = renderProps._get_payload()

            DocumentFiltersBase._Call(self.API.IGR_Render_Page_Ex, page.Handle, self.Handle, DocumentFiltersBase._ToUTF16(options), ctypes.byref(props))

        def RenderPages(self, extractor):
            for page in extractor.Pages:
                try:
                    self.RenderPage(page)
                finally:
                    page.Close()

        def BlankPage(self, width, height, options=""):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_Blank_Page, self.Handle, DocumentFiltersBase._ToUTF16(options), width, height, None)

        def AddCustomMetadata(self, name, value):
            self._options += "META(\"" + name + "\",\"" + value + "\");"

        def Arc(self, x, y, x2, y2, x3, y3, x4, y4):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_Arc, self.Handle, x, y, x2, y2, x3, y3, x4, y4)

        def AngleArc(self, x1, y1, radius, startAngle, sweepAngle):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_AngleArc, self.Handle, x1, y1, radius, startAngle, sweepAngle)

        def Chord(self, x, y, x2, y2, x3, y3, x4, y4):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_Chord, self.Handle, x, y, x2, y2, x3, y3, x4, y4)

        def Ellipse(self, x, y, x2, y2):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_Ellipse, self.Handle, x, y, x2, y2)

        def Rect(self, x, y, x2, y2):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_Rect, self.Handle, IGR_LONG(x), IGR_LONG(y), IGR_LONG(x2), IGR_LONG(y2))

        def LineTo(self, x, y):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_LineTo, self.Handle, x, y)

        def MoveTo(self, x, y):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_MoveTo, self.Handle, x, y)

        def Pie(self, x, y, x2, y2, x3, y3, x4, y4):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_Pie, self.Handle, x, y, x2, y2, x3, y3, x4, y4)

        def RoundRect(self, x, y, x2, y2, radius):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_RoundRect, self.Handle, x, y, x2, y2, radius)

        def SetPen(self, color, width, style):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_SetPen, self.Handle, color, width, style)

        def SetBrush(self, color, style):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_SetBrush, self.Handle, color, style)

        def SetFont(self, text, size, style):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_SetFont, self.Handle, DocumentFiltersBase._ToUTF16(text), size, style)

        def SetOpacity(self, opacity):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_SetOpacity, self.Handle, opacity)

        def TextOut(self, x, y, text):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_TextOut, self.Handle, x, y, DocumentFiltersBase._ToUTF16(text))

        def TextRect(self, x, y, x2, y2, text, flags):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_TextRect, self.Handle, x, y, x2, y2, DocumentFiltersBase._ToUTF16(text), flags)

        def Rotation(self, degrees):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_Rotation, self.Handle, degrees)

        def Reset(self):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_Reset, self.Handle)

        def MeasureText(self, text):
            width = IGR_LONG(0)
            height = IGR_LONG(0)
            DocumentFiltersBase._Call(self.API.IGR_Canvas_MeasureText, self.Handle, DocumentFiltersBase._ToUTF16(text), ctypes.byref(width), ctypes.byref(height))
            return (width.value, height.value)

        def TextWidth(self, text):
            width, _1 = self.MeasureText(text)
            return width

        def TextHeight(self, text):
            _1, height = self.MeasureText(text)
            return height

        def DrawImage(self, x, y, imagedata, mimetype):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_DrawImage, self.Handle, x, y, imagedata, len(imagedata), DocumentFiltersBase._ToUTF16(mimetype))

        def DrawScaleImage(self, x, y, x2, y2, imagedata,  mimetype):
            DocumentFiltersBase._Call(self.API.IGR_CanvasIGR_Canvas_DrawScaleImage_DrawImage, self.Handle, x, y, x2, y2, imagedata, len(imagedata), DocumentFiltersBase._ToUTF16(mimetype))

        def Annotate(self, annotation):
            if isinstance(annotation, DocumentFilters.Annotations.Annotation):
                annotation = annotation.serialize()
            DocumentFiltersBase._Call(self.API.IGR_Canvas_Annotate_JSON, self.Handle, DocumentFiltersBase._ToUTF16(annotation))

        def ClearBookmarks(self):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_Bookmarks_Clear, self.Handle)

        def AppendBookmark(self, bookmark):
            DocumentFiltersBase._Call(self.API.IGR_Canvas_Bookmarks_Append, self.Handle, bookmark._packBookmark())

        def AppendBookmarkRecursive(self, bookmark):
            def walk(bookmark):
                self.AppendBookmark(bookmark)
                for i in bookmark._children:
                    i.Level = bookmark.Level + 1
                    walk(i)
            walk(bookmark)

        def _needStream(self):
            return self._stream

        def _needHandle(self):
            if self._handle == None:
                self._handle = IGR_LONG(0)
                if self._filename != None:
                    DocumentFiltersBase._Call(self.API.IGR_Make_Output_Canvas, \
                        self._type, \
                        DocumentFiltersBase._ToUTF16(self._filename), \
                        DocumentFiltersBase._ToUTF16(self._options), \
                        ctypes.byref(self._handle))
                elif self._stream != None:
                    DocumentFiltersBase._Call(self.API.IGR_Make_Output_Canvas_On, \
                        self._type, \
                        self._needStream(), \
                        DocumentFiltersBase._ToUTF16(self._options), \
                        ctypes.byref(self._handle))
                else:
                    raise IGRException('Invalid canvas')
            return self._handle

        @property
        def Handle(self):
            return self._needHandle()
    
    class CompareSettings():

        def __init__(self):
            self._item = IGR_Text_Compare_Settings()
            self._item.struct_size = ctypes.sizeof(IGR_Text_Compare_Settings)

        @property
        def CompareType(self): return self._item.compare_type

        @CompareType.setter
        def CompareType(self, value): self._item.compare_type = value

        @property
        def Flags(self): return self._item.flags
        
        @Flags.setter
        def Flags(self, value): self._item.flags = value

    class CompareDocumentSettings():
        def __init__(self):
            self._item = IGR_Text_Compare_Document_Source()
            self._item.struct_size = ctypes.sizeof(IGR_Text_Compare_Document_Source)

        @property
        def FirstPage(self): return self._item.doc_first_page

        @FirstPage.setter
        def FirstPage(self, value): self._item.doc_first_page = value
    
        @property
        def PageCount(self): return self._item.doc_page_count

        @PageCount.setter
        def PageCount(self, value): self._item.doc_page_count = value

        @property
        def Margins(self): return self._item.doc_margins

        @Margins.setter
        def Margins(self, value): self._item.doc_margins = value        
    

    class CompareDocumentSource(CompareDocumentSettings):
        def __init__(self):
            super().__init__()
            self._extractor = None

        @property
        def Extractor(self): return self._extractor

        @Extractor.setter
        def Extractor(self, value): 
            if isinstance(value, DocumentFilters.Extractor):
                self._extractor = value
                self._item.doc_handle = value._handle

        @property
        def DocumentHandle(self): return self._item.doc_handle

        @DocumentHandle.setter
        def DocumentHandle(self, value): 
            self._item.doc_handle = value


    class CompareResults(DocumentFiltersBase):
        def __init__(self, handle):
            self._handle = handle
            self._current = None
            self._res = None

        def __enter__(self):
            return self

        def __exit__(self, exception_type, exception_value, traceback):
            self.Close()            

        def Close(self):
            if self._handle is not None:
                DocumentFiltersBase._Call(self.API.IGR_Text_Compare_Close, self._handle)
                self._handle = None

        @property
        def Current(self): return self._res
        
        def MoveNext(self):
            if self._handle is None:
                return False
            
            self._current = IGR_Compare_Documents_Difference()
            self._current.struct_size = ctypes.sizeof(IGR_Compare_Documents_Difference)
            self._res = None

            if DocumentFiltersBase._Call(self.API.IGR_Text_Compare_Next, self._handle, self._current) == IGR_OK:
                self._res = DocumentFilters.CompareResultDifference(self._current)
                return True
            return False

        def GetNext(self):
            if self.MoveNext():
                return self._res
            else:
                return None

    class CompareResultDifference(DocumentFiltersBase):
        def __init__(self, item):
            self._type = item.type
            self._source = item.doc_source
            self._originalPageIndex = item.original_page_index
            self._revisedPageIndex = item.revised_page_index
            self._details = list()
            for i in range(item.item_count):
                self._details.append(DocumentFilters.CompareResultDifferenceDetail(item.items[i]))
            DocumentFiltersBase._Call(self.API.IGR_Text_Compare_Difference_Dispose, item)

        @property
        def Type(self): return self._type

        @property
        def Source(self): return self._source

        @property
        def OriginalPageIndex(self): return self._originalPageIndex

        @property
        def RevisedPageIndex(self): return self._revisedPageIndex

        @property
        def Details(self): return self._details

        def GetText(self):
            result = ""
            last = None
            for detail in self._details:
                if last is not None:
                    if detail.Bounds.top > last.top:
                        result += "\n"
                    elif detail.Bounds.left - 2 > last.right:
                        result += " "
                result += detail.Text
            return result

    class CompareResultDifferenceDetail(DocumentFiltersBase):

        def __init__(self, item):
            self._pageIndex = item.page_index
            self._bounds = IGR_FRect()
            self._bounds.left = item.bounds.left
            self._bounds.top = item.bounds.top
            self._bounds.right = item.bounds.right
            self._bounds.bottom = item.bounds.bottom
            self._text = DocumentFiltersBase._FromUTF16(ctypes.cast(item.text, ctypes.POINTER(ctypes.c_char)), 0xff)
        
        @property
        def PageIndex(self): return self._pageIndex

        @property
        def Text(self): return self._text

        @property
        def Bounds(self): return self._bounds

    class OpenCallback:
        class ActionHeartbeat:
            def __init__(self, internal):
                self._internal: IGR_Open_Callback_Action_Heartbeat = internal

        class ActionPassword:
            def __init__(self, internal):
                self._internal: IGR_Open_Callback_Action_Password = internal
                
            def GetId(self) -> str: return DocumentFiltersBase._FromUTF16(self._internal.id)
            def SetPassword(self, value: str): DocumentFiltersBase._ToUTF16Array(self._internal.password, value)
        
        class ActionLocalize:
            def __init__(self, internal):
                self._internal: IGR_Open_Callback_Action_Localize = internal
            
            @property
            def StringId(self) -> int: return self._internal.string_id

            @property
            def Original(self) -> str: return DocumentFiltersBase._FromUTF16(self._internal.original)

            @property
            def Replacement(self) -> str: return DocumentFiltersBase._FromUTF16(self._internal.replacement)

            @Replacement.setter
            def Replacement(self, value): 
                if value is not None:
                    DocumentFiltersBase._ToUTF16Array(self._internal.replacement, value)

        @property
        def Action(self) -> int: return self._action
    
        @property
        def Heartbeat(self) -> ActionHeartbeat: return self._heartbeat

        @property
        def Password(self) -> ActionPassword: return self._password
        
        @property
        def Localize(self) -> ActionLocalize: return self._localize

    class Extractor(DocumentFiltersBase):
        class SubFileEnumerator(DocumentFiltersBase):
            def __init__(self,  documentHandle, creator, getter):
                self._docHandle = documentHandle
                self._creator = creator
                self._getter = getter

            def __iter__(self):
                # Create a new enumerator instance
                h = HSUBFILES()
                DocumentFiltersBase._Call(self._creator, self._docHandle, ctypes.byref(h))
                return self.Iterator(h, self._docHandle, self._getter)

            class Iterator(DocumentFiltersBase):
                def __init__(self, enumeratorHandle, documentHandle, getter):
                    self._handle = enumeratorHandle
                    self._docHandle = documentHandle
                    self._getter = getter

                def __exit__(self, exception_type, exception_value, traceback):
                    self.close()

                def __del__(self):
                    self.close()

                def __next__(self):
                    result = DocumentFilters.Extractor._NextSubFile(self._handle, self._docHandle, self.API.IGR_Subfiles_Next, self._getter)
                    if result == None:
                        raise StopIteration
                    return result

                def next(self):
                    return self.__next__()
                
                def close(self):
                    if self._handle is not None:
                        DocumentFiltersBase._Call(self.API.IGR_Subfiles_Close, self._handle)
                        self._handle = None                    

        def __init__(self):
            self._handle = IGR_LONG(0)
            self._streamHandle = IGR_LONG(0)
            self._capabilities = IGR_LONG(0)
            self._filetype = IGR_LONG(0)
            self._pageCount = IGR_LONG(0)
            self._stream = None
            self._eof = False
            self._pageIndex = 0
            self._subfilesEnumerator = None
            self._imageEnumerator = None
            self._FPtr_IGR_OPEN_CALLBACK = None
            self._localize = {}
            self._localizer: 'Callable[[int, str], str]' = None

        def __enter__(self):
            return self

        def __exit__(self, exception_type, exception_value, traceback):
            self.Close()

        def __del__(self):
            self.Close()

        @staticmethod
        def _NextSubFile(enumeratorHandle, documentHandle, next, extractor):
            size = IGR_LONGLONG(0)
            date = IGR_LONGLONG(0)
            id = DocumentFiltersBase._UTF16Buffer(4096)
            name = DocumentFiltersBase._UTF16Buffer(1024)
            if DocumentFiltersBase._Call(next, enumeratorHandle, id, name, ctypes.byref(date), ctypes.byref(size)) == 0:

                return DocumentFilters.SubFile(documentHandle, \
                    DocumentFiltersBase._FromUTF16(id, 4096), \
                    DocumentFiltersBase._FromUTF16(name, 1024), \
                    size.value, \
                    date.value, \
                    extractor)
            return None

        @staticmethod
        def Make(source):
            if _IsPythonStream(source):
                return DocumentFilters.Extractor.FromStream(source)
            elif isinstance(source, str):
                return DocumentFilters.Extractor.FromFilename(source)
            elif isinstance(source, (bytes, bytearray)):
                return DocumentFilters.Extractor.FromBytes(source)
            elif isinstance(source, IGRStream):
                return DocumentFilters.Extractor.FromStream(source)
            else:
                raise Exception('Cannot create extractor on ' + str(type(source)))

        @staticmethod
        def FromFilename(filename):
            result = DocumentFilters.Extractor()
            result._from_filename(filename)
            return result

        @staticmethod
        def FromStream(stream):
            if _IsPythonStream(stream):
                return DocumentFilters.Extractor.FromStream(StreamBridge.Make(stream, False))
            else:
                result = DocumentFilters.Extractor()
                result._from_stream(stream)
                return result

        @staticmethod
        def FromBytes(bytes):
            return DocumentFilters.Extractor.FromStream(io.BytesIO(bytes))

        def _from_filename(self, filename):
            strm = ctypes.POINTER(IGR_Stream)()
            DocumentFiltersBase._Call(self.API.IGR_Make_Stream_From_File, DocumentFiltersBase._ToUTF16(filename), 0, ctypes.byref(strm))          
            self._stream = strm

        def _from_stream(self, stream):
            self._stream = ctypes.cast(stream, ctypes.POINTER(IGR_Stream))

        def _needStream(self):
            return self._stream

        def _needHandle(self, mode = IGR_BODY_AND_META):
            if self._handle.value == 0:
                self.Open(mode)
            return self._handle.value

        def Close(self, closeStream = True):
            if self._subfilesEnumerator is not None:
                DocumentFiltersBase._Call(self.API.IGR_Subfiles_Close, self._subfilesEnumerator)
                self._subfilesEnumerator = None
            if self._imageEnumerator is not None:
                DocumentFiltersBase._Call(self.API.IGR_Subfiles_Close, self._imageEnumerator)
                self._imageEnumerator = None
            if self._handle.value > 0:
                DocumentFiltersBase._Call(self.API.IGR_Close_File, self._handle)
                self._handle = IGR_LONG(0)
            if closeStream and self._stream != None:
                self._stream.contents.Close(self._stream)
                self._stream = None
            self._eof = False
            self._FPtr_IGR_OPEN_CALLBACK = None
        
        def _getOpenCallback(self, callback):
            def __IGR_Open_Callback(action, payload, context):
                callbackRequest = DocumentFilters.OpenCallback()
                callbackRequest._action = action
                if action == IGR_OPEN_CALLBACK_ACTION_HEARTBEAT:
                    callbackRequest._heartbeat = DocumentFilters.OpenCallback.ActionHeartbeat(IGR_Open_Callback_Action_Heartbeat.from_address(payload))
                elif action == IGR_OPEN_CALLBACK_ACTION_PASSWORD:
                    callbackRequest._password = DocumentFilters.OpenCallback.ActionPassword(IGR_Open_Callback_Action_Password.from_address(payload))
                elif action == IGR_OPEN_CALLBACK_ACTION_LOCALIZE:
                    callbackRequest._localize = DocumentFilters.OpenCallback.ActionLocalize(IGR_Open_Callback_Action_Localize.from_address(payload))
                
                result = None
                if action == IGR_OPEN_CALLBACK_ACTION_LOCALIZE:
                    if len(self._localize) > 0:
                        callbackRequest._localize.Replacement = self._localize.get(callbackRequest._localize.Original)
                    if self._localizer is not None:
                        callbackRequest._localize.Replacement = self._localizer(callbackRequest._localize.StringId, callbackRequest._localize.Original)

                if callback is None:
                    result = IGR_OK

                # Handle exceptions raised from user-provided callback
                # - Not handling these results in "Exception ignored on calling ctypes callback function" output from ctypes
                try:
                    if callback is not None:
                        result = callback(callbackRequest)
                except Exception as error:
                    # `Exception` is the base class of all non-fatal exceptions.  It doesn't seem like we want to 
                    # handle `BaseException`, which includes fatal exceptions used to indicate that the program should terminate.
                    print(f"Exception raised from user-provided IGR_OPEN_CALLBACK. Handling as if IGR_E_ERROR was returned. Exception details: error={error}, type(error)={type(error)}")
                    return IGR_E_ERROR
                
                # Handle user-provided callback returning incorrect type
                # - Not handling this results in "Exception ignored on converting result of ctypes callback function" output from ctypes
                try:
                    # Throws TypeError if conversion to int is not possible.
                    # For some reason we get "Exception ignored on converting result of ctypes callback function: 'c_int' object cannot 
                    # be interpreted as an integer" if this is IGR_LONG(result) or IGR_SHORT(result).
                    return int(result)
                except TypeError as error:
                    print(f"Return value from user-provided IGR_OPEN_CALLBACK cannot be interpreted as an integer. Handling as if IGR_E_ERROR was returned. Exception details: error={error}, type(error)={type(error)}")
                    return IGR_E_ERROR
                
            if callback == None and len(self._localize) == 0 and self._localizer is None:
                return None
            else:
                return IGR_OPEN_CALLBACK(__IGR_Open_Callback)

        def Open(self, flags = IGR_BODY_AND_META, options = "", callback: 'Callable[[DocumentFilters.OpenCallback], int]' = None):
            if self._handle.value > 0:
                self.Close(False)
            
            FPtr_IGR_OPEN_CALLBACK = self._getOpenCallback(callback)

            DocumentFiltersBase._Call(self.API.IGR_Open_Ex, \
                IGR_OPEN_FROM_STREAM, \
                self._needStream(), \
                IGR_LONG(flags), \
                DocumentFiltersBase._ToUTF16(options), \
                ctypes.byref(self._capabilities), \
                ctypes.byref(self._filetype), \
                0,
                FPtr_IGR_OPEN_CALLBACK,
                0,
                ctypes.byref(self._handle))
                
            # Store _FuncPointer as member of Extractor so it does not get garbage collected
            self._FPtr_IGR_OPEN_CALLBACK = FPtr_IGR_OPEN_CALLBACK

            if ( (flags & 0xffff0000) == IGR_FORMAT_IMAGE):
                DocumentFiltersBase._Call(self.API.IGR_Get_Page_Count, self._handle, ctypes.byref(self._pageCount))

        def _GetFileTypeAndCapabilities(self):
            try:
                if self._handle:
                    return self._capabilities.value, self._filetype.value
                else:
                    stream = self._needStream()
                    capabilities = IGR_LONG()
                    filetype = IGR_LONG()
                    DocumentFiltersBase._Call(self.API.IGR_Get_Stream_Type, stream, ctypes.byref(capabilities), ctypes.byref(filetype))
                    return capabilities.value, filetype.value
            except:
                return 0, 0


        def getFileType(self, what = None):
            _1, type = self._GetFileTypeAndCapabilities()
            if what != None:
                return DocumentFilters.Format.Fetch(self.API, type, what)
            return type

        def getSupportsText(self):
            return (self.GetFileCapabilities() & IGR_FILE_SUPPORTS_TEXT) != 0

        def getSupportsSubFiles(self):
            return (self.GetFileCapabilities() & IGR_FILE_SUPPORTS_SUBFILES) != 0

        def getSupportsHTML(self):
            return (self.GetFileCapabilities() & IGR_FILE_SUPPORTS_HDHTML) != 0

        def GetFileCapabilities(self):
            caps, _1 = self._GetFileTypeAndCapabilities()
            return caps

        def getEOF(self):
            return self.EOF

        def GetText(self, maxLength = 4096, stripControlCodes = False):
            data = ctypes.create_string_buffer(maxLength * 2)
            size = IGR_LONG(maxLength)
            DocumentFiltersBase._Call(self.API.IGR_Get_Text, self._needHandle(), ctypes.byref(data), ctypes.byref(size))
            self._eof = size.value == 0
            result =  DocumentFiltersBase._FromUTF16(data, size.value)
            if stripControlCodes:
                result = re.sub('[\x01-\x08,\x0b,\x0c-\x10]', '', result.replace('\x0e', '\n'))

            return result

        def SaveTo(self, stream, encoding = "utf-8", stripControlCodes = False):
            if isinstance(stream, str):
                with io.open(stream, "w", encoding = encoding) as stream:
                    self.SaveTo(stream)

            elif isinstance(stream, io.IOBase):
                while not self.EOF:
                    stream.write(self.GetText(stripControlCodes=stripControlCodes))
            else:
                raise IGRException("invalid argument")

        def CopyTo(self, stream):
            if isinstance(stream, str):
                with io.open(stream, "wb") as stream:
                    self.CopyTo(stream)

            elif isinstance(stream, io.IOBase):
                s = self._needStream()
                buffer = ctypes.create_string_buffer(1024)
                while 1:
                    len = s.contents.Read(s, buffer, 1024)
                    if len == 0: break
                    stream.write(buffer[:len])
            else:
                raise IGRException("Cannot CopyTo " + str(type(stream)))

        def getHashMD5(self):
            data = DocumentFiltersBase._UTF16Buffer(PERCEPTIVE_SZ_MD5_HEX_LEN)
            DocumentFiltersBase._Call(self.API.IGR_Calculate_MD5, self._needStream(), data)
            return DocumentFiltersBase._FromUTF16(data, PERCEPTIVE_SZ_MD5_HEX_LEN)

        def getHashSHA1(self):
            data = DocumentFiltersBase._UTF16Buffer(PERCEPTIVE_SZ_SHA1_HEX_LEN)
            DocumentFiltersBase._Call(self.API.IGR_Calculate_SHA1, self._needStream(), data)
            return DocumentFiltersBase._FromUTF16(data, PERCEPTIVE_SZ_SHA1_HEX_LEN)

        def GetPageCount(self):
            self._needHandle(IGR_FORMAT_IMAGE)
            return self._pageCount.value

        def GetPage(self, index):
            if index < 0 or index > self.PageCount:
                raise IndexError('page index out of bounds')
            return DocumentFilters.Page(self._needHandle(), index)

        def GetFirstSubFile(self):
            if self._subfilesEnumerator == None:
                self._subfilesEnumerator = HSUBFILES()
                DocumentFiltersBase._Call(self.API.IGR_Get_Subfiles_Enumerator, self._needHandle(), ctypes.byref(self._subfilesEnumerator))
            else:
                DocumentFiltersBase._Call(self.API.IGR_Subfiles_Reset, self._subfilesEnumerator)
            return self.GetNextSubFile()

        def GetNextSubFile(self):
            if self._subfilesEnumerator == None:
                return self.GetFirstSubFile()
            return DocumentFilters.Extractor._NextSubFile(self._subfilesEnumerator, self._needHandle(), self.API.IGR_Subfiles_Next, self.API.IGR_Extract_Subfile_Stream)

        def GetFirstImage(self):
            if self._imageEnumerator == None:
                self._imageEnumerator = HSUBFILES()
                DocumentFiltersBase._Call(self.API.IGR_Get_Images_Enumerator, self._needHandle(), ctypes.byref(self._imageEnumerator))
            else:
                DocumentFiltersBase._Call(self.API.IGR_Subfiles_Reset, self._imageEnumerator)
            return self.GetNextImage()

        def GetNextImage(self):
            if self._imageEnumerator == None:
                return self.GetFirstImage()
            return DocumentFilters.Extractor._NextSubFile(self._imageEnumerator, self._needHandle(), self.API.IGR_Subfiles_Next, self.API.IGR_Extract_Image_Stream)

        def GetSubFile(self, id):
            return DocumentFilters.SubFile(self._needHandle(), id, '', 0, 0, self.API.IGR_Extract_Subfile_Stream)

        def GetImage(self, id):
            return DocumentFilters.SubFile(self._needHandle(), id, '', 0, 0, self.API.IGR_Extract_Image_Stream)

        def GetFirstPage(self):
            self._pageIndex = 0
            return self.GetNextPage()

        def GetNextPage(self):
            if self._pageIndex < self.PageCount:
                result = self.GetPage(self._pageIndex)
                self._pageIndex += 1
                return result
            else:
                return None

        def GetRootBookmark(self):
            res = IGR_Bookmark()
            DocumentFiltersBase._Call(self.API.IGR_Get_Bookmark_Root, self._needHandle(), ctypes.byref(res))
            return DocumentFilters.Bookmark(self._needHandle(), res)
        
        def Compare(self, otherDocument, otherDocumentSettings = None, thisDocumentSettings = None, compareSettings = None):
            if otherDocument is None:
                raise IGRException("otherDocument cannot be None", 4)
            
            otherDocumentSource = None
            if isinstance(otherDocument, DocumentFilters.CompareDocumentSource):
                otherDocumentSource = otherDocument
            elif isinstance(otherDocument, DocumentFilters.Extractor):
                otherDocumentSource = DocumentFilters.CompareDocumentSource()
                otherDocumentSource.Extractor = otherDocument

                if isinstance(otherDocumentSettings, DocumentFilters.CompareDocumentSettings):
                    otherDocumentSource.FirstPage = otherDocumentSettings.FirstPage
                    otherDocumentSource.PageCount = otherDocumentSettings.PageCount
                    otherDocumentSource.Margins = otherDocumentSettings.Margins
            else:
                raise IGRException("otherDocument must be an Extractor or otherDocument", 4)

            thisDocumentSource = DocumentFilters.CompareDocumentSource()
            thisDocumentSource.DocumentHandle = self._needHandle()
            if isinstance(thisDocumentSettings, DocumentFilters.CompareDocumentSettings):
                thisDocumentSource.FirstPage = thisDocumentSettings.FirstPage
                thisDocumentSource.PageCount = thisDocumentSettings.PageCount
                thisDocumentSource.Margins = thisDocumentSettings.Margins
            
            res = IGR_HTEXTCOMPARE()

            wrappedSettings = DocumentFilters.CompareSettings()
            if isinstance(compareSettings, DocumentFilters.CompareSettings):
                wrappedSettings = compareSettings
            
            DocumentFiltersBase._Call(self.API.IGR_Text_Compare_Documents
                                      , ctypes.byref(thisDocumentSource._item)
                                      , ctypes.byref(otherDocumentSource._item)
                                      , ctypes.byref(wrappedSettings._item)
                                      , ctypes.byref(res))
            return DocumentFilters.CompareResults(res)        

        @property
        def FirstSubFile(self): return self.GetFirstSubFile()

        @property
        def NextSubFile(self): return self.GetNextSubFile()

        @property
        def FirstPage(self): return self.GetFirstPage()

        @property
        def NextPage(self): return self.GetNextPage()

        @property
        def FileType(self): return self.getFileType()

        @property
        def Capabilities(self): return self.GetFileCapabilities()

        @property
        def SupportsText(self): return self.getSupportsText()

        @property
        def SupportsSubFiles(self): return self.getSupportsSubFiles()

        @property
        def SupportsHTML(self): return self.getSupportsHTML()

        @property
        def EOF(self): return self._eof

        @property
        def EndOfStream(self): return self.EOF

        @property
        def PageCount(self): return self.GetPageCount()

        @property
        def HashMD5(self): return self.getHashMD5()

        @property
        def HashSHA1(self): return self.getHashSHA1()

        @property
        def Pages(self):
            count = lambda: self.GetPageCount()
            getter = lambda index: self.GetPage(index)
            return DocumentFiltersBase._ListOf(count, getter)

        @property
        def SubFiles(self):
            return DocumentFilters.Extractor.SubFileEnumerator(self._needHandle(), self.API.IGR_Get_Subfiles_Enumerator, self.API.IGR_Extract_Subfile_Stream)

        @property
        def Images(self):
            return DocumentFilters.Extractor.SubFileEnumerator(self._needHandle(), self.API.IGR_Get_Images_Enumerator, self.API.IGR_Extract_Image_Stream)

        @property
        def RootBookmark(self):
            return self.GetRootBookmark()

        @property
        def Localize(self): return self._localize

        @property
        def Localizer(self) -> 'Callable[[int, str], str]': 
            return self._localizer

        @Localizer.setter
        def Localizer(self, value: 'Callable[[int, str], str]'): 
            self._localizer = value        


    class SubFile(Extractor):
        def __init__(self, docHandle, id, name, size, date, extractor):
            super(DocumentFilters.SubFile, self).__init__()
            self._docHandle = docHandle
            self._id = id
            self._name = name
            self._size = size
            self._date = date
            self._extractor = extractor

        def _needStream(self):
            if self._stream == None:
                stream = ctypes.POINTER(IGR_Stream)()
                DocumentFiltersBase._Call(self._extractor, self._docHandle, DocumentFiltersBase._ToUTF16(self._id), ctypes.byref(stream))
                self._stream = stream
            return self._stream

        def getID(self): return self._id
        def getName(self): return self._name
        def getFileDate(self): return DocumentFiltersBase._FileTimeToDateTime(self._date)
        def getFileSize(self): return self._size
        def getFileTime(self): return DocumentFiltersBase._FileTimeToDateTime(self._date)

        @property
        def Name(self): return self.getName()

        @property
        def ID(self): return self.getID()

        @property
        def FileDate(self): return self.getFileDate()

        @property
        def FileSize(self): return self.getFileSize()

    def Initialize(self, license, resource_path = ".", dll_path = None):
        ecb = Error_Control_Block()
        isb = Instance_Status_Block()
        isb.Licensee_ID1 = license.encode('utf8')
        handle = IGR_SHORT()

        self._APIPrepare(dll_path)
        self.API.Init_Instance(0, resource_path.encode('utf8'), ctypes.byref(isb), ctypes.byref(handle), ctypes.byref(ecb))
        DocumentFiltersBase._Check(0, ecb)

    def Close(self):
        DocumentFiltersBase._Call(self.API.Close_Instance)

    def GetExtractorFromByteArray(self, bytes):
        return self.GetExtractor(bytes = bytes)

    def GetExtractorFromFile(self, filename):
        return self.GetExtractor(filename = filename)

    def GetExtractor(self, source = None, bytes = None, stream = None, filename = None):
        if bytes != None:
            return DocumentFilters.Extractor.FromBytes(bytes)
        elif stream != None:
            return DocumentFilters.Extractor.FromStream(stream)
        elif filename != None:
            return DocumentFilters.Extractor.FromFilename(filename)
        else:
            return DocumentFilters.Extractor.Make(source)

    def GetMemoryStatus(self):
        return ""

    def OpenExtractor(self, source = None, mode = IGR_BODY_AND_META, options = "", callback: 'Callable[[DocumentFilters.OpenCallback], int]' = None):
        res = self.GetExtractor(source)
        if res is not None:
            res.Open(mode, options, callback)
        return res

    def MakeOutputCanvas(self, destination, canvasType, options = ""):
        return DocumentFilters.Canvas.Make(destination, canvasType, options)

    def GetAvailableOptions(self):
        return DocumentFilters.Option.GetAll()

    def GetSupportedFormats(self):
        return DocumentFilters.Format.GetAll()

    def GetPageImage(self, source, pageIndex = 0, options = ""):
        with self.OpenExtractor(source, IGR_FORMAT_IMAGE, options) as file:
            with file.GetPage(pageIndex) as page:
                return page.GetPageBitmap()

    Annotation = Annotations.Annotation
    AnnotationAction = Annotations.Action
    AnnotationActionGoTo = Annotations.ActionGoTo
    AnnotationActionGoToRemote = Annotations.ActionGoToRemote
    AnnotationActionNamed = Annotations.ActionNamed
    AnnotationActionUri = Annotations.ActionUri
    AnnotationAppearanceStream = Annotations.AppearanceStream
    AnnotationAppearanceStreams = Annotations.AppearanceStreams
    AnnotationBarcode = Annotations.Barcode
    AnnotationBorderStyle = Annotations.BorderStyle
    AnnotationDefaultAppearance = Annotations.DefaultAppearance
    AnnotationEllipse = Annotations.Ellipse
    AnnotationFreeText = Annotations.FreeText
    AnnotationGS1_128 = Annotations.GS1_128
    AnnotationInk = Annotations.Ink
    AnnotationLine = Annotations.Line
    AnnotationLineEndings = Annotations.LineEndings
    AnnotationLink = Annotations.Link
    AnnotationNote = Annotations.Note
    AnnotationPoint = Annotations.Point
    AnnotationPolygon = Annotations.Polygon
    AnnotationPopup = Annotations.Popup
    AnnotationRect = Annotations.Rect
    AnnotationRectangle = Annotations.Rectangle
    AnnotationStamp = Annotations.Stamp
    AnnotationStickyNote = Annotations.StickyNote
    AnnotationTextMarkup = Annotations.TextMarkup
    AnnotationHighlight = Annotations.Highlight
    AnnotationSquiggly = Annotations.Squiggly
    AnnotationUnderline = Annotations.Underline
    AnnotationQrCode = Annotations.QrCode
    AnnotationAztec = Annotations.Aztec
    AnnotationPolyline = Annotations.Polyline
    AnnotationPDF417 = Annotations.PDF417
    AnnotationDataMatrix = Annotations.DataMatrix
    AnnotationCode39 = Annotations.Code39
    AnnotationCode93 = Annotations.Code93
    AnnotationCode128 = Annotations.Code128
    AnnotationStrikeout = Annotations.Strikeout
    AnnotationPolyLine = Annotations.Polyline


def IGRStreamBridge(stream):
    stream.Write = lambda data: stream.write(data)
    stream.Read = lambda size: stream.read(size)
    return stream

Api = DocumentFilters
RenderPageProperties = DocumentFilters.RenderPageProperties
Bookmark = DocumentFilters.Bookmark
Annotations = DocumentFilters.Annotations
AnnotationAction = Annotations.Action
AnnotationActionGoTo = Annotations.ActionGoTo
AnnotationActionGoToRemote = Annotations.ActionGoToRemote
AnnotationActionNamed = Annotations.ActionNamed
AnnotationActionUri = Annotations.ActionUri
AnnotationAppearanceStream = Annotations.AppearanceStream
AnnotationAppearanceStreams = Annotations.AppearanceStreams
AnnotationBarcode = Annotations.Barcode
AnnotationBorderStyle = Annotations.BorderStyle
AnnotationDefaultAppearance = Annotations.DefaultAppearance
AnnotationEllipse = Annotations.Ellipse
AnnotationFreeText = Annotations.FreeText
AnnotationGS1_128 = Annotations.GS1_128
AnnotationInk = Annotations.Ink
AnnotationLine = Annotations.Line
AnnotationLineEndings = Annotations.LineEndings
AnnotationLink = Annotations.Link
AnnotationNote = Annotations.Note
AnnotationPoint = Annotations.Point
AnnotationPolygon = Annotations.Polygon
AnnotationPopup = Annotations.Popup
AnnotationRect = Annotations.Rect
AnnotationRectangle = Annotations.Rectangle
AnnotationStamp = Annotations.Stamp
AnnotationStickyNote = Annotations.StickyNote
AnnotationTextMarkup = Annotations.TextMarkup
AnnotationHighlight = Annotations.Highlight
AnnotationSquiggly = Annotations.Squiggly
AnnotationUnderline = Annotations.Underline
AnnotationQrCode = Annotations.QrCode
AnnotationAztec = Annotations.Aztec
AnnotationPolyline = Annotations.Polyline
AnnotationPDF417 = Annotations.PDF417
AnnotationDataMatrix = Annotations.DataMatrix
AnnotationCode39 = Annotations.Code39
AnnotationCode93 = Annotations.Code93
AnnotationCode128 = Annotations.Code128
AnnotationStrikeout = Annotations.Strikeout
AnnotationPolyLine = Annotations.Polyline