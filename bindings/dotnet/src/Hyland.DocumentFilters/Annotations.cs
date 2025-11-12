using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.Security.Cryptography;
using System.Text;
using static System.Net.Mime.MediaTypeNames;

namespace Hyland.DocumentFilters.Annotations
{
    /// <summary>
    /// Represents various characteristics of the annotation
    /// </summary>
    [Flags]
    public enum Flags
    {
        /// <summary>
        /// Indicates the annotation should not be displayed if the annotation type is unknown.
        /// </summary>
        Invisible = 0x1,

        /// <summary>
        /// Indicates the annotation should not be displayed nor printed nor allowed to interact with the user.
        /// </summary>
        Hidden = 0x2,

        /// <summary>
        /// Indicates the annotation should be printed when the page is printed.
        /// </summary>
        Print = 0x4,

        /// <summary>
        /// Indicates the annotation’s appearance should not be scaled to match the magnification of the page.
        /// </summary>
        NoZoom = 0x8,

        /// <summary>
        /// Indicates the annotation’s appearance should not be rotated to match the rotation of the page.
        /// </summary>
        NoRotate = 0x10,

        /// <summary>
        /// Indicates the annotation should not be displayed nor allowed to interact with the user.
        /// </summary>
        NoView = 0x20,

        /// <summary>
        /// Indicates the annotation should not be allowed to interact with the user.
        /// </summary>
        ReadOnly = 0x40,

        /// <summary>
        /// Indicates the annotation should not be allowed to be deleted and its properties should not be allowed to be modified by the user.
        /// </summary>
        Locked = 0x80,

        /// <summary>
        /// Indicates the interpretation of the annotation’s NoView flag should be inverted for certain events.
        /// </summary>
        ToggleNoView = 0x100,

        /// <summary>
        /// Indicates the annotation’s contents should not be allowed to be modified by the user.
        /// </summary>
        LockedContents = 0x200
    };

    /// <summary>
    /// Represents the type of annotation.
    /// </summary>
    public enum Type
    {
        /// <summary>
        /// Indicates the annotation type is unknown.
        /// </summary>
        Unknown = ISYS11dfConstants.IGR_ANNOTATION_UNKNOWN,

        /// <summary>
        /// Indicates the annotation is a Text annotation.
        /// </summary>
        Text = ISYS11dfConstants.IGR_ANNOTATION_TEXT,

        /// <summary>
        /// Indicates the annotation is a Link annotation.
        /// </summary>
        Link = ISYS11dfConstants.IGR_ANNOTATION_LINK,

        /// <summary>
        /// Indicates the annotation is a Free Text annotation.
        /// </summary>
        Freetext = ISYS11dfConstants.IGR_ANNOTATION_FREETEXT,

        /// <summary>
        /// Indicates the annotation is a Line annotation.
        /// </summary>
        Line = ISYS11dfConstants.IGR_ANNOTATION_LINE,

        /// <summary>
        /// Indicates the annotation is a Square annotation.
        /// </summary>
        Rectangle = ISYS11dfConstants.IGR_ANNOTATION_SQUARE,

        /// <summary>
        /// Indicates the annotation is a Circle annotation.
        /// </summary>
        Ellipse = ISYS11dfConstants.IGR_ANNOTATION_CIRCLE,

        /// <summary>
        /// Indicates the annotation is a Polygon annotation.
        /// </summary>
        Polygon = ISYS11dfConstants.IGR_ANNOTATION_POLYGON,

        /// <summary>
        /// Indicates the annotation is a Polyline annotation.
        /// </summary>
        Polyline = ISYS11dfConstants.IGR_ANNOTATION_POLYLINE,

        /// <summary>
        /// Indicates the annotation is a Highlight annotation.
        /// </summary>
        Highlight = ISYS11dfConstants.IGR_ANNOTATION_HIGHLIGHT,

        /// <summary>
        /// Indicates the annotation is a Underline annotation.
        /// </summary>
        Underline = ISYS11dfConstants.IGR_ANNOTATION_UNDERLINE,

        /// <summary>
        /// Indicates the annotation is a Squiggly-underline annotation.
        /// </summary>
        Squiggly = ISYS11dfConstants.IGR_ANNOTATION_SQUIGGLE,

        /// <summary>
        /// Indicates the annotation is a Strikeout annotation.
        /// </summary>
        Strikeout = ISYS11dfConstants.IGR_ANNOTATION_STRIKEOUT,

        /// <summary>
        /// Indicates the annotation is a Rubber Stamp annotation.
        /// </summary>
        Stamp = ISYS11dfConstants.IGR_ANNOTATION_STAMP,

        /// <summary>
        /// Indicates the annotation is a Caret annotation.
        /// </summary>
        Caret = ISYS11dfConstants.IGR_ANNOTATION_CARET,

        /// <summary>
        /// Indicates the annotation is a Ink annotation.
        /// </summary>
        Ink = ISYS11dfConstants.IGR_ANNOTATION_INK,

        /// <summary>
        /// Indicates the annotation is a Pop-up annotation.
        /// </summary>
        Popup = ISYS11dfConstants.IGR_ANNOTATION_POPUP,

        /// <summary>
        /// Indicates the annotation is a File Attachment annotation.
        /// </summary>
        FileAttachment = ISYS11dfConstants.IGR_ANNOTATION_FILEATTACHMENT,

        /// <summary>
        /// Indicates the annotation is a Sound annotation.
        /// </summary>
        Sound = ISYS11dfConstants.IGR_ANNOTATION_SOUND,

        /// <summary>
        /// Indicates the annotation is a Movie annotation.
        /// </summary>
        Movie = ISYS11dfConstants.IGR_ANNOTATION_MOVIE,

        /// <summary>
        /// Indicates the annotation is a Widget annotation.
        /// </summary>
        Widget = ISYS11dfConstants.IGR_ANNOTATION_WIDGET,

        /// <summary>
        /// Indicates the annotation is a Screen annotation.
        /// </summary>
        Screen = ISYS11dfConstants.IGR_ANNOTATION_SCREEN,

        /// <summary>
        /// Indicates the annotation is a Printer’s Mark annotation.
        /// </summary>
        Printermark = ISYS11dfConstants.IGR_ANNOTATION_PRINTERMARK,

        /// <summary>
        /// Indicates the annotation is a Trap Network annotation.
        /// </summary>
        Trapnet = ISYS11dfConstants.IGR_ANNOTATION_TRAPNET,

        /// <summary>
        /// Indicates the annotation is a Watermark annotation.
        /// </summary>
        Watermark = ISYS11dfConstants.IGR_ANNOTATION_WATERMARK,

        /// <summary>
        /// Indicates the annotation is a 3D annotation.
        /// </summary>
        ThreeD = ISYS11dfConstants.IGR_ANNOTATION_3D,

        /// <summary>
        /// Indicates the annotation is a Barcode annotation.
        /// </summary>
        Barcode = ISYS11dfConstants.IGR_ANNOTATION_BARCODE,

