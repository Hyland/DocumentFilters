/*
(c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved.

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
* Document Filters Example - Convert a document to UTF8 text
****************************************************************************/

#include <DocumentFiltersObjects.h>
#include <DocumentFiltersSamples.h>
#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>

namespace DF = Hyland::DocFilters;

struct options_t
{
	std::string filename;
	std::string output;
	std::string license_key;
	bool subfiles = false;
	bool keep_control_codes = false;
};

void process_file(DF::Extractor& doc, const options_t& options, std::ostream& dest)
{
	static const size_t chunk_size = 4096;

	// Setup a password prompt handler...
	DocumentFiltersSamples::handle_password_prompt(doc);

	// Open to document...
	doc.Open(DF::OpenMode::Text, IGR_BODY_AND_META, L"OCR=ON;OCR_REORIENT_PAGES=ON");

	// Extract the text...
	while (!doc.getEOF())
	{
		auto chunk = DF::w_to_u8(doc.getText(chunk_size, !options.keep_control_codes));
		dest.write(chunk.data(), chunk.size());
	}

	if (options.subfiles)
	{
		// Extract the subfiles...
		for (auto&& sub : doc.subfiles())
		{
			process_file(sub, options, dest);
		}
	}
}

void process_file(DF::Api& api, const options_t& options)
{
	static const uint8_t utf8_byte_order_marker[] = { 0xEF, 0xBB, 0xBF };

	auto&& doc = api.GetExtractor(options.filename);

	std::cerr << options.filename << " ["
		<< doc.getFileType() << ", "
		<< DF::w_to_u8(doc.getFileType(DF::Format::What::LongName)) << "]" << std::endl;

	if (options.output.empty())
		process_file(doc, options, std::cout);
	else
	{
		std::ofstream dest;
		dest.open(options.output, std::ios::binary | std::ios::out);
		dest.write((const char*)&utf8_byte_order_marker[0], sizeof(utf8_byte_order_marker));
		process_file(doc, options, dest);
	}
}

int main(int argc, char* argv[])
{
	CLI::App app("Hyland Document Filters: ConvertDocumentToUTF8WithOCR");

	try {
		options_t options;

		app.add_option("filename", options.filename, "Filename of file to convert")->required();
		app.add_option("-o,--output", options.output, "Output file to create");
		app.add_option("-l,--license", options.license_key, "License key for Document Filters");
		app.add_flag("-s,--subfiles", options.subfiles, "Also extract subfiles");
		app.add_flag("-k,--keep-control-codes", options.keep_control_codes, "Keep DocFilters control codes in output");
		app.parse(argc, argv);

		DF::Api api(DocumentFiltersSamples::get_license_key(options.license_key), ".");

		process_file(api, options);
	}
	catch (const CLI::ParseError& e) {
		return app.exit(e);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
}