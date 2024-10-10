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
* Document Filters Example - Get the document type
****************************************************************************/

#include <DocumentFiltersObjects.h>
#include <DocumentFiltersSamples.h>
#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>

namespace DF = Hyland::DocFilters;

void process_file(DF::Api& api, const std::filesystem::path &filename)
{
	if (std::filesystem::is_directory(filename))
	{
		for (auto&& entry : std::filesystem::directory_iterator(filename))
			process_file(api, entry.path());
	}
	else
	{
		std::cout << filename.string() << std::endl;

		auto&& extractor = api.GetExtractor(filename);
		auto doc_type = extractor.getFileType();
		auto doc_type_name = DF::w_to_u8(extractor.getFileType(DF::Format::What::LongName));

		std::cout 
			<< "Format Code: " << doc_type << std::endl
			<< "Format Name: " << DF::w_to_u8(extractor.getFileType(DF::Format::What::LongName)) << std::endl
			<< "Config Name: " << DF::w_to_u8(extractor.getFileType(DF::Format::What::ConfigName)) << std::endl
			<< "  Mime Type: " << DF::w_to_u8(extractor.getFileType(DF::Format::What::MimeType)) << std::endl
			<< std::endl;
	}
}

int main(int argc, char* argv[])
{
	CLI::App app("Hyland Document Filters: GetDocumentType");

	try {
		std::vector<std::string> filenames;
		std::string output;
		std::string license_key;

		app.add_option("filename", filenames, "Filename of file to convert")->required();
		app.add_option("-l,--license", license_key, "license key for Document Filters");
		app.parse(argc, argv);

		DF::Api api(DocumentFiltersSamples::get_license_key(license_key), ".");
		for (auto&& filename : filenames)
			process_file(api, filename);
	}
	catch (const CLI::ParseError& e) {
		return app.exit(e);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
}