        /// <summary>
        /// Indicates the annotation is a named destination annotation.
        /// </summary>
        NamedDestination = ISYS11dfConstants.IGR_ANNOTATION_NAMED_DESTINATION,
    }

    /// <summary>
    /// Represents the border style of the annotation.
    /// </summary>
    public enum BorderStyleType
    {
        /// <summary>
        /// Indicates the border style is unknown.
        /// </summary>
        Unknown = 0,

        /// <summary>
        /// Indicates the border style is solid.
        /// </summary>
        Solid = 1,

        /// <summary>
        /// Indicates the border style is dashed.
        /// </summary>
        Dashed = 2,

        /// <summary>
        /// Indicates the border style is beveled.
        /// </summary>
        Beveled = 3,

        /// <summary>
        /// Indicates the border style is inset.
        /// </summary>
        Inset = 4,

        /// <summary>
        /// Indicates the border style is underlined.
        /// </summary>
        Underline = 5
    }

    /// <summary>
    /// Represents the type of line ending for the annotation.
    /// </summary>
    public enum LineEndingType
    {
        /// <summary>
        /// Indicates the line ending type is none.
        /// </summary>
        None = 0,

        /// <summary>
        /// Indicates the line ending type is square.
        /// </summary>
        Square = 1,

        /// <summary>
        /// Indicates the line ending type is circle.
        /// </summary>
        Circle = 2,

        /// <summary>
        /// Indicates the line ending type is diamond.
        /// </summary>
        Diamond = 3,

        /// <summary>
        /// Indicates the line ending type is open arrow.
        /// </summary>
        OpenArrow = 4,

        /// <summary>
        /// Indicates the line ending type is closed arrow.
        /// </summary>
        ClosedArrow = 5,

        /// <summary>
        /// Indicates the line ending type is butt.
        /// </summary>
        Butt = 6,

        /// <summary>
        /// Indicates the line ending type is reverse open arrow.
        /// </summary>
        RightOpenArrow = 7,

        /// <summary>
        /// Indicates the line ending type is reverse closed arrow.
        /// </summary>
        RightClosedArrow = 8,

        /// <summary>
        /// Indicates the line ending type is slash.
        /// </summary>
        Slash = 9
    }

    /// <summary>
    /// Represents the alignment type of the annotation.
    /// </summary>

    public enum AlignmentType
    {
        /// <summary>
        /// Indicates the alignment type is left.
        /// </summary>
        Left = 0,

        /// <summary>
        /// Indicates the alignment type is center.
        /// </summary>
        Center = 1,

        /// <summary>
        /// Indicates the alignment type is right.
        /// </summary>
        Right = 2
    }

    /// <summary>
    /// Internal interface for annotations that have points.
    /// </summary>
    internal interface IHasPoints
    {
        /// <summary>
        /// The points of the annotation.
        /// </summary>
        List<System.Drawing.Point> Points { get; set; }
    }

    /// <summary>
    /// Internal interface for annotations that have a border.
    /// </summary>
    internal interface IHasLineEndings
    {
        /// <summary>
        /// The line endings of the annotation.
        /// </summary>
        LineEndings LineEndings { get; set; }
    }

    /// <summary>
    /// Internal interface for annotations that have a default appearance.
    /// </summary>
    internal interface IHasDefaultAppearance
    {
        /// <summary>
        /// The default appearance of the annotation.
        /// </summary>
        DefaultAppearance DefaultAppearance { get; set; }
    }

    /// <summary>
    /// Internal interface for annotations that have a rect differences.
    /// </summary>
    internal interface IHasRectDifferences
    {
        /// <summary>
        /// The rectangle differences of the annotation.
        /// </summary>
        System.Drawing.Rectangle RectDifferences { get; set; }
    }

    /// <summary>
    /// Internal interface for annotations that have an interior color.
    /// </summary>
    internal interface IHasInteriorColor
    {
        /// <summary>
        /// The interior color of the annotation.
        /// </summary>
        System.Drawing.Color InteriorColor { get; set; }
    }

    /// <summary>
    /// Represents the line ending of the annotation.
    /// </summary>
    public class LineEndings
    {
        /// <summary>
        /// The type of line ending at the beginning.
        /// </summary>
        public LineEndingType Begin { get; set; } = LineEndingType.None;

        /// <summary>
        /// The type of line ending at the end.
        /// </summary>
        public LineEndingType End { get; set; } = LineEndingType.None;
    }

    /// <summary>
    /// Represents the border appearance of the annotation.
    /// </summary>
    public class BorderStyle
    {
        /// <summary>
        /// The type of border style.
        /// </summary>
        public BorderStyleType Type { get; set; } = BorderStyleType.Solid;

        /// <summary>
        /// The width of the border.
        /// </summary>
        public double Width { get; set; } = 1;

        /// <summary>
        /// The intensity of the border.
        /// </summary>
        public double Intensity { get; set; }

        /// <summary>
        /// The dash pattern of the border.
        /// </summary>
        public List<int> Dash { get; set; } = null;
    }

    /// <summary>
    /// Internal class for serializing objects to JSON.
    /// </summary>
    internal class Serializer
    {
        private System.IO.TextWriter _stream;

        /// <summary>
        /// Constructor for the serializer.
        /// </summary>
        /// <param name="stream">The destination stream.</param>
        internal Serializer(System.IO.TextWriter stream)
        {
            _stream = stream;
        }

        /// <summary>
        /// Writes a date to the stream.
        /// </summary>
        public Serializer WriteDate(DateTime value)
        {
            return Write(value.ToString("r"));
        }

        /// <summary>
        /// Write a rectangle to the stream.
        /// </summary>
        public Serializer WriteRect(System.Drawing.Rectangle value)
        {
            if (value.IsEmpty)
                return Raw("null");
            else
                return Write(new double[] { value.Left, value.Top, value.Right, value.Bottom });
        }

        /// <summary>
        /// Write a color to the stream.
        /// </summary>
        public Serializer WriteColor(System.Drawing.Color value)
        {
            if (value.IsEmpty)
                return Raw("null");
            else
                return Write(string.Format("#{0:X2}{1:X2}{2:X2}", value.R, value.G, value.B));
        }

        /// <summary>
        /// Write a string to the stream.
        /// </summary>
        public Serializer WriteString(string value)
        {
            _stream.Write("\"");
            if (value != null)
            {
                for (var i = 0; i < value.Length; ++i)
                {
                    switch(value[i])
                    {
                        case '\r': _stream.Write("\\r"); break;
                        case '\n': _stream.Write("\\n"); break;
                        case '\t': _stream.Write("\\t"); break;
                        case '\\': _stream.Write("\\\\"); break;
                        case '\"': _stream.Write("\\\""); break;
                        default:   _stream.Write(value[i]); break;
                    }
                }
            }
            _stream.Write("\"");
            return this;
        }

        /// <summary>
        /// Write a raw string to the stream.
        /// </summary>
        Serializer Raw(string value)
        {
            _stream.Write(value);
            return this;
        }

