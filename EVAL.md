# Document Filters Evaluation

Document Filters may be used without a license key, in a feature-limited evaluation.  This is the easiest way to try Document Filters.

When ready, you may send an email to DocFiltersEval@hyland.com to request a license key for a time-limited evaluation.

| Mode                       | License Key  | Features | Expiration |
| -------------------------- | ------------ | -------- | ---------- |
| Full Featured              | REQUIRED     | ALL      | *Optional* |
| Time-Limited Evaluation    | REQUIRED     | ALL      | REQUIRED   |
| Feature-Limited Evaluation | NONE         | LIMITED  | NONE       |

## Feature-Limited Evaluation

When initializing Document Filters in your code, provide an empty string ("") for the license key.

The feature-limited evaluation has the following limitations:
- Processing files (outside of file type identification) is only supported for files of type:
  - Text
  - PDF
  - Microsoft Office (Word, Excel, Powerpoint)
  - OpenDocument Format (ODF)
- Output formats are limited to:
  - Text
  - PDF
  - Raster image (e.g. PNG, JPEG, GIF, WEBP, BMP, TIFF)
- Each page of output contains an "Evaluation Copy" watermark

File type identification is not impacted.  For example, the functions IGR_Get_File_Type() and IGR_Get_Stream_Type() in the C API are not impacted.

## Time-Limited Evaluation

A time-limited evaluation allows you to use all of the features of Document Filters until the expiration of the evaluation license key.

Send an email to DocFiltersEval@hyland.com to request a license key for a time-limited evaluation.