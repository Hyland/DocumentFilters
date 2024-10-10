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
* Document Filters Example - Convert a document to classic HTML
****************************************************************************/

#include <DocumentFiltersObjects.h>
#include <DocumentFiltersSamples.h>
#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>

namespace DF = Hyland::DocFilters;

void process_file(DF::Extractor& doc, const std::filesystem::path& out_filename)
{
	static const size_t chunk_size = 4096;

	auto&& out_folder = out_filename.parent_path();

	// Setup a password prompt handler...
	DocumentFiltersSamples::handle_password_prompt(doc);

	// Open the document...
	doc.Open(DF::OpenMode::ClassicHtml);

	// Extract the text...
	std::cerr << " - Extracting HTML to " << out_filename << std::endl;

	std::ofstream out_stream(out_filename, std::ios::binary);
	while (!doc.getEOF())
	{
		auto chunk = DF::w_to_u8(doc.getText(chunk_size, true));
		out_stream.write(chunk.data(), chunk.size());
	}

	// Extract the embedded images...
	for (auto&& img : doc.images())
	{
		auto img_filename = out_folder / img.getName();
		std::cerr << " - Extracting image to " << img_filename << std::endl;
		img.CopyTo(img_filename.wstring());
	}
}

void process_file(DF::Api& api,	std::string filename, std::string output)
{
	auto&& doc = api.GetExtractor(filename);

	std::cerr << filename << " [" 
		<< doc.getFileType() << ", " 
		<< DF::w_to_u8(doc.getFileType(DF::Format::What::LongName)) << "]" << std::endl;

	process_file(doc, output);
}


int main(int argc, char *argv[])
{
	CLI::App app("Hyland Document Filters: ConvertDocumentToClassicHTML");

	try {
		std::string filename;
		std::string output;
		std::string license_key;

		app.add_option("filename", filename, "Filename of file to convert")->required();
		app.add_option("-o,--output", output, "Output file to create");
		app.add_option("-l,--license", license_key, "license key for Document Filters");
		app.parse(argc, argv);

		if (output.empty())
			output = std::filesystem::path(filename).stem().string() + ".html";
		
		DF::Api api(DocumentFiltersSamples::get_license_key(license_key), ".");
		process_file(api, filename, output);
	}
	catch (const CLI::ParseError& e) {
		return app.exit(e);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