        /// <summary>
        /// Write a stream to the stream.
        /// </summary>
        Serializer WriteStream(System.IO.Stream stream)
        {
            var pos = stream.Position;
            var length = stream.Length;
            if (length < int.MaxValue)
            {
                byte[] buffer = new byte[stream.Length];
                int read = stream.Read(buffer, 0, (int)length);
                WriteString(Convert.ToBase64String(buffer, 0, read));
            }
            else
            {
                Raw("null");
            }
            stream.Position = pos;
            return this;
        }

        /// <summary>
        /// Write an object to the stream.
        /// </summary>
        public Serializer Write(object value)
        {
            var type = value?.GetType();
            if (value == null)
            {
                _stream.Write("null");
                return this;
            }
            else if (value is string) return WriteString(value as string);
            else if (type == typeof(bool)) return Raw((bool)value ? "true" : "false");
            else if (type == typeof(char)) return WriteString(value.ToString());
            else if (type == typeof(byte)) return Raw(((byte)value).ToString(CultureInfo.InvariantCulture));
            else if (type == typeof(sbyte)) return Raw(((sbyte)value).ToString(CultureInfo.InvariantCulture));
            else if (type == typeof(short)) return Raw(((short)value).ToString(CultureInfo.InvariantCulture));
            else if (type == typeof(ushort)) return Raw(((ushort)value).ToString(CultureInfo.InvariantCulture));
            else if (type == typeof(int)) return Raw(((int)value).ToString(CultureInfo.InvariantCulture));
            else if (type == typeof(uint)) return Raw(((uint)value).ToString(CultureInfo.InvariantCulture));
            else if (type == typeof(long)) return Raw(((long)value).ToString(CultureInfo.InvariantCulture));
            else if (type == typeof(ulong)) return Raw(((ulong)value).ToString(CultureInfo.InvariantCulture));
            else if (type == typeof(float)) return Raw(((float)value).ToString(CultureInfo.InvariantCulture));
            else if (type == typeof(double)) return Raw(((double)value).ToString(CultureInfo.InvariantCulture));
            else if (type == typeof(decimal)) return Raw(((decimal)value).ToString(CultureInfo.InvariantCulture));
            else if (type == typeof(DateTime)) return WriteDate((DateTime)value);
            else if (type == typeof(System.Drawing.Rectangle)) return WriteRect((System.Drawing.Rectangle)value);
            else if (type == typeof(System.Drawing.Color)) return WriteColor((System.Drawing.Color)value);
            else if (value is System.Collections.IDictionary) return WriteDictionary((System.Collections.IDictionary)value);
            else if (value is System.Collections.IEnumerable) return WriteArray((System.Collections.IEnumerable)value);
            else if (value is System.IO.Stream) return WriteStream((System.IO.Stream)value);
            else if (type.IsEnum)
            {
                if (type.IsDefined(typeof(FlagsAttribute), inherit: false))
                {
                    // Flags
                    var enumValues = Enum.GetValues(type);
                    var enumNames = Enum.GetNames(type);
                    var v = (int)value;
                    var l = new List<string>();
                    for (var i = 0; i < enumValues.Length; ++i)
                    {
                        if ((v & (int)enumValues.GetValue(i)) != 0)
                        {
                            l.Add(PropName(enumNames[i]));
                        }
                    }
                    Write(l);
                }
                else
                {
                    WriteString(PropName(value.ToString()));
                }
            }
            else
            {
                _stream.Write("{");
                bool first = true;
                foreach (var p in type.GetProperties())
                {
                    var val = p.GetValue(value, new object[] { });
                    if (val == null)
                        continue;

                    if (!first)
                        Raw(",");

                    string pName = MapPropertyName(p);

                    Write(PropName(pName))
                        .Raw(":")
                        .Write(val);

                    first = false;
                }
                _stream.Write("}");
            }
            return this;
        }

        /// <summary>
        /// Returns the property name.
        /// </summary>
        public static string PropName(string s)
        {
            return s.Substring(0, 1).ToLower() + s.Substring(1);
        }

        /// <summary>
        /// Write an array to the stream.
        /// </summary>
        public Serializer WriteArray(System.Collections.IEnumerable value)
        {
            _stream.Write("[");
            bool first = true;
            foreach (var i in value)
            {
                if (!first) _stream.Write(",");
                Write(i);
                first = false;
            }
            _stream.Write("]");
            return this;
        }

        /// <summary>
        /// Write a dictionary to the stream.
        /// </summary>
        public Serializer WriteDictionary(System.Collections.IDictionary value)
        {
            _stream.Write("{");
            foreach (System.Collections.DictionaryEntry i in value)
            {
                Write(i.Key.ToString())
                    .Raw(":")
                    .Write(i.Value);
            }
            _stream.Write("}");
            return this;
        }

        /// <summary>
        /// Map the property name.
        /// </summary>
        private string MapPropertyName(System.Reflection.PropertyInfo propertyInfo)
        {
            if (propertyInfo.Name == nameof(Link.QuadPoints) &&
                propertyInfo.PropertyType == typeof(List<Point>) &&
                propertyInfo.DeclaringType == typeof(Link))
            {
                return "points";
            }

            return propertyInfo.Name;
        }
    }

    /// <summary>
    /// Internal class for deserializing objects from JSON.
    /// </summary>
    internal interface IIGR_Annotation_Info_Provider
    {
        int type { get; }
        int flags { get; }
        int x { get; }
        int y { get; }
        int width { get; }
        int height { get; }

        string GetStr(string path, int maxLength = 4096);

        int GetLong(string path);
    }

    internal class IGR_Annotation_Info_Provider : DocumentFiltersBase, IIGR_Annotation_Info_Provider
    {
        private IGR_Annotation _annotation;
        public IGR_Annotation_Info_Provider(IGR_Annotation annotation)
        {
            _annotation = annotation;
        }

        public int type => _annotation.type;
        public int flags => _annotation.flags;
        public int x => _annotation.x;
        public int y => _annotation.y;
        public int width => _annotation.width;
        public int height => _annotation.height;

        public int GetLong(string path)
        {
            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder pathSb = new StringBuilder(path);
            int res = 0;

            Check(ISYS11df.IGR_Get_Page_Annotation_Long(ref _annotation, pathSb, ref res, ref ecb), ecb);

            return res;
        }

        public string GetStr(string path, int maxLength = 4096)
        {
            if (path == "text")
                maxLength = 128*1024;

            Error_Control_Block ecb = new Error_Control_Block();
            StringBuilder pathSb = new StringBuilder(path);
            StringBuilder res = new StringBuilder(maxLength);
            int ml = res.Capacity;

            Check(ISYS11df.IGR_Get_Page_Annotation_Str(ref _annotation, pathSb, ref ml, res, ref ecb), ecb);
            res.Length = ml;

            return res.ToString();
        }
    }

    internal class DocFiltersAnnotationFactory
    {
        private readonly IIGR_Annotation_Info_Provider _provider;

        internal DocFiltersAnnotationFactory(IIGR_Annotation_Info_Provider provider)
        {
            if (provider == null)
            {
                throw new ArgumentNullException(nameof(provider));
            }
            _provider = provider;
        }

