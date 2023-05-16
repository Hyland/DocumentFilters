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
* Document Filters Example - Convert a document to Interactive HD HTML
*
* This example creates a html view of a document which can be manipulated by
* javascript to provide more functionality than a standard static page.
*
* There are two parts to the sample, the page generate logic use standard
* Document Filters HDHTML exports to create the page data. The sample then
* injects CSS and Javascript to provide extra viewing capabilities, such as
* Zoom, Rotate, Highlight, Redact and Metadata Preview.
*
****************************************************************************/

#include "DocumentFilters.h"
#include "DocumentFiltersObjects.h"
#include "DocumentFiltersSamples.h"

#include <iostream>
#include <sstream>
#include <exception>
#include <memory>
#include <cstring>
#include <sys/stat.h>

Perceptive::DocumentFilters DocumentFilters;

void process_file(const std::string &filename, const std::string &OutFilename, bool Async)
{
	std::cerr << "Processing (FILE): " << filename << std::endl;
	std::auto_ptr<Perceptive::Extractor> Extractor(DocumentFilters.GetExtractor(filename));

	std::string OptionsMain;
	std::string OptionsPage;

	OptionsMain = OptionsPage =
		"HTML_INLINE_IMAGES=on;"
		"HDHTML_INCLUDE_WORD_INDEXES=on;";
	// HTML_INLINE_IMAGES forces any images to be inlined directly into the HTML using url(data:image/png;base64,
	// rather than having external files. 
	//
	// HDHTML_INCLUDE_WORD_INDEXES will include information about each word in the run data for a page. This is 
	// used by the javascript to map between the web browsers co-ordinate system and the word co-ordinate system
	// used by Document Filters. In this example, it's used by the redaction function.

	OptionsMain += "HDHTML_OUTPUT_INJECT_HEAD=@perceptive-viewer-inject.html;";
	// HDHTML_OUTPUT_INJECT_HEAD will load the contents of the perceptive-viewer-inject.html file and place it
	// at the bottom of the <HEAD> section of the main HTML page. This allows us to inject stylesheets, javascript
	// and extra metadata that will be loaded when the page is viewed.

	OptionsPage += "HDHTML_OUTPUT_BOILERPLATE=off";
	// HDHTML_OUTPUT_BOILERPLATE disables the surrounding <html>...<body> tags that typically get generated into
	// the HTML output.  This is used when generateing ASYNC pages, which must only contain the actual page data
	// and not the surrounding html.


	int DocType = Extractor->getFileType();
	std::cerr << "DocType: " << DocType << ", " << Perceptive::GetFileTypeName(DocType)
		<< ", SupportsText: " << Extractor->getSupportsText()
		<< ", SupportsSubFiles: " << Extractor->getSupportsSubFiles() << std::endl;
	Extractor->Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE);

	std::auto_ptr<Perceptive::Stream> OutputStream(new Perceptive::FileStream(OutFilename, "wb+"));
	std::auto_ptr<Perceptive::Canvas> Canvas(DocumentFilters.MakeOutputCanvas(OutputStream.get(), IGR_DEVICE_HTML, OptionsMain));
	std::auto_ptr<Perceptive::Page> Page;

	int pageNum = 0;
	for (Page.reset(Extractor->GetFirstPage()); Page.get(); Page.reset(Extractor->GetNextPage()))
	{
		std::cerr << "Rendering Page " << ++pageNum << std::endl;

		// When in Async mode, always render the first page directly into the main output
		if (!Async || pageNum == 1)
			Canvas->RenderPage(Page.get());
		else
		{
			// Render page stubs instead of the actual page content
			int w = Page->getWidth();
			int h = Page->getHeight();

			std::stringstream pageExtension;
			pageExtension << "_page_" << pageNum << ".html";

			std::string PageFilename = Perceptive::ChangeFileExt(OutFilename, pageExtension.str());

			std::stringstream html;
			html << "<div class=\"idf-page\" style=\"width: " << w << "px; height: " << h << "px; border: 1px solid black; overflow:hidden;\" title=\"" << Perceptive::ExtractFileName(PageFilename) << "\">";
			html << "</div>" << std::endl;

			std::string htmlOutput = html.str();
			OutputStream->Write((void*)htmlOutput.c_str(), (ULONG)htmlOutput.size());

			// Render page to it's own file
			std::auto_ptr<Perceptive::Canvas> PageCanvas(DocumentFilters.MakeOutputCanvas(PageFilename, IGR_DEVICE_HTML, OptionsPage));
			PageCanvas->RenderPage(Page.get());
		}
	}

	std::string OutDir = Perceptive::ExtractFilePath(OutFilename);
	Perceptive::CopyFile("perceptive-viewer-utils.js", OutDir + "perceptive-viewer-utils.js", false);
	Perceptive::CopyFile("perceptive-viewer-utils.css", OutDir + "perceptive-viewer-utils.css", false);
}

void usage(const char* process_name)
{
	std::cerr << "Document Filters: ConvertDocumentToInteractiveHDHTML C++ Example" << std::endl;
	std::cerr << "(c) 2023 Hyland Software, Inc. and its affiliates. All rights reserved." << std::endl;
	std::cerr << "ALL RIGHTS RESERVED" << std::endl << std::endl;
	std::cerr << "Usage: " << process_name << " [options] DocFilename OutFilename" << std::endl << std::endl;
	std::cerr << "Options" << std::endl;
	std::cerr << "    -a, --async        Generate HTML that loads page data on demand. The generated output " << std::endl;
	std::cerr << "                       MUST be loaded into a web server." << std::endl << std::endl;
	std::cerr << "                       The page will fail to load using the file:// protocol" << std::endl;
}

int main(int argc, char *argv[])
{
	try
	{
		DocumentFilters.Initialize(DocumentFiltersSamples::get_license_key(), ".");

		// Ensure that the required viewer redistributables findable
		if (!Perceptive::FileExists("perceptive-viewer-utils.js")
			|| !Perceptive::FileExists("perceptive-viewer-utils.css")
			|| !Perceptive::FileExists("perceptive-viewer-inject.html"))
		{
			std::cerr << "Error: redistributable files are missing from application directory" << std::endl
				<< " - perceptive-viewer-utils.js" << std::endl
				<< " - perceptive-viewer-utils.css" << std::endl
				<< " - perceptive-viewer-inject.html" << std::endl;
			return 1;
		}

		// Command line parsing
		const char *SourceFile = NULL, *OutputFile = NULL;
		bool Async = false;

		for (int i = 1; i < argc; i++)
		{
			if (argv[i][0] == '-')
			{
				if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--async") == 0)
					Async = true;
				else
					std::cerr << "Unknown option: " << argv[i] << std::endl;
			}
			else if (SourceFile == NULL)
			{
				SourceFile = argv[i];
			}
			else if (OutputFile == NULL)
			{
				OutputFile = argv[i];
			}
			else
			{
				std::cerr << "Unexpected command line argument: " << argv[i] << std::endl;
				return 1;
			}
		}

		if (SourceFile == NULL || OutputFile == NULL)
		{
			usage(argv[0]);
			return 1;
		}

		process_file(SourceFile, OutputFile, Async);
		return 0;
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
