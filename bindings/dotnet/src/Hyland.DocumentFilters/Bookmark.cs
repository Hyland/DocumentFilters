//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace Hyland.DocumentFilters
{ 
  /// <summary>
    /// Represents a bookmark in the DocumentFilters library.
    /// </summary>
    public class Bookmark : DocumentFiltersBase
    {
        private readonly int? _doc;
        internal IGR_Bookmark _item;
        private IEnumerable<Bookmark> _children;

        /// <summary>
        /// Represents the possible actions for a bookmark.
        /// </summary>
        public enum ActionType
        {
            /// <summary>
            /// Indicates that the bookmark is a GoTo action.
            /// </summary>
            GoTo = ISYS11dfConstants.IGR_BOOKMARK_ACTION_GOTO,

            /// <summary>
            /// Indicates that the bookmark is a GoToR action.
            /// </summary>
            Link = ISYS11dfConstants.IGR_BOOKMARK_ACTION_LINK
        };

        /// <summary>
        /// Represents the possible fit types for a bookmark.
        /// </summary>
        public enum FitType
        {
            /// <summary>
            /// Indicates that the bookmark has no fit type.
            /// </summary>
            None = ISYS11dfConstants.IGR_BOOKMARK_FIT_NONE,

            /// <summary>
            /// Indicates that the bookmark fits both width and height.
            /// </summary>
            Both = ISYS11dfConstants.IGR_BOOKMARK_FIT_BOTH,

            /// <summary>
            /// Indicates that the bookmark fits the width.
            /// </summary>
            Width = ISYS11dfConstants.IGR_BOOKMARK_FIT_WIDTH,

            /// <summary>
            /// Indicates that the bookmark fits the height.
            /// </summary>
            Height = ISYS11dfConstants.IGR_BOOKMARK_FIT_HEIGHT,

            /// <summary>
            /// Indicates that the bookmark fits the rectangle.
            /// </summary>
            Rect = ISYS11dfConstants.IGR_BOOKMARK_FIT_RECT,
        }

        /// <summary>
        /// Represents the possible text style flags for a bookmark.
        /// </summary>
        [Flags]
        public enum TextStyleFlags
        {
            /// <summary>
            /// Indicates that the bookmark has a bold text style.
            /// </summary>
            Bold = ISYS11dfConstants.IGR_TEXT_STYLE_BOLD,

            /// <summary>
            /// Indicates that the bookmark has an italic text style.
            /// </summary>
            Italic = ISYS11dfConstants.IGR_TEXT_STYLE_ITALIC,

            /// <summary>
            /// Indicates that the bookmark has an underline text style.
            /// </summary>
            Underline = ISYS11dfConstants.IGR_TEXT_STYLE_UNDERLINE,
        }

        /// <summary>
        /// Gets or sets the title of the bookmark.
        /// </summary>
        public string Title
        {
            get
            {
                return _item.title;
            }
            set
            {
                _ = value ?? throw new ArgumentNullException(nameof(Title));

                int sizeConst = 255;
                FieldInfo field = typeof(IGR_Bookmark).GetField(nameof(IGR_Bookmark.title));
                object[] attributes = field.GetCustomAttributes(typeof(MarshalAsAttribute), false);
                if (attributes.Length > 0 && (attributes[0] is MarshalAsAttribute marshalAsAttribute))
                {
                    sizeConst = marshalAsAttribute.SizeConst;
                }

                var stringLength = Encoding.Unicode.GetByteCount(value) / sizeof(ushort);

                if (stringLength >= sizeConst)
                    throw new ArgumentException($"String length {stringLength} exceeds fixed buffer size {sizeConst}", nameof(Title));

                _item.title = value;
            }
        }

        /// <summary>
        /// Gets or sets the destination of the bookmark.
        /// </summary>
        public string Destination
        {
            get
            {
                return _item.dest;
            }
            set
            {
                _ = value ?? throw new ArgumentNullException(nameof(Destination));

                int sizeConst = 1024;
                FieldInfo field = typeof(IGR_Bookmark).GetField(nameof(IGR_Bookmark.dest));
                object[] attributes = field.GetCustomAttributes(typeof(MarshalAsAttribute), false);
                if (attributes.Length > 0 && (attributes[0] is MarshalAsAttribute marshalAsAttribute))
                {
                    sizeConst = marshalAsAttribute.SizeConst;
                }

                var stringLength = Encoding.Unicode.GetByteCount(value) / sizeof(ushort);

                if (stringLength >= sizeConst)
                    throw new ArgumentException($"String length {stringLength} exceeds fixed buffer size {sizeConst}", nameof(Destination));

                _item.dest = value;
            }
        }

        /// <summary>
        /// Gets or sets the color of the bookmark.
        /// </summary>
        public System.Drawing.Color Color
        {
            get
            {
                return System.Drawing.Color.FromArgb((_item.color & 0xFF0000) >> 16, (_item.color & 0xFF00) >> 8, _item.color & 0xFF);
            }
            set
            {
                _item.color = ((int)value.R) << 16 | ((int)value.G) << 8 | ((int)value.B);
            }
        }

        /// <summary>
        /// Gets or sets the action of the bookmark.
        /// </summary>
        public ActionType Action
        {
            get
            {
                return (ActionType)_item.action;
            }
            set
            {
                _item.action = (int)value;
            }
        }

        /// <summary>
        /// Gets or sets the fit type of the bookmark.
        /// </summary>
        public FitType Fit
        {
            get
            {
                return (FitType)_item.fit;
            }
            set
            {
                _item.fit = (int)value;
            }
        }

        /// <summary>
        /// Gets the children bookmarks of the current bookmark.
        /// </summary>
        public IEnumerable<Bookmark> Children => _children;

        /// <summary>
        /// Gets or sets the level of the bookmark.
        /// </summary>
        public int Level
        {
            get
            {
                return _item.level;
            }
            set
            {
                _item.level = value;
            }
        }

        /// <summary>
        /// Gets or sets the zoom factor of the bookmark.
        /// </summary>
        public double Zoom
        {
            get
            {
                return _item.zoom;
            }
            set
            {
                _item.zoom = (int)value;
            }
        }

        /// <summary>
        /// Gets or sets the rectangle of the bookmark.
        /// </summary>
        public System.Drawing.Rectangle Rect
        {
            get
            {
                return new System.Drawing.Rectangle(_item.x, _item.y, _item.width, _item.height);
            }
            set
            {
                _item.x = value.X;
                _item.y = value.Y;
                _item.width = value.Width;
                _item.height = value.Height;
            }
        }

        /// <summary>
        /// Gets or sets the page index of the bookmark.
        /// </summary>
        public int PageIndex
        {
            get
            {
                return int.TryParse(Destination, out int res) ? res : -1;
            }
            set
            {
                Destination = $"{value}";
            }
        }

        /// <summary>
        /// Gets or sets the text style flags of the bookmark.
        /// </summary>
        public TextStyleFlags TextStyle
        {
            get
            {
                return (TextStyleFlags)_item.text_style;
            }
            set
            {
                _item.text_style = (int)value;
            }
        }

        /// <summary>
        /// Gets the first child bookmark of the current bookmark.
        /// </summary>
        /// <returns>The first child bookmark.</returns>
        public Bookmark GetFirstChild()
        {
            if (_doc.HasValue && !(_children is IList<Bookmark>))
            {
                IGR_Bookmark res = new IGR_Bookmark();
                if (ISYS11df.IGR_Get_Bookmark_First_Child(_doc.Value, ref _item, ref res, ref ecb) == 0)
                    return new Bookmark(_doc.Value, res);
            }
            else
            {
                return Children.GetEnumerator().Current;
            }
            return null;
        }

        /// <summary>
        /// Gets the next sibling bookmark of the current bookmark.
        /// </summary>
        /// <returns>The next sibling bookmark.</returns>
        [Obsolete("Enumerate parent bookmark's children. GetNextSibling will result in undefined behavior when creating bookmarks.")]
        public Bookmark GetNextSibling()
        {
            if (_doc.HasValue)
            {
                IGR_Bookmark res = new IGR_Bookmark();
                if (ISYS11df.IGR_Get_Bookmark_Next_Sibling(_doc.Value, ref _item, ref res, ref ecb) == 0)
                    return new Bookmark(_doc.Value, res);
            }
            return null;
        }

        /// <summary>
        /// Adds a child bookmark to the current bookmark.
        /// </summary>
        /// <param name="bookmark">The child bookmark to add.</param>
        public void AddChild(Bookmark bookmark)
        {
            if (bookmark.Level != Level + 1)
                throw new ArgumentOutOfRangeException(nameof(Level), $"{nameof(Level)} must be {Level + 1}");

            if (!(_children is IList<Bookmark>))
            {
                _children = new List<Bookmark>(_children);
            }

            var list = (IList<Bookmark>)Children;
            list.Add(bookmark);
        }

        internal Bookmark(int docHandle, IGR_Bookmark? item = null)
        {
            _doc = docHandle;
            if (item.HasValue)
            {
                _item = item.Value;
            }
            else
            {
                _item = new IGR_Bookmark();
                Check(ISYS11df.IGR_Get_Bookmark_Root(docHandle, ref _item, ref ecb));
            }
            _children = new BookmarkEnumerable(this);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Bookmark"/> class.
        /// </summary>
        public Bookmark()
        {
            _doc = null;
            _item = new IGR_Bookmark();
            _children = new List<Bookmark>();
        }

        internal class BookmarkEnumerable : IEnumerable<Bookmark>
        {
            internal class BookmarkEnumerator : IEnumerator<Bookmark>
            {
                private Bookmark _parent;
                private bool _first;

                public BookmarkEnumerator(Bookmark bookmark)
                {
                    _parent = bookmark;
                    _first = true;
                }

                public Bookmark Current { get; private set; }

                object IEnumerator.Current => Current;

                public void Dispose()
                {
                }

                public bool MoveNext()
                {
                    if (_first)
                        Current = _parent.GetFirstChild();
                    else if (Current != null)
#pragma warning disable CS0618 // Type or member is obsolete
                        Current = Current.GetNextSibling();
#pragma warning restore CS0618 // Type or member is obsolete

                    _first = false;
                    return Current != null;
                }

                public void Reset()
                {
                    _first = true;
                }
            }

            Bookmark _bookmark;
            public BookmarkEnumerable(Bookmark bookmark)
            {
                _bookmark = bookmark;
            }
            public IEnumerator<Bookmark> GetEnumerator()
            {
                return new BookmarkEnumerator(_bookmark);
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return new BookmarkEnumerator(_bookmark);
            }
        }

    }
}