        public Annotation CreateAnnotation(string path = "")
        {
            Type type = Enum.IsDefined(typeof(Type), _provider.type) ? (Type)_provider.type : Type.Unknown;
            Annotation ann = GetAnnotationObject(type, path);

            ann.Flags = (Flags)_provider.flags;
            ann.Rect = new System.Drawing.Rectangle(_provider.x, _provider.y, _provider.width, _provider.height);

            Dictionary<string, object> ignoreSet = new Dictionary<string, object>()
            {
                { nameof(ann.Type),  null },
                { nameof(ann.Flags), null },
                { nameof(ann.Rect),  null },
            };

            FillTargetWithProperties(path, ref ann, ignoreSet);

            return ann;
        }

        private Annotation CreateAnnotationQueryOnly(string path = "", Type? forceType = null)
        {
            Type type;
            if (forceType.HasValue)
            {
                type = forceType.Value;
            }
            else
            {
                object t = null;
                FillTarget(path + ".type", ref t, typeof(Type));
                if (t == null)
                {
                    return null;
                }
                type = (Type)t;
            }    

            Annotation ann = GetAnnotationObject(type, path);

            Dictionary<string, object> ignoreSet = new Dictionary<string, object>()
            {
                { nameof(ann.Type),  null },
            };

            FillTargetWithProperties(path, ref ann, ignoreSet);

            return ann;
        }

        private Annotation GetAnnotationObject(Type type, string path = "")
        {
            switch (type)
            {
                case Type.Unknown:
                    return new Annotation(type);
                case Type.Text:
                    return new StickyNote();
                case Type.Link:
                    return new Link();
                case Type.Freetext:
                    return new FreeText();
                case Type.Line:
                    return new Line();
                case Type.Rectangle:
                    return new Rectangle();
                case Type.Ellipse:
                    return new Ellipse();
                case Type.Polygon:
                    return new Polygon();
                case Type.Polyline:
                    return new PolyLine();
                case Type.Highlight:
                    return new Highlight();
                case Type.Underline:
                    return new Underline();
                case Type.Squiggly:
                    return new Squiggly();
                case Type.Strikeout:
                    return new Strikeout();
                case Type.Stamp:
                    return new Stamp();
                case Type.Caret:
                    return new Annotation(type);
                case Type.Ink:
                    return new Ink();
                case Type.Popup:
                    return new Popup();
                case Type.FileAttachment:
                case Type.Sound:
                case Type.Movie:
                case Type.Widget:
                case Type.Screen:
                case Type.Printermark:
                case Type.Trapnet:
                case Type.Watermark:
                case Type.ThreeD:
                    return new Annotation(type);
                case Type.Barcode:
                    return GetBarcodeObject(path);
                default:
                    throw new InvalidOperationException();
            }
        }

        private Barcode GetBarcodeObject(string path = "")
        {
            string subType = _provider.GetStr(path + ".subType").ToLowerInvariant();
            switch (subType)
            {
                case "qr":
                    return new QrCode();
                case "aztec":
                    return new Aztec();
                case "dm":
                    return new DataMatrix();
                case "pdf417":
                    return new PDF417();
                case "code39":
                    return new Code39();
                case "code93":
                    return new Code93();
                case "code128":
                    return new Code128();
                case "gs1-128":
                    return new GS1_128();
                default:
                    return new Barcode();
            }
        }

        internal Action GetActionObject(string path = "")
        {
            string type = _provider.GetStr(path + ".type").ToLowerInvariant();
            switch (type)
            {
                case "goto":
                    return new ActionGoTo();
                case "uri":
                    return new ActionURI();
                case "gotor":
                    return new ActionGoToRemote();
                case "named":
                    return new ActionNamed();
                default:
                    return new Action();
            }
        }

        private void FillTargetWithProperties<T>(string path, ref T target, Dictionary<string, object> ignoreSet = null)
        {
            if (target == null)
            {
                return;
            }

            System.Reflection.PropertyInfo[] properties = target.GetType().GetProperties();
            if (properties.Length == 0)
            {
                Debug.Fail($"{nameof(FillTarget)} is missing a branch for type {target.GetType()}");
            }

            foreach (var prop in properties)
            {
                string propName = prop.Name;
                if (ignoreSet != null &&
                    ignoreSet.ContainsKey(propName))
                {
                    continue;
                }

                if (!prop.CanWrite)
                    continue;

                propName = Serializer.PropName(MapPropertyName(prop));

                string propPath = string.IsNullOrEmpty(path) ? propName : path + "." + propName;

                object propTarget = prop.GetValue(target, null);

                FillTarget(propPath, ref propTarget, prop.PropertyType);

                prop.SetValue(target, propTarget, null);
            }
        }

        private string MapPropertyName(System.Reflection.PropertyInfo propertyInfo)
        {
            if (propertyInfo.Name == nameof(Annotation.Border) &&
                propertyInfo.PropertyType == typeof(BorderStyle) &&
                propertyInfo.DeclaringType == typeof(Annotation))
            {
                return "border";
            }

            if (propertyInfo.Name == nameof(BorderStyle.Type) &&
                propertyInfo.PropertyType == typeof(BorderStyleType) &&
                propertyInfo.DeclaringType == typeof(BorderStyle))
            {
                return "type";
            }

            if (propertyInfo.Name == nameof(TextMarkup.Points) &&
                propertyInfo.PropertyType == typeof(List<Point>) &&
                propertyInfo.DeclaringType == typeof(TextMarkup))
            {
                return "points";
            }

            if (propertyInfo.Name == nameof(Note.Author) &&
                propertyInfo.PropertyType == typeof(string) &&
                propertyInfo.DeclaringType == typeof(Note))
            {
                return "author";
            }

            return propertyInfo.Name;
        }

