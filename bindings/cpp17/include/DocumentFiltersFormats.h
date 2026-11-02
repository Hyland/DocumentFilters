/*
(c) 2026 Hyland Software, Inc. and its affiliates. All rights reserved.

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
 * Document Filters format IDs
 *
 * The values returned through the filetype parameter of IGR_Get_File_Type
 * and accepted as formatid by IGR_Get_Format_Attribute. Format IDs never
 * change, and a retired ID is never reused.
 ****************************************************************************/

#ifndef HYLAND_DOCFILTERS_FORMATS_H_DEF
#define HYLAND_DOCFILTERS_FORMATS_H_DEF

/**
 * Identifies the format of a document, as returned by IGR_Get_File_Type.
 *
 * Retired IDs, which are never reused: 17, 21, 31, 41, 42, 61, 62.
 */
typedef enum IGR_FORMATID
{
	IGR_FORMATID_UNKNOWN = 0,                  /**< Unknown. Binding: Unknown */
	IGR_FORMATID_BYPASS = 1,                   /**< Bypass. Binding: Bypass */
	IGR_FORMATID_TEXT_ASCII = 2,               /**< Text (ASCII). Binding: TextASCII */
	IGR_FORMATID_WORDSTAR_2_3_4 = 3,           /**< Wordstar 3 or 4. Binding: WordStar2_3_4 */
	IGR_FORMATID_MULTIMATE = 4,                /**< Multimate. Binding: MultiMate */
	IGR_FORMATID_MS_WORD_FOR_DOS = 5,          /**< Microsoft Word for DOS. Binding: MSWordForDOS */
	IGR_FORMATID_WORDPERFECT = 6,              /**< WordPerfect 5 - X4. Binding: WordPerfect */
	IGR_FORMATID_DCA_RFT = 7,                  /**< IBM DCA/RFT. Binding: DCARFT */
	IGR_FORMATID_SOURCE_CODE = 8,              /**< Source Code. Binding: SourceCode */
	IGR_FORMATID_PROFESSIONAL_WRITE = 9,       /**< Professional Write. Binding: ProfessionalWrite */
	IGR_FORMATID_WORDPERFECT_42 = 10,          /**< WordPerfect 4.2. Binding: WordPerfect42 */
	IGR_FORMATID_XYWRITE = 11,                 /**< XYWrite. Binding: XyWrite */
	IGR_FORMATID_WORDSTAR_5 = 12,              /**< Wordstar 5. Binding: WordStar5 */
	IGR_FORMATID_WORDSTAR_2000 = 13,           /**< Wordstar 2000. Binding: WordStar2000 */
	IGR_FORMATID_LOTUS_MANUSCRIPT = 14,        /**< Lotus Manuscript. Binding: LotusManuscript */
	IGR_FORMATID_DISPLAYWRITE_4 = 15,          /**< DisplayWrite 4. Binding: DisplayWrite4 */
	IGR_FORMATID_WANG_IWP = 16,                /**< Wang IWP. Binding: WangIWP */
	IGR_FORMATID_FRAMEWORK_3 = 18,             /**< FrameWork 3. Binding: Framework3 */
	IGR_FORMATID_DCA_FFT = 19,                 /**< IBM FFT. Binding: DCAFFT */
	IGR_FORMATID_RTF = 20,                     /**< Microsoft RTF. Binding: RTF */
	IGR_FORMATID_OPEN_ACCESS_2 = 22,           /**< Open Access II. Binding: OpenAccess2 */
	IGR_FORMATID_DBASE = 23,                   /**< dBASE / FoxPro. Binding: DBase */
	IGR_FORMATID_QA_WRITE = 24,                /**< Q & A Write. Binding: QAWrite */
	IGR_FORMATID_MS_WORD = 25,                 /**< Microsoft Word. Binding: MSWord */
	IGR_FORMATID_DISPLAYWRITE_5 = 26,          /**< DisplayWrite 5. Binding: DisplayWrite5 */
	IGR_FORMATID_WANG_WP_PLUS = 27,            /**< Wang WP PLUS. Binding: WangWPPlus */
	IGR_FORMATID_SIGNATURE = 28,               /**< Internal use only. Unused */
	IGR_FORMATID_MASS11 = 29,                  /**< MASS-11 PC. Binding: Mass11 */
	IGR_FORMATID_AMI_PRO = 30,                 /**< Lotus Ami Pro. Binding: AmiPro */
	IGR_FORMATID_GENERIC_SPREADSHEET = 32,     /**< SpreadSheet/Other. Binding: GenericSpreadsheet */
	IGR_FORMATID_AUTOFORMAT = 33,              /**< Best Guess (of...). Binding: Autoformat */
	IGR_FORMATID_MS_WORKS = 34,                /**< Microsoft Works. Binding: MSWorks */
	IGR_FORMATID_GRAPHIC = 35,                 /**< External Image Viewer. Binding: Graphic */
	IGR_FORMATID_WESTLAW = 36,                 /**< Internal use only. Unused */
	IGR_FORMATID_UNIPLEX = 37,                 /**< Uniplex. Binding: Uniplex */
	IGR_FORMATID_EXTERNAL = 38,                /**< External OEM DLL. Binding: External */
	IGR_FORMATID_TRANSCRIPT = 39,              /**< Transcript. Binding: Transcript */
	IGR_FORMATID_SGML = 40,                    /**< SGML Text Only. Binding: SGML */
	IGR_FORMATID_COMPUSERVE_EMAIL = 43,        /**< Compuserve E-Mail. Binding: CompuServeEmail */
	IGR_FORMATID_ISYS_ANNOTATION = 44,         /**< ISYS Annotation. Binding: ISYSAnnotation */
	IGR_FORMATID_SQL = 45,                     /**< Internal use only. SQL Data Source */
	IGR_FORMATID_WINDOWS_WRITE = 46,           /**< Windows Write. Binding: WindowsWrite */
	IGR_FORMATID_POWERPOINT = 47,              /**< Microsoft PowerPoint. Binding: PowerPoint */
	IGR_FORMATID_MS_WORKS_3 = 48,              /**< MS Works 3. Binding: MSWorks3 */
	IGR_FORMATID_HTML = 49,                    /**< HTML Text Only. Binding: HTML */
	IGR_FORMATID_HTML_PASSTHRU = 50,           /**< HTML Raw Codes. Binding: HTMLPassThru */
	IGR_FORMATID_PDF = 51,                     /**< Adobe Acrobat (PDF). Binding: PDF */
	IGR_FORMATID_EXCEL = 52,                   /**< Microsoft Excel. Binding: Excel */
	IGR_FORMATID_EMAIL = 53,                   /**< Internal use only. E-mail */
	IGR_FORMATID_OUTLOOK_MSG = 54,             /**< Microsoft MSG. Binding: OutlookMSG */
	IGR_FORMATID_DWG = 55,                     /**< AutoDesk AutoCAD. Binding: DWG */
	IGR_FORMATID_HWP = 56,                     /**< Hangul. Binding: HWP */
	IGR_FORMATID_LOTUS_WORD_PRO = 57,          /**< Lotus Word Pro. Binding: LotusWordPro */
	IGR_FORMATID_CHM = 58,                     /**< Microsoft HTML Help (CHM). Binding: CHM */
	IGR_FORMATID_ZIP = 59,                     /**< ZIP Archive. Binding: ZIP */
	IGR_FORMATID_MP3 = 60,                     /**< MP3. Binding: MP3 */
	IGR_FORMATID_LOTUS_NOTES = 63,             /**< Lotus Notes. Binding: LotusNotes */
	IGR_FORMATID_EML = 64,                     /**< Microsoft EML. Binding: EML */
	IGR_FORMATID_MHTML = 65,                   /**< Microsoft Web Archive (MHT). Binding: MHTML */
	IGR_FORMATID_SWF = 66,                     /**< Adobe Shockwave Flash. Binding: SWF */
	IGR_FORMATID_HTML_METAONLY = 67,           /**< HTML Metadata only. Binding: HTMLMetaOnly */
	IGR_FORMATID_VCARD = 68,                   /**< vCard Electronic business card. Binding: VCard */
	IGR_FORMATID_GOLDMINE = 69,                /**< Internal use only. GoldMine */
	IGR_FORMATID_VISIO = 70,                   /**< Microsoft Visio. Binding: Visio */
	IGR_FORMATID_GENERIC_IFILTER = 71,         /**< IFilter. Binding: GenericIFilter */
	IGR_FORMATID_TIFF = 72,                    /**< TIFF Image. Binding: TIFF */
	IGR_FORMATID_JPEG = 73,                    /**< JPEG Exif. Binding: JPEG */
	IGR_FORMATID_TEXT_UTF8 = 74,               /**< Text (UTF8). Binding: TextUTF8 */
	IGR_FORMATID_TEXT_UTF16 = 75,              /**< Text (UTF16). Binding: TextUTF16 */
	IGR_FORMATID_OPENDOCUMENT = 76,            /**< Open Document Format. Binding: OpenDocument */
	IGR_FORMATID_XML = 77,                     /**< XML document file. Binding: XML */
	IGR_FORMATID_PST = 78,                     /**< Microsoft PST/OST. Binding: PST */
	IGR_FORMATID_MBOX = 79,                    /**< Sendmail MBOX. Binding: Mbox */
	IGR_FORMATID_ONENOTE = 80,                 /**< Microsoft OneNote. Binding: OneNote */
	IGR_FORMATID_MS_MDI = 81,                  /**< Microsoft Document Image (MDI). Binding: MSMDI */
	IGR_FORMATID_WINDOWS_MEDIA = 82,           /**< Windows Multimedia (WMA/WMV). Binding: WindowsMedia */
	IGR_FORMATID_XPS = 83,                     /**< Microsoft XPS. Binding: XPS */
	IGR_FORMATID_METADATA_ONLY = 84,           /**< Metadata. Binding: MetadataOnly */
	IGR_FORMATID_LOGFILE = 85,                 /**< Log file. Binding: LogFile */
	IGR_FORMATID_7Z = 86,                      /**< 7ZIP Archive. Binding: SevenZip */
	IGR_FORMATID_GZIP = 87,                    /**< GZ Archive. Binding: GZIP */
	IGR_FORMATID_RAR = 88,                     /**< RAR Archive. Binding: RAR */
	IGR_FORMATID_APPLE_IWORK = 89,             /**< Apple iWork. Binding: AppleIWork */
	IGR_FORMATID_MS_PROJECT = 90,              /**< Microsoft Project. Binding: MSProject */
	IGR_FORMATID_TOXIC_DOC = 91,               /**< ToxicDoc. Binding: ToxicDoc */
	IGR_FORMATID_ICHITARO = 92,                /**< JustSystems Ichitaro. Binding: Ichitaro */
	IGR_FORMATID_ISO_IMAGE = 93,               /**< ISO Disk Image. Binding: ISOImage */
	IGR_FORMATID_CAB = 94,                     /**< Microsoft Cabinet Archive. Binding: CAB */
	IGR_FORMATID_DMG = 95,                     /**< Apple Disk Image. Binding: DMG */
	IGR_FORMATID_HFS = 96,                     /**< HFS Partition. Binding: HFS */
	IGR_FORMATID_BZIP2 = 97,                   /**< BZIP Archive. Binding: Bzip2 */
	IGR_FORMATID_CPIO = 98,                    /**< CPIO UNIX Archive. Binding: CPIO */
	IGR_FORMATID_TAR = 99,                     /**< TAR Archive. Binding: TAR */
	IGR_FORMATID_LZH = 100,                    /**< LZH Archive. Binding: LZH */
	IGR_FORMATID_ARJ = 101,                    /**< ARJ Archive. Binding: ARJ */
	IGR_FORMATID_MS_BINDER = 102,              /**< Microsoft Binder. Binding: MSBinder */
	IGR_FORMATID_RPM = 103,                    /**< RPM Package. Binding: RPM */
	IGR_FORMATID_MS_PUBLISHER = 104,           /**< Microsoft Publisher. Binding: MSPublisher */
	IGR_FORMATID_QUICKBOOKS_BACKUP = 105,      /**< QuickBooks Backup. Binding: QuickBooksBackup */
	IGR_FORMATID_REAL_MEDIA = 106,             /**< Real Media. Binding: RealMedia */
	IGR_FORMATID_FLV = 107,                    /**< Adobe Flash Video. Binding: FLV */
	IGR_FORMATID_3GP = 108,                    /**< 3gp Audio. Binding: ThreeGp */
	IGR_FORMATID_VOB = 109,                    /**< DVD Video Object. Binding: VOB */
	IGR_FORMATID_TTF = 110,                    /**< TrueType Font. Binding: TTF */
	IGR_FORMATID_QUICKBOOKS_FOR_WINDOWS = 111, /**< QuickBooks for Windows. Binding: QuickBooksForWindows */
	IGR_FORMATID_DGN = 112,                    /**< Intergraph/Microstation CAD (DGN). Binding: DGN */
	IGR_FORMATID_MPEG = 113,                   /**< MPEG Video. Binding: MPEG */
	IGR_FORMATID_MP4 = 114,                    /**< MPEG-4 Video. Binding: MP4 */
	IGR_FORMATID_AIFF = 115,                   /**< AIFF. Binding: AIFF */
	IGR_FORMATID_OGG_VORBIS = 116,             /**< Ogg Vorbis Audio. Binding: OggVorbis */
	IGR_FORMATID_OGG_FLAC = 117,               /**< Ogg Flac Audio. Binding: OggFLAC */
	IGR_FORMATID_BMP = 118,                    /**< Microsoft Bitmap (BMP). Binding: BMP */
	IGR_FORMATID_MS_ACCESS = 119,              /**< Microsoft Access Database (MDB). Binding: MSAccess */
	IGR_FORMATID_PCX = 120,                    /**< PCX Image. Binding: PCX */
	IGR_FORMATID_GIF = 121,                    /**< GIF Image (GIF). Binding: GIF */
	IGR_FORMATID_PNG = 122,                    /**< PNG Image (PNG). Binding: PNG */
	IGR_FORMATID_PSD = 123,                    /**< Adobe Photoshop Image (PSD). Binding: PSD */
	IGR_FORMATID_WAV = 124,                    /**< WAVE Audio. Binding: WAV */
	IGR_FORMATID_AVI = 125,                    /**< AVI Video. Binding: AVI */
	IGR_FORMATID_QUICKTIME = 126,              /**< Apple QuickTime. Binding: QuickTime */
	IGR_FORMATID_MIDI = 127,                   /**< MIDI Audio. Binding: MIDI */
	IGR_FORMATID_WMF = 128,                    /**< Windows Metafile (WMF). Binding: WMF */
	IGR_FORMATID_EMF = 129,                    /**< Windows Enhanced Metafile (EMF). Binding: EMF */
	IGR_FORMATID_POSTSCRIPT = 130,             /**< PostScript (PS). Binding: PostScript */
	IGR_FORMATID_ADOBE_INDESIGN = 131,         /**< Adobe InDesign. Binding: AdobeInDesign */
	IGR_FORMATID_ADOBE_ILLUSTRATOR = 132,      /**< Adobe Illustrator. Binding: AdobeIllustrator */
	IGR_FORMATID_WINDOWS_EXECUTABLE = 133,     /**< Windows EXE/DLL (EXE,DLL). Binding: WindowsExecutable */
	IGR_FORMATID_ELF = 134,                    /**< Linux ELF. Binding: ELF */
	IGR_FORMATID_BINHEX = 135,                 /**< BinHex Encoded. Binding: BinHex */
	IGR_FORMATID_DBASE_FILE = 136,             /**< Internal use only. dBASE Database */
	IGR_FORMATID_PARADOX = 137,                /**< Paradox Database. Binding: Paradox */
	IGR_FORMATID_STUFFIT = 138,                /**< StuffIt Archive. Binding: StuffIt */
	IGR_FORMATID_CORELDRAW = 139,              /**< Corel Draw. Binding: CorelDRAW */
	IGR_FORMATID_STUFFITX = 140,               /**< StuffIt X. Binding: StuffItX */
	IGR_FORMATID_QUARKXPRESS = 141,            /**< Quark XPress. Binding: QuarkXPress */
	IGR_FORMATID_MSI = 142,                    /**< Microsoft Installer (MSI). Binding: MSI */
	IGR_FORMATID_ACE = 143,                    /**< ACE Archive. Binding: ACE */
	IGR_FORMATID_MACBINARY = 144,              /**< Apple Bin File. Binding: MacBinary */
	IGR_FORMATID_STUFFIT_SEA = 145,            /**< StuffIt Self Extracting Archive. Binding: StuffItSEA */
	IGR_FORMATID_IFO = 146,                    /**< DVD IFO (IFO). Binding: IFO */
	IGR_FORMATID_MS_MOVIE_MAKER = 147,         /**< Microsoft Movie Maker. Binding: MSMovieMaker */
	IGR_FORMATID_MATHCAD = 148,                /**< MathCAD. Binding: Mathcad */
	IGR_FORMATID_TORRENT = 149,                /**< BitTorrent Metafile. Binding: Torrent */
	IGR_FORMATID_LNK = 150,                    /**< Shortcut/Link. Binding: LNK */
	IGR_FORMATID_DXF = 151,                    /**< AutoCAD DXF. Binding: DXF */
	IGR_FORMATID_EXCEL_FULLTEXT = 152,         /**< Microsoft Excel (full text). Binding: ExcelFullText */
	IGR_FORMATID_OUTLOOK_EXPRESS_DBX = 153,    /**< Outlook Express mailbox. Binding: OutlookExpressDBX */
	IGR_FORMATID_ONENOTE_TOC = 154,            /**< Microsoft OneNote TOC. Binding: OneNoteTOC */
	IGR_FORMATID_TNEF = 155,                   /**< Microsoft Outlook RTF. Binding: TNEF */
	IGR_FORMATID_DICOM = 156,                  /**< DICOM. Binding: DICOM */
	IGR_FORMATID_TEXT_ISYS_MARKUP = 157,       /**< Text with ISYS markup. Binding: TextISYSMarkup */
	IGR_FORMATID_JAVA_CLASS = 158,             /**< Java Class (CLASS). Binding: JavaClass */
	IGR_FORMATID_ALZ = 159,                    /**< ALZ Archive (ALZ). Binding: ALZ */
	IGR_FORMATID_EGG = 160,                    /**< EGG Archive (EGG). Binding: EGG */
	IGR_FORMATID_NDOC = 161,                   /**< Internal use only. NDOC File (NDOC) */
	IGR_FORMATID_ISYS_TRANSACTION_FILE = 162,  /**< ISYS Transaction File (ISTX). Binding: ISYSTransactionFile */
	IGR_FORMATID_ISYS_INDEX_FILE = 163,        /**< ISYS Index File (ISYS.IX*). Binding: ISYSIndexFile */
	IGR_FORMATID_VCALENDAR = 164,              /**< VCalendar Electronic Calendar. Binding: VCalendar */
	IGR_FORMATID_SYMBIAN_SIS = 165,            /**< Symbian OS File. Binding: SymbianSIS */
	IGR_FORMATID_FITS = 166,                   /**< FITS Image. Binding: FITS */
	IGR_FORMATID_HANCELL = 167,                /**< Hancom Office Hancell. Binding: Hancell */
	IGR_FORMATID_UNIX_COMPRESS = 168,          /**< Compress Archive (.Z). Binding: UnixCompress */
	IGR_FORMATID_UUENCODE = 169,               /**< UU Encoded File. Binding: UUEncode */
	IGR_FORMATID_JUNGUM = 170,                 /**< JungUm File (.GUL). Binding: JungUm */
	IGR_FORMATID_TEXT_MBCS = 171,              /**< Text (Multibyte). Binding: TextMBCS */
	IGR_FORMATID_TEXT_UTF32 = 172,             /**< Text (UTF32). Binding: TextUTF32 */
	IGR_FORMATID_WINDOWS_OS_FILE = 173,        /**< Microsoft Windows Binary. Binding: WindowsOSFile */
	IGR_FORMATID_EMPTY_FILE = 174,             /**< Empty File. Binding: EmptyFile */
	IGR_FORMATID_CORRUPT_FILE = 175,           /**< Corrupt File. Binding: CorruptFile */
	IGR_FORMATID_MXF = 176,                    /**< Material Exchange Format (MXF). Binding: MXF */
	IGR_FORMATID_WEBP = 177,                   /**< WEBP Image. Binding: WebP */
	IGR_FORMATID_ICO = 178,                    /**< ICON Image. Binding: ICO */
	IGR_FORMATID_UNIX_AR = 179,                /**< Unix AR Archive. Binding: UnixAr */
	IGR_FORMATID_STARVIEW_METAFILE = 180,      /**< StarView Metafile (SVM). Binding: StarViewMetafile */
	IGR_FORMATID_NETPBM = 181,                 /**< Netpbm (PPM, PBM, PGM, PNM). Binding: Netpbm */
	IGR_FORMATID_XZ = 182,                     /**< XZ Archive. Binding: XZ */
	IGR_FORMATID_ISYS_DOCPACK = 183,           /**< DocPack. Binding: ISYSDocPack */
	IGR_FORMATID_PICT = 184,                   /**< Macintosh PICT Image. Binding: PICT */
	IGR_FORMATID_SVG = 185,                    /**< Scalable Vector Graphic. Binding: SVG */
	IGR_FORMATID_MS_WORD_FOR_MAC = 186,        /**< Microsoft Word for Mac. Binding: MSWordForMac */
	IGR_FORMATID_DCX = 187,                    /**< DCX Image. Binding: DCX */
	IGR_FORMATID_TGA = 188,                    /**< Targa Image. Binding: TGA */
	IGR_FORMATID_EPS = 189,                    /**< EPS Image. Binding: EPS */
	IGR_FORMATID_JPEG2000 = 190,               /**< JPEG2000 Image. Binding: JPEG2000 */
	IGR_FORMATID_CALS_RASTER = 191,            /**< CALS Raster Image. Binding: CALSRaster */
	IGR_FORMATID_GEM_RASTER = 192,             /**< GEM Image. Binding: GEMRaster */
	IGR_FORMATID_WBMP = 193,                   /**< WBMP Image. Binding: WBMP */
	IGR_FORMATID_IFF = 194,                    /**< IFF Image. Binding: IFF */
	IGR_FORMATID_APPLE_PAGES = 195,            /**< Apple Pages. Binding: ApplePages */
	IGR_FORMATID_COREL_PRESENTATIONS = 196,    /**< Corel Presentation. Binding: CorelPresentations */
	IGR_FORMATID_CATIA = 197,                  /**< CATIA CAD. Binding: CATIA */
	IGR_FORMATID_STL_BINARY = 198,             /**< Stereolithography CAD (Binary). Binding: STLBinary */
	IGR_FORMATID_STL_TEXT = 199,               /**< Stereolithography CAD (Text). Binding: STLText */
	IGR_FORMATID_STEP = 200,                   /**< STEP 3D CAD. Binding: STEP */
	IGR_FORMATID_IGES = 201,                   /**< IGS CAD. Binding: IGES */
	IGR_FORMATID_PROE_MODEL = 202,             /**< Pro/ENGINEER Model. Binding: ProEModel */
	IGR_FORMATID_SOLIDWORKS = 203,             /**< SolidWorks. Binding: SolidWorks */
	IGR_FORMATID_JT = 204,                     /**< JT CAD. Binding: JT */
	IGR_FORMATID_PARASOLID = 205,              /**< Parasolid Model Part. Binding: Parasolid */
	IGR_FORMATID_PROE_ASSEMBLY = 206,          /**< Pro/ENGINEER Assembly. Binding: ProEAssembly */
	IGR_FORMATID_PROE_DRAWING = 207,           /**< Pro/ENGINEER Drawing. Binding: ProEDrawing */
	IGR_FORMATID_PROE_DRAWING_FORM = 208,      /**< Pro/ENGINEER Drawing Form. Binding: ProEDrawingForm */
	IGR_FORMATID_WAVEFRONT_OBJ = 209,          /**< OBJ 3D Image. Binding: WavefrontOBJ */
	IGR_FORMATID_DWF = 210,                    /**< Autodesk WHIP. Binding: DWF */
	IGR_FORMATID_APPLE_KEYNOTE = 211,          /**< Apple Keynote. Binding: AppleKeynote */
	IGR_FORMATID_APPLE_NUMBERS = 212,          /**< Apple Numbers. Binding: AppleNumbers */
	IGR_FORMATID_BROOKTROUT_FAX = 213,         /**< Brooktrout Fax. Binding: BrooktroutFax */
	IGR_FORMATID_JEDMICS_C4 = 214,             /**< JEDMICS C4. Binding: JedmicsC4 */
	IGR_FORMATID_MSO_OBJECT = 215,             /**< MSO Object. Binding: MSOObject */
	IGR_FORMATID_IMNET_RASTER = 216,           /**< IMNET Medical. Binding: IMNETRaster */
	IGR_FORMATID_IMNET_COLD = 217,             /**< IMNET COLD. Binding: IMNETCold */
	IGR_FORMATID_MS_PAINT = 218,               /**< MS Paint Image. Binding: MSPaint */
	IGR_FORMATID_MACPAINT = 219,               /**< MAC Paint Image. Binding: MacPaint */
	IGR_FORMATID_FLASHPIX = 220,               /**< FlashPix. Binding: FlashPix */
	IGR_FORMATID_CGM = 221,                    /**< Computer Graphics Metafile. Binding: CGM */
	IGR_FORMATID_XXENCODE = 222,               /**< Xxencoded File. Binding: XXEncode */
	IGR_FORMATID_DOMINO_XML = 223,             /**< Domino XML. Binding: DominoXML */
	IGR_FORMATID_NCR_IMAGE = 224,              /**< NCR Image. Binding: NCRImage */
	IGR_FORMATID_MS_CLIPBOARD = 225,           /**< Microsoft Windows Clipboard. Binding: MSClipboard */
	IGR_FORMATID_APPLE_PLIST = 226,            /**< Apple Property List (Binary). Binding: ApplePlist */
	IGR_FORMATID_EPUB = 227,                   /**< Electronic Publication. Binding: EPUB */
	IGR_FORMATID_APPLE_IBOOKS = 228,           /**< Apple iBooks Publication. Binding: AppleIBooks */
	IGR_FORMATID_QUATTRO_PRO = 229,            /**< Quattro Pro Spreadsheet. Binding: QuattroPro */
	IGR_FORMATID_OUTLOOK_MAC_OLM = 230,        /**< Outlook for Mac OLM. Binding: OutlookMacOLM */
	IGR_FORMATID_OUTLOOK_MAC_MSG_SOURCE = 231, /**< Outlook for Mac Message Source. Binding: OutlookMacMSGSource */
	IGR_FORMATID_OUTLOOK_MAC_MESSAGE = 232,    /**< Outlook for Mac Message. Binding: OutlookMacMessage */
	IGR_FORMATID_WORDPERFECT_GRAPHICS = 233,   /**< WordPerfect Graphics. Binding: WordPerfectGraphics */
	IGR_FORMATID_CUR = 234,                    /**< Windows Cursor Image. Binding: CUR */
	IGR_FORMATID_XBM = 235,                    /**< X-Windows Bitmap. Binding: XBM */
	IGR_FORMATID_XPM = 236,                    /**< X-Windows Pixmap. Binding: XPM */
	IGR_FORMATID_PAINTSHOPPRO = 237,           /**< Paintshop Pro. Binding: PaintShopPro */
	IGR_FORMATID_DWG_2018 = 238,               /**< AutoDesk AutoCAD 2018. Binding: DWG2018 */
	IGR_FORMATID_SQLITE = 239,                 /**< SQLite DB. Binding: SQLite */
	IGR_FORMATID_DOCUWORKS = 240,              /**< Fuji Xerox DocuWorks. Binding: DocuWorks */
	IGR_FORMATID_SGI_IMAGE = 241,              /**< SGI Image File. Binding: SGIImage */
	IGR_FORMATID_EOT = 242,                    /**< MS Embedded OpenType fonts. Binding: EOT */
	IGR_FORMATID_PHOTO_CD = 243,               /**< Kodak Photo CD. Binding: PhotoCD */
	IGR_FORMATID_AFP = 244,                    /**< AFP MO:DCA. Binding: AFP */
	IGR_FORMATID_THUMBS_DB = 245,              /**< Windows Thumbnail Cache. Binding: ThumbsDB */
	IGR_FORMATID_VS_USER_OPTIONS = 246,        /**< VS Solution User Options. Binding: VSUserOptions */
	IGR_FORMATID_APPLE_DOUBLE = 247,           /**< AppleDouble. Binding: AppleDouble */
	IGR_FORMATID_PROCMON_LOG = 248,            /**< Process Monitor Log. Binding: ProcMonLog */
	IGR_FORMATID_WINDOWS_RESOURCE = 249,       /**< Resource File. Binding: WindowsResource */
	IGR_FORMATID_SUN_RASTER = 250,             /**< Sun Raster Image (RAS). Binding: SunRaster */
	IGR_FORMATID_PCL = 251,                    /**< Printer Command Language. Binding: PCL */
	IGR_FORMATID_SCITEX_CT = 252,              /**< Scitex CT Image(CT). Binding: ScitexCT */
	IGR_FORMATID_CRW = 253,                    /**< Canon Raw Image. Binding: CRW */
	IGR_FORMATID_XWD = 254,                    /**< X Window System Screen Dump. Binding: XWD */
	IGR_FORMATID_NOCANOPEN = 255,              /**< Cannot Open. Binding: NoCanOpen */
	IGR_FORMATID_OFFICE_ENCRYPTED = 256,       /**< Password Protected Office File. Binding: OfficeEncrypted */
	IGR_FORMATID_ZIP_SFX = 257,                /**< ZIP Archive (SFX). Binding: ZIPSFX */
	IGR_FORMATID_RAR_SFX = 258,                /**< RAR Archive (SFX). Binding: RARSFX */
	IGR_FORMATID_7Z_SFX = 259,                 /**< 7ZIP Archive (SFX). Binding: SevenZipSFX */
	IGR_FORMATID_PSB = 260,                    /**< Adobe Photoshop Large Document Format (PSB). Binding: PSB */
	IGR_FORMATID_WORDPERFECT_FOR_MAC_1 = 261,  /**< WordPerfect for Mac 1. Binding: WordPerfectForMac1 */
	IGR_FORMATID_CR2 = 262,                    /**< Canon Camera Raw Image 2. Binding: CR2 */
	IGR_FORMATID_CR3 = 263,                    /**< Canon Camera Raw Image 3. Binding: CR3 */
	IGR_FORMATID_PDF_XFA_XML = 264,            /**< PDF XFA XML. Binding: PDFXFAXML */
	IGR_FORMATID_HWPX = 265,                   /**< Hancom Hangul HWPX. Binding: HWPX */
	IGR_FORMATID_CFB = 266,                    /**< Microsoft Compound Binary File. Binding: CFB */
	IGR_FORMATID_CFB_CORRUPT = 267,            /**< Microsoft Compound Binary File (Corrupted). Binding: CFBCorrupt */
	IGR_FORMATID_ZIP_CORRUPT = 268,            /**< ZIP Archive (Corrupted). Binding: ZIPCorrupt */
	IGR_FORMATID_JSON = 269,                   /**< JSON. Binding: JSON */
	IGR_FORMATID_HL7_CONTAINER = 270,          /**< HL7 Container. Binding: HL7Container */
	IGR_FORMATID_HL7_MESSAGE = 271,            /**< HL7 Message. Binding: HL7Message */
	IGR_FORMATID_YAML = 272,                   /**< YAML. Binding: YAML */
	IGR_FORMATID_MARKDOWN = 273,               /**< Markdown. Binding: Markdown */
	IGR_FORMATID_CSV = 274,                    /**< Comma-Separated Values. Binding: CSV */
	IGR_FORMATID_TSV = 275                     /**< Tab-Separated Values. Binding: TSV */
} IGR_FORMATID;

#endif /* HYLAND_DOCFILTERS_FORMATS_H_DEF */
