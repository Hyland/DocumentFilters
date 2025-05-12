//===========================================================================
// (c) 2020 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.IO;
using System.Runtime.InteropServices;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Represents the callback for opening a document.
    /// </summary>
    public class OpenCallback
    {
        /// <summary>
        /// Represents a pointer to a resource handle, initialized to zero. 
        /// </summary>
        public IntPtr Handle { get; internal set; } = IntPtr.Zero;
        /// <summary>
        /// Gets or sets the action to be performed.
        /// </summary>
        public int Action { get; internal set; }

        /// <summary>
        /// Gets or sets the heartbeat action payload.
        /// </summary>
        public IGR_Open_Callback_Action_Heartbeat Heartbeat { get; internal set; }

        /// <summary>
        /// Gets or sets the password action payload.
        /// </summary>
        public IGR_Open_Callback_Action_Password Password { get; internal set; }

        /// <summary>
        /// Gets or sets the localize action payload.
        /// </summary>
        public IGR_Open_Callback_Action_Localize Localize { get; internal set; }

        /// <summary>
        /// Gets or sets the log action payload.
        /// </summary>
        public IGR_Open_Callback_Action_Log_Level LogLevel { get; internal set; }

        /// <summary>
        /// Gets or sets the log message action payload.
        /// </summary>
        public IGR_Open_Callback_Action_Log_Message LogMessage { get; internal set; }

        /// <summary>
        /// Gets or sets the approve external resource payload.
        /// </summary>
        public IGR_Open_Callback_Action_Approve_External_Resource ApproveExternalResource { get; internal set; }

        /// <summary>
        /// Gets the resource stream through a callback action. 
        /// </summary>
        public IGR_Open_Callback_Action_Get_Resource_Stream GetResourceStream { get; internal set; }

        /// <summary>
        /// Retrieves an OCR image through the GetOcrImage property. It is accessible internally within the assembly.
        /// </summary>
        public IGR_Open_Callback_Action_OCR_Image GetOcrImage { get; internal set; }
    }

    /// <summary>
    /// Defines text styles for OCR, including Bold, Italic, and Underline. Each style is represented as a flag for
    /// flexible combinations.
    /// </summary>
    [Flags]
    public enum OcrTextStyle
    {
        /// <summary>
        /// Represents the bold text style constant in the ISYS11dfConstants class. Used to apply bold formatting to
        /// text.
        /// </summary>
        Bold = ISYS11dfConstants.IGR_TEXT_STYLE_BOLD,

        /// <summary>
        /// Represents the italic text style constant from ISYS11dfConstants. Used to apply italic formatting.
        /// </summary>
        Italic = ISYS11dfConstants.IGR_TEXT_STYLE_ITALIC,

        /// <summary>
        /// Represents the underline text style constant from ISYS11dfConstants. Used to apply underline formatting to
        /// text.
        /// </summary>
        Underline = ISYS11dfConstants.IGR_TEXT_STYLE_UNDERLINE,
    }

    /// <summary>
    /// Represents information related to the style of OCR (Optical Character Recognition) images. 
    /// </summary>
    public class OcrStyleInfo
    {
        private IGR_Open_Callback_Action_OCR_Image_Style_Info _payload = new IGR_Open_Callback_Action_OCR_Image_Style_Info();

        /// <summary>
        /// Gets or sets the font family used in the payload. It directly modifies the font_family property of the
        /// payload object.
        /// </summary>
        public string FontFamily { get => _payload.font_family; set => _payload.font_family = value; }

        /// <summary>
        /// Gets or sets the font size from the payload. The value is a float representing the size of the font.
        /// </summary>
        public float FontSize { get => _payload.font_size; set => _payload.font_size = value; }

        /// <summary>
        /// Gets or sets the text style for OCR processing. 
        /// </summary>
        public OcrTextStyle TextStyle { get => (OcrTextStyle)_payload.text_style; set => _payload.text_style = (uint)value; }

        /// <summary>
        /// Returns the payload containing OCR image style information. This is an internal property of type
        /// IGR_Open_Callback_Action_OCR_Image_Style_Info.
        /// </summary>
        internal IGR_Open_Callback_Action_OCR_Image_Style_Info Payload => _payload;
    }

    /// <summary>
    /// Represents an action for processing OCR images in an open callback context. 
    /// </summary>
    public class OpenCallbackActionOcrImage 
    {
        private readonly IGR_Open_Callback_Action_OCR_Image _ocrImage;
        private readonly IGR_Open_DIB_Info _dib_info;
        private readonly IntPtr _handle;

        /// <summary>
        /// Initializes an instance of the OpenCallbackActionOcrImage class, pinning the provided OCR image callback for
        /// use.
        /// </summary>
        /// <param name="callback">The parameter represents a callback action for processing OCR images.</param>
        public OpenCallbackActionOcrImage(OpenCallback callback)
        {
            _handle = callback.Handle;
            _ocrImage = callback.GetOcrImage;
            _dib_info = Marshaler.PtrToStructure<IGR_Open_DIB_Info>(_ocrImage.source_image_pixels);
        }


        /// <summary>
        /// Returns the raw OCR image object. This property provides access to the underlying image used for OCR
        /// processing.
        /// </summary>
        public IGR_Open_Callback_Action_OCR_Image Raw => _ocrImage;

        /// <summary>
        /// Retrieves the pixel data from the source image of the OCR process. This data is represented by the
        /// IGR_Open_DIB_Info type.
        /// </summary>
        public IGR_Open_DIB_Info PixelData => _dib_info;

        /// <summary>
        /// Gets the source page index as an integer from the OCR image. This value represents the page number
        /// associated with the image.
        /// </summary>
        public int SourcePageIndex => (int) _ocrImage.source_page_index;

        /// <summary>
        /// Gets the source rectangle from the OCR image. It returns an IGR_Rect representing the dimensions and
        /// position.
        /// </summary>
        public IGR_Rect SourceRect => _ocrImage.source_rect;

        /// <summary>
        /// Gets the stream of the OCR image.
        /// </summary>
        public void SaveImage(string filename, string mimeType)
        {
            _ocrImage.SaveImage(_handle, filename, mimeType);
        }

        /// <summary>
        /// Initiates a block processing operation using specified parameters.
        /// </summary>
        /// <param name="blockType">Defines the type of block to be processed.</param>
        /// <param name="points">Represents the coordinates or points relevant to the block.</param>
        public void StartBlock(uint blockType, IGR_QuadPoint points)
        {
            _ocrImage.StartBlock(_handle, blockType, ref points);
        }

        /// <summary>
        /// Close the block.
        /// </summary>
        /// <param name="blockType"></param>
        public void EndBlock(uint blockType)
        {
            _ocrImage.EndBlock(_handle, blockType);
        }

        /// <summary>
        /// Adds text to an OCR image using specified parameters.
        /// </summary>
        /// <param name="text">The string of text to be added to the OCR image.</param>
        /// <param name="points">Coordinates that define where the text will be placed on the image.</param>
        /// <param name="flags">Options that modify the behavior of the text addition process.</param>
        /// <param name="style">Information regarding the style of the text being added to the image.</param>
        public void AddText(string text, IGR_QuadPoint points, uint flags = 0, OcrStyleInfo style = null)
        {
            IntPtr stylePtr = Marshaler.StructureToPtr(style);
            try
            {
                _ocrImage.AddText(_handle, text, ref points, flags, stylePtr);
            }
            finally
            {
                if (stylePtr != IntPtr.Zero)
                    Marshal.FreeHGlobal(stylePtr);
            }
        }

        /// <summary>
        /// Adds specified text to a defined area of an image with optional flags and style settings.
        /// </summary>
        /// <param name="text">The string content to be added to the image.</param>
        /// <param name="rect">Defines the rectangular area on the image where the text will be placed.</param>
        /// <param name="flags">Specifies additional options for how the text is rendered.</param>
        /// <param name="style">Contains style information that affects the appearance of the text.</param>
        public void AddText(string text, System.Drawing.Rectangle rect, uint flags = 0, OcrStyleInfo style = null)
        {
            IGR_QuadPoint points = new IGR_QuadPoint
            {
                upperLeft = new IGR_FPoint
                {
                    x = rect.Left,
                    y = rect.Top
                },
                upperRight = new IGR_FPoint
                {
                    x = rect.Right,
                    y = rect.Top
                },
                lowerLeft = new IGR_FPoint
                {
                    x = rect.Left,
                    y = rect.Bottom
                },
                lowerRight = new IGR_FPoint
                {
                    x = rect.Right,
                    y = rect.Bottom
                }
            };
            AddText(text, points, flags, style);
        }

        /// <summary>
        /// Adjusts the orientation of an image by a specified angle in degrees.
        /// </summary>
        /// <param name="degrees">Specifies the angle to rotate the image.</param>
        public void Reorient(float degrees)
        {
            _ocrImage.Reorient(_handle, degrees);
        }
    }
}