        private void FillTarget(string path, ref object target, System.Type targetType)
        {
            if (targetType == typeof(int))
            {
                target = _provider.GetLong(path);
                return;
            }

            if (targetType == typeof(string))
            {
                target = _provider.GetStr(path);
                return;
            }

            if (targetType == typeof(double))
            {
                string str = _provider.GetStr(path);
                if (double.TryParse(str, out double d))
                {
                    target = d;
                }
                else if (str != "")
                {
                    Debug.Fail(string.Format("Failed to convert '{0}' to double.", str));
                }
                return;
            }
            if (targetType == typeof(DateTime))
            {
                string str = _provider.GetStr(path);
                if (DateTime.TryParse(str, out DateTime dt))
                {
                    target = dt;
                }
                else if (str != "")
                {
                    Debug.Fail(string.Format("Failed to convert '{0}' to DateTime.", str));
                }
                return;
            }
            if (targetType == typeof(bool))
            {
                string str = _provider.GetStr(path);
                if (bool.TryParse(str, out bool b))
                {
                    target = b;
                }
                else if (str != "")
                {
                    Debug.Fail(string.Format("Failed to convert '{0}' to bool.", str));
                }
                return;
            }
            if (targetType.IsEnum)
            {
                string str = _provider.GetStr(path);
                try
                {
                    object value = Enum.Parse(targetType, str);
                    if (Enum.IsDefined(targetType, value) || value.ToString().Contains(","))
                    {
                        target = value;
                    }
                    else
                    {
                        Debug.Fail(string.Format("{0} is not an underlying value of the {1} enumeration.", str, targetType));
                    }
                }
                catch (ArgumentException)
                {
                    if (str != "")
                    {
                        Debug.Fail(string.Format("'{0}' is not an member of the {1} enumeration.", str, targetType));
                    }
                }

                return;
            }

            if (IsNullOrEmpty(path))
            {
                return;
            }

            if (targetType == typeof(System.Drawing.Color))
            {
                string v = _provider.GetStr(path);
                if (!string.IsNullOrEmpty(v) && v.StartsWith("#"))
                {
                    // Color is either #AARRGGBB or #RRGGBB
                    v = v.Substring(1);
                    if (v.Length == 6)
                        target = Color.FromArgb(0xff,
                            Convert.ToInt32(v.Substring(0, 2), 16),
                            Convert.ToInt32(v.Substring(2, 2), 16),
                            Convert.ToInt32(v.Substring(4, 2), 16));
                    else if (v.Length == 8)
                        target = Color.FromArgb(
                            Convert.ToInt32(v.Substring(0, 2), 16),
                            Convert.ToInt32(v.Substring(2, 2), 16),
                            Convert.ToInt32(v.Substring(4, 2), 16),
                            Convert.ToInt32(v.Substring(6, 2), 16));
                    else
                        target = System.Drawing.Color.Transparent;
                }
                return;
            }
            if (targetType == typeof(System.Drawing.Point))
            {
                int x = _provider.GetLong(path + ".x");
                int y = _provider.GetLong(path + ".y");

                target = new System.Drawing.Point(x, y);
                return;
            }
            if (targetType == typeof(System.Drawing.Rectangle))
            {
                int left = _provider.GetLong(path + ".left");
                int top = _provider.GetLong(path + ".top");
                int right = _provider.GetLong(path + ".right");
                int bottom = _provider.GetLong(path + ".bottom");

                target = System.Drawing.Rectangle.FromLTRB(left, top, right, bottom);
                return;
            }

            if (targetType == typeof(System.IO.Stream))
            {
                target = new System.IO.MemoryStream(Convert.FromBase64String(_provider.GetStr(path)));
                return;
            }

            // Special casing 'Replies' because we need to make annotation objects with Type 'Text' even if the type is something else
            if (path.EndsWith("replies") && targetType == typeof(List<Note>))
            {
                List<Note> list = new List<Note>();

                try
                {
                    for (int i = 0; ; ++i)
                    {
                        string itemPath = path + "[" + i + "]";
                        Note note = null;
                        if (!IsNullOrEmpty(itemPath))
                        {
                            note = (Note)CreateAnnotationQueryOnly(itemPath, Type.Text);
                        }
                        list.Add(note);
                    }
                }
                catch (IGRException ex) when (ex.errorCode == ISYS11dfConstants.IGR_E_NOT_FOUND)
                { }

                target = list;
                return;
            }

            if (typeof(System.Collections.IList).IsAssignableFrom(targetType))
            {
                if (!targetType.IsGenericType)
                {
                    throw new InvalidOperationException("Unable to determine Type of items in list.");
                }

                System.Collections.IList list = (System.Collections.IList)Activator.CreateInstance(targetType);
                System.Type genType = targetType.GetGenericArguments()[0];

                // We decided to not implement .Count alias
                //
                //long count = _provider.GetLong(path + ".Count");
                //if (count < 1)
                //{
                //    return;
                //}
                //for (int i = 0; i < count; ++i)
                //{
                //    object o = default;
                //    FillTarget(path + "[" + i + "]", ref o, genType);
                //    list.Add(o);
                //}

                try
                {
                    for (int i = 0; ; ++i)
                    {
                        object o = null;
                        FillTarget(path + "[" + i + "]", ref o, genType);
                        list.Add(o);
                    }
                }
                catch (IGRException ex) when (ex.errorCode == ISYS11dfConstants.IGR_E_NOT_FOUND)
                { }

                target = list;
                return;
            }

            if (typeof(Annotation).IsAssignableFrom(targetType))
            {
                target = CreateAnnotationQueryOnly(path);
            }
            else if (targetType == typeof(Action))
            {
                target = GetActionObject(path);
                FillTargetWithProperties(path, ref target);
            }
            else
            {
                if (target == null)
                {
                    target = Activator.CreateInstance(targetType);
                }
                FillTargetWithProperties(path, ref target);
            }
        }

        private bool IsNullOrEmpty(string path)
        {
            string str = _provider.GetStr(path, 1);
            return string.IsNullOrEmpty(str);
        }
    }

    /// <summary>
    /// A pop-up annotation displays text in a pop-up window for entry and editing. 
    /// It typically does not appear alone but is associated with a markup annotation, its 
    /// parent annotation, and is used for editing the parent’s text.
    /// 
    /// It has no appearance stream or associated actions of its own.
    /// </summary>
    public class Popup : Annotation
    {

        /// <summary>
        /// Constructor for the Popup annotation.
        /// </summary>
        public Popup() : base(Type.Popup) { }

        /// <summary>
        /// Indicates whether the annotation is open/visible.
        /// </summary>
        public bool Open { get; set; }
    }

    /// <summary>
    /// Represents the default appearance values for the annotation, independent of any specific appearance stream.
    /// </summary>
    public class DefaultAppearance
    {
        /// <summary>
        /// Indicates the font name.
        /// </summary>
        public string FontName { get; set; } = "Helvetica";

        /// <summary>
        /// Indicates the font size.
        /// </summary>
        public double FontSize { get; set; } = 14;

        /// <summary>
        /// Indicates the text color.
        /// </summary>
        public System.Drawing.Color TextColor { get; set; } = System.Drawing.Color.Black;
    }

    /// <summary>
    /// Represents the base class for all annotation types.
    /// </summary>
    public class Annotation
    {
        /// <summary>
        /// Internal constructor for the annotation.
        /// </summary>
        internal Annotation(Type type)
        {
            Type = type;
        }

        /// <summary>
        /// Indicates the type of the annotation.
        /// </summary>
        public Type Type { get; private set; }

        /// <summary>
        /// Indicates the flags of the annotation.
        /// </summary>
        public Flags Flags { get; set; } = Flags.Print;

        /// <summary>
        /// Indicates the name of the annotation.
        /// </summary>
        public string Name { get; set; } = Guid.NewGuid().ToString("D");

        /// <summary>
        /// Indicates the text of the annotation.
        /// </summary>
        public string Text { get; set; }

        /// <summary>
        /// Indicates the rectangle of the annotation.
        /// </summary>
        public System.Drawing.Rectangle Rect { get; set; }

        /// <summary>
        /// Indicates the date the annotation was last modified.
        /// </summary>
        public DateTime DateModified { get; set; } = DateTime.UtcNow;

        /// <summary>
        /// Indicates the date the annotation was created.
        /// </summary>
        public DateTime DateCreated { get; set; } = DateTime.UtcNow;

        /// <summary>
        /// Indicates the border style of the annotation.
        /// </summary>
        public BorderStyle Border { get; set; }

        /// <summary>
        /// Indicates the color of the annotation.
        /// </summary>
        public System.Drawing.Color Color { get; set; }

        /// <summary>
        /// Contains the list of replies to the annotation.
        /// </summary>
        public List<Note> Replies { get; set; } = new List<Note>();

        /// <summary>
        /// Converts the annotation to a JSON string.
        /// </summary>
        /// <returns></returns>
        public string Serialize()
        {
            var strm = new System.IO.StringWriter();
            var serialzier = new Serializer(strm);
            serialzier.Write(this);
            return strm.ToString();
        }

        /// <summary>
        /// Pop-up annotation for entering or editing the text associated 
        /// with this annotation.
        /// </summary>
        public Popup Popup { get; set; }

        /// <summary>
        /// The text label to be displayed in the title bar of the 
        /// annotation's pop-up window when open and active. By convention, 
        /// this entry identifies the user who added the annotation.
        /// </summary>
        public string Title { get; set; }

        /// <summary>
        /// Text representing a short description of the subject being 
        /// addressed by the annotation.
        /// </summary>
        public string Subject { get; set; }

        /// <summary>
        /// The constant opacity value to be used in painting the annotation
        /// </summary>
        public double Opacity { get; set; } = 1;

        /// <summary>
        /// A name describing the intent of the markup annotation. Intents 
        /// allow viewer applications to distinguish between different uses 
        /// and behaviors of a single markup annotation type. If this entry 
        /// is not present or its value is the same as the annotation type, 
        /// the annotation has no explicit intentand should behave in a 
        /// generic manner in a viewer app
        /// </summary>
        public string Intent { get; set; }

        /// <summary>
        /// Contains the appearance streams for the annotation.
        /// </summary>
        public AppearanceStreams Appearance { get; set; }

        /// <summary>
        /// Internal method to create an annotation from an IGR_Annotation.
        /// </summary>
        static internal Annotation From(IGR_Annotation annotation)
        {
            var provider = new IGR_Annotation_Info_Provider(annotation);
            var factory = new DocFiltersAnnotationFactory(provider);
            return factory.CreateAnnotation();
        }
    }

    /// <summary>
    /// Represents the base class for note annotations.
    /// </summary>
    public class Note : Annotation
    {
        /// <summary>
        /// Indicates the state of the annotation.
        /// </summary>
        public string State { get; set; }

        /// <summary>
        /// Indicates the state model of the annotation.
        /// </summary>
        public string StateModel { get; set; }

        /// <summary>
        /// Indicates the author of the annotation.
        /// </summary>
        public string Author { get; set; }

        /// <summary>
        /// Constructor for the Note annotation.
        /// </summary>
        public Note() : base(Type.Text) { }
    }

    /// <summary>
    /// A text annotation represents a "sticky note" attached to a point in the document. When closed, the annotation appears
    /// as an icon; when open, it displays a pop-up window containing the text of the note in a font and size chosen by the
    /// viewer application.Text annotations do not scale and rotate with the page; they behave as if the NoZoom and NoRotate
    /// annotation flags were always set.
    /// </summary>

    public class StickyNote : Note
    {
        /// <summary>
        /// Indicates the name of the icon to use when the annotation is closed.
        /// </summary>
        public string IconName { get; set; }

        /// <summary>
        /// Indicates if the annotation is open/visible.
        /// </summary>
        public bool Open { get; set; }
    }

    /// <summary>
    /// Represents the highlight type of the annotation.
    /// </summary>
    public enum HighlightType
    {
        /// <summary>
        /// Indicates the highlight type is unknown.
        /// </summary>
        Unknown = 0,

        /// <summary>
        /// Indicates the highlight type is inverted.
        /// </summary>
        Invert = 1,

        /// <summary>
        /// Indicates the highlight type is outlined.
        /// </summary>
        Outline = 2,

        /// <summary>
        /// Indicates the highlight type is pushed.
        /// </summary>
        Push = 3
    }

    /// <summary>
    /// A link annotation represents either a hypertext link to a destination elsewhere in the document or an action to be performed.
    /// </summary>
    public class Link : Annotation, IHasPoints
    {

        /// <summary>
        /// Constructor for the Link annotation.
        /// </summary>
        public Link() : base(Type.Link) { }

        /// <summary>
        /// Indicates the highlight type of the annotation.
        /// </summary>
        public HighlightType Highlight { get; set; }

        /// <summary>
        /// Indicates the points of the annotation.
        /// </summary>
        public List<System.Drawing.Point> Points { get; set; } = new List<Point>();

        /// <summary>
        /// Indicates the points of the annotation.
        /// </summary>
        public List<System.Drawing.Point> QuadPoints => Points;

        /// <summary>
        /// Indicates the action to be performed when the annotation is activated.
        /// </summary>
        public Action Action { get; set; }

    }

    /// <summary>
    /// A free text annotation displays text directly on the page. Unlike an ordinary text annotation, a free text 
    /// annotation has no open or closed state; instead of being displayed in a pop-up window, the text is always visible.
    /// </summary>
    public class FreeText : Annotation, IHasDefaultAppearance
    {
        /// <summary>
        /// Constructor for the FreeText annotation.
        /// </summary>
        public FreeText() : base(Type.Freetext) { }

        /// <summary>
        /// Indicates the default appearance values for the annotation.
        /// </summary>
        public DefaultAppearance DefaultAppearance { get; set; }

        /// <summary>
        /// Indicates the text alignment of the annotation.
        /// </summary>
        public AlignmentType Alignment { get; set; }
    }

    /// <summary>
    /// A line annotation displays a single straight line on the page. When opened, it displays a pop-up window containing the text of the associated note. 
    /// </summary>
    public class Line : Annotation, IHasLineEndings, IHasInteriorColor
    {

        /// <summary>
        /// Constructor for the Line annotation.
        /// </summary>
        public Line() : base(Type.Line) { }

        /// <summary>
        /// Indicates the line endings of the annotation.
        /// </summary>
        public LineEndings LineEndings { get; set; } = new LineEndings();

        /// <summary>
        /// Indicates the interior color of the annotation.
        /// </summary>
        public System.Drawing.Color InteriorColor { get; set; } = System.Drawing.Color.Empty;

        /// <summary>
        /// Indicates the points of the annotation.
        /// </summary>
        public List<System.Drawing.Point> Points { get; set; } = new List<System.Drawing.Point>();
    }

    /// <summary>
    /// A rectangle annotation displays a rectangle on the page. When opened, it displays a pop-up window containing the text of the associated note.
    /// </summary>
    public class Rectangle : Annotation, IHasInteriorColor, IHasRectDifferences
    {
        /// <summary>
        /// Constructor for the Rectangle annotation.
        /// </summary>
        public Rectangle() : base(Type.Rectangle) { }

        /// <summary>
        /// Indicates the interior color of the annotation.
        /// </summary>
        public System.Drawing.Color InteriorColor { get; set; }

        /// <summary>
        /// Indicates the rectangle differences of the annotation.
        /// </summary>
        public System.Drawing.Rectangle RectDifferences { get; set; }
    }

    /// <summary>
    /// A circle or ellipse annotation displays an ellipse on the page. When opened, it displays a pop-up window containing the text of the associated note.
    /// </summary>
    public class Ellipse : Annotation, IHasInteriorColor, IHasRectDifferences
    {
        /// <summary>
        /// Constructor for the Ellipse annotation.
        /// </summary>
        public Ellipse() : base(Type.Ellipse) { }

        /// <summary>
        /// Indicates the interior color of the annotation.
        /// </summary>
        public System.Drawing.Color InteriorColor { get; set; }

        /// <summary>
        /// Indicates the rectangle differences of the annotation.
        /// </summary>
        public System.Drawing.Rectangle RectDifferences { get; set; }
    }

    /// <summary>
    /// Polygon annotations display closed polygons on the page. Such 
    /// polygons may have any number of vertices connected by straight lines. 
    /// Polyline annotations are similar to polygons, except that the 
    /// first and last vertex are not implicitly connected.
    /// </summary>
    public class Polygon : Annotation, IHasPoints, IHasInteriorColor
    {
        /// <summary>
        /// Constructor for the Polygon annotation.
        /// </summary>
        public Polygon() : base(Type.Polygon) { }

        /// <summary>
        /// Indicates the interior color of the annotation.
        /// </summary>
        public System.Drawing.Color InteriorColor { get; set; }

        /// <summary>
        /// Indicates the points of the annotation.
        /// </summary>
        public List<System.Drawing.Point> Points { get; set; } = new List<System.Drawing.Point>();
    }

    /// <summary>
    /// Polygon annotations display closed polygons on the page. Such polygons 
    /// may have any number of vertices connected by straight lines. Polyline 
    /// annotations are similar to polygons, except that the first and last 
    /// vertex are not implicitly connected.
    /// </summary>
    public class PolyLine : Annotation, IHasPoints, IHasLineEndings
    {
        /// <summary>
        /// Constructor for the PolyLine annotation.
        /// </summary>
        public PolyLine() : base(Type.Polyline) { }

        /// <summary>
        /// Indicates the points of the annotation.
        /// </summary>
        public List<System.Drawing.Point> Points { get; set; } = new List<System.Drawing.Point>();

        /// <summary>
        /// Indicates the line endings of the annotation.
        /// </summary>
        public LineEndings LineEndings { get; set; } = new LineEndings();
    }

    /// <summary>
    /// Represents the base class for text markup annotations.
    /// </summary>
    public class TextMarkup : Annotation, IHasPoints
    {
        /// <summary>
        /// Constructor for the TextMarkup annotation.
        /// </summary>
        internal TextMarkup(Type type) : base(type) { }

        /// <summary>
        /// Indicates the points of the annotation.
        /// </summary>
        public List<System.Drawing.Point> Points { get; set; } = new List<System.Drawing.Point>();
    }

    /// <summary>
    /// Highlight annotations are used to highlight text content on a page.
    /// </summary>
    public class Highlight : TextMarkup
    {
        /// <summary>
        /// Constructor for the Highlight annotation.
        /// </summary>
        public Highlight() : base(Type.Highlight) { }
    }

    /// <summary>
    /// Squiggly annotations are used to underline text content on a page with a squiggly line.
    /// </summary>
    public class Squiggly : TextMarkup
    {
        /// <summary>
        /// Constructor for the Squiggly annotation.
        /// </summary>
        public Squiggly() : base(Type.Squiggly) { }
    }

    /// <summary>
    /// Strikeout annotations are used to strike through text content on a page.
    /// </summary>
    public class Strikeout : TextMarkup
    {
        /// <summary>
        /// Constructor for the Strikeout annotation.
        /// </summary>
        public Strikeout() : base(Type.Strikeout) { }
    }

    /// <summary>
    /// Underline annotations are used to underline text content on a page.
    /// </summary>
    public class Underline : TextMarkup
    {
        /// <summary>
        /// Constructor for the Underline annotation.
        /// </summary>
        public Underline() : base(Type.Underline) { }
    }

    /// <summary>
    /// A rubber stamp annotation displays text or graphics intended to look 
    /// as if they were stamped on the page with a rubber stamp. When opened, 
    /// it displays a pop-up window containing the text of the associated 
    /// note.
    /// </summary>
    public class Stamp : Annotation, IHasDefaultAppearance
    {
        /// <summary>
        /// Constructor for the Stamp annotation.
        /// </summary>
        public Stamp() : base(Type.Stamp) { }

        /// <summary>
        /// Indicates the default appearance values for the annotation.
        /// </summary>
        public DefaultAppearance DefaultAppearance { get; set; }
    }

    /// <summary>
    /// Represents the base class for all barcode annotations.
    /// </summary>
    public class Barcode : Annotation
    {
        /// <summary>
        /// Constructor for the Barcode annotation.
        /// </summary>
        public Barcode() : base(Type.Barcode)
        {
            Color = Color.Black;
        }

        /// <summary>
        /// Indicates the subtype of the barcode.
        /// </summary>
        public string SubType { get; set; }

        /// <summary>
        /// Indicates the caption of the barcode.
        /// </summary>
        public string Caption { get; set; }

        /// <summary>
        /// Indicates the error correction level of the barcode.
        /// </summary>
        public int ErrorCorrectionLevel { get; set; } = 3;

        /// <summary>
        /// Indicates the margin of the barcode.
        /// </summary>
        public int Margin { get; set; } = 3;

        /// <summary>
        /// Indicates the background color of the barcode.
        /// </summary>
        public Color BgColor { get; set; } = Color.White;

        /// <summary>
        /// Indicates the content of the barcode.
        /// </summary>
        public string Content { get; set; }
    }

    /// <summary>
    /// Represents a QR code barcode annotation.
    /// </summary>
    public class QrCode : Barcode
    {
        /// <summary>
        /// Constructor for the QR code barcode annotation.
        /// </summary>
        public QrCode()
        {
            SubType = "qr";
        }
    }

    /// <summary>
    /// Represents an Aztec barcode annotation.
    /// </summary>
    public class Aztec : Barcode
    {
        /// <summary>
        /// Constructor for the Aztec barcode annotation.
        /// </summary>
        public Aztec()
        {
            SubType = "aztec";
        }
    }

    /// <summary>
    /// Represents a DataMatrix barcode annotation.
    /// </summary>
    public class DataMatrix : Barcode
    {
        /// <summary>
        /// Constructor for the DataMatrix barcode annotation.
        /// </summary>
        public DataMatrix()
        {
            SubType = "dm";
        }
    }

    /// <summary>
    /// Represents a PDF417 barcode annotation.
    /// </summary>
    public class PDF417 : Barcode
    {
        /// <summary>
        /// Constructor for the PDF417 barcode annotation.
        /// </summary>
        public PDF417()
        {
            SubType = "pdf417";
        }
    }

    /// <summary>
    /// Represents a Code39 barcode annotation.
    /// </summary>
    public class Code39 : Barcode
    {
        /// <summary>
        /// Constructor for the Code39 barcode annotation.
        /// </summary>
        public Code39()
        {
            SubType = "code39";
        }
    }

    /// <summary>
    /// Represents a Code93 barcode annotation.
    /// </summary>

    public class Code93 : Barcode
    {
        /// <summary>
        /// Constructor for the Code93 barcode annotation.
        /// </summary>
        public Code93()
        {
            SubType = "code93";
        }
    }

    /// <summary>
    /// Represents a Code128 barcode annotation.
    /// </summary>
    public class Code128 : Barcode
    {
        /// <summary>
        /// Constructor for the Code128 barcode annotation.
        /// </summary>
        public Code128()
        {
            SubType = "code128";
        }
    }

    /// <summary>
    /// Represents a GS1-128 barcode annotation.
    /// </summary>
    public class GS1_128 : Barcode
    {
        /// <summary>
        /// Constructor for the GS1-128 barcode annotation.
        /// </summary>
        public GS1_128()
        {
            SubType = "gs1-128";
        }

        /// <summary>
        /// Contains the application identifiers for the barcode.
        /// </summary>
        public IDictionary<string, string> Parts { get; set; } = new Dictionary<string, string>();

        /// <summary>
        /// Indicates if the barcode should be automatically captioned.
        /// </summary>
        public bool AutoCaption { get; set; }
    }

    /// <summary>
    /// An ink annotation represents a freehand "scribble" composed of one or 
    /// more disjoint paths. When opened, it displays a pop-up window 
    /// containing the text of the associated note.
    /// </summary>
    public class Ink : Annotation, IHasPoints
    {
        /// <summary>
        /// Constructor for the Ink annotation.
        /// </summary>
        public Ink() : base(Type.Ink) { }

        /// <summary>
        /// The first entry (i.e. path) in <see cref="InkList"/>.
        /// </summary>
        public List<System.Drawing.Point> Points
        {
            get
            {
                return InkList?[0];
            }
            set
            {
                if (InkList == null)
                {
                    InkList = new List<List<System.Drawing.Point>>();
                }
                InkList[0] = value;
            }
        }

        /// <summary>
        /// (Required) An array of n arrays, each representing a stroked path. 
        /// Each array is a series of alternating horizontal and vertical 
        /// coordinates in default user space, specifying points along the 
        /// path. When drawn, the points are connected by straight lines or 
        /// curves in an implementation-dependent way.
        /// </summary>
        public List<List<System.Drawing.Point>> InkList { get; set; } = new List<List<System.Drawing.Point>>
                                                                        {
                                                                            new List<System.Drawing.Point>(),
                                                                        };
    }

    /// <summary>
    /// Represents a named destination in the document.
    /// </summary>
    public class NamedDestination : Annotation
    {
        /// <summary>
        /// Constructor for the NamedDestination annotation.
        /// </summary>
        public NamedDestination() : base(Type.NamedDestination) { }
    }

    /// <summary>
    /// Represents the appearance stream of an annotation.
    /// </summary>
    public class AppearanceStream
    {
        /// <summary>
        /// The content in a supported format, such as SVG.
        /// </summary>
        public System.IO.Stream Content { get; set; }

        /// <summary>
        /// The content type of the Content stream.
        /// </summary>
        public string ContentType { get; set; }

        /// <summary>
        /// The encoding to use when embedding appearance stream.
        /// </summary>
        public string Encoding { get; private set; } = "Base64";

        /// <summary>
        /// Constructor for the AppearanceStream.
        /// </summary>
        public AppearanceStream()
        {
        }

        /// <summary>
        /// Constructor for the AppearanceStream.
        /// </summary>
        public AppearanceStream(System.IO.Stream stream)
        {
            Content = stream;
        }

#if !NETSTANDARD
        /// <summary>
        /// Constructor for the AppearanceStream.
        /// </summary>
        public AppearanceStream(System.Drawing.Image image)
        {
            Content = new System.IO.MemoryStream();
            image.Save(Content, System.Drawing.Imaging.ImageFormat.Png);
        }
#endif
    }

    /// <summary>
    /// Represents the appearance streams for an annotation.
    /// </summary>
    public class AppearanceStreams
    {
        /// <summary>
        /// Indicates the default appearance stream for the annotation.
        /// </summary>
        public AppearanceStream Normal { get; set; }

        /// <summary>
        /// Indicates the mouse-over appearance stream for the annotation.
        /// </summary>
        public AppearanceStream RollOver { get; set; }

        /// <summary>
        /// Indicates the appearance stream for when the annotation is clicked.
        /// </summary>
        public AppearanceStream Down { get; set; }
    }

    /// <summary>
    /// Represents the base class for Link actions.
    /// </summary>
    public class Action
    {

        /// <summary>
        /// Indicates the action type.
        /// </summary>
        public string Type { get; protected set; }

        /// <summary>
        /// Indicates if the action is performed in a new window.
        /// </summary>
        public bool NewWindow { get; set; }
    }

    /// <summary>
    /// Represents a goto action that scrolls to a specified location in the document.
    /// </summary>
    public class ActionGoTo : Action
    {
        /// <summary>
        /// Constructor for the GoTo action.
        /// </summary>
        public ActionGoTo()
        {
            Type = "GoTo";
        }

        /// <summary>
        /// Indicates the page number to scroll to.
        /// </summary>
        public int Page { get; set; }

        /// <summary>
        /// Indicates the named location to scroll to.
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Indicates the rectangle to scroll to.
        /// </summary>
        public System.Drawing.Rectangle Rect { get; set; }

        /// <summary>
        /// Indicates the zoom level to set once scrolled.
        /// </summary>
        public double Zoom { get; set; }
    }

    /// <summary>
    /// Represents a URI action that opens another document.
    /// </summary>
    public class ActionURI : Action
    {
        /// <summary>
        /// Constructor for the URI action.
        /// </summary>
        public ActionURI()
        {
            Type = "URI";
        }

        /// <summary>
        /// Indicates the URI to open.
        /// </summary>
        public string Uri { get; set; }
    }

    /// <summary>
    /// Represents a goto remote action that opens another document.
    /// </summary>
    public class ActionGoToRemote : ActionGoTo
    {
        /// <summary>
        /// Constructor for the GoToR action.
        /// </summary>
        public ActionGoToRemote()
        {
            Type = "GoToR";
        }

        /// <summary>
        /// Indicates the file to open.
        /// </summary>
        public string Filename { get; set; }
    }

    /// <summary>
    /// Represents a named action that can be executed.
    /// </summary>
    public class ActionNamed : Action
    {
        /// <summary>
        /// Constructor for the Named action.
        /// </summary>
        public ActionNamed()
        {
            Type = "Named";
        }

        /// <summary>
        /// Indicates the name of the action.
        /// </summary>
        public string Name { get; set; }
    }
}
