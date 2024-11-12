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

#include <DocumentFiltersObjects.h>
#include <DocumentFiltersSamples.h>
#include <CLI/CLI.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <map>

namespace DF = Hyland::DocFilters;

struct options_t {
	std::string license_key;
	std::string file_name;
	std::vector<std::string> include_files;
	bool recurse = false;
	int max_depth = INT32_MAX;
	bool list = false;
	std::string dir = ".";
	bool flatten = false;
};

void process_file(DF::Extractor& extractor, const std::string& fileName, const options_t& options, int depth) {

	DocumentFiltersSamples::handle_password_prompt(extractor);

	std::cerr << "Container: " << fileName << std::endl;
	try {
		extractor.Open(DF::OpenMode::Text, IGR_BODY_AND_META);

		for (auto&& subfile : extractor.subfiles())
		{
			if (options.list) {
				auto dt = subfile.getFileDate().ToTm(true);

				std::ostringstream oss;
				if (dt.has_value() && dt.value().tm_year > 70)
					oss << std::put_time(&dt.value(), "%d %b %Y %I:%M:%S %p");
				else
					oss << "<...no date...>";

				std::cout << oss.str() << " " << subfile.getSize() << " " << DF::w_to_u8(subfile.getName()) << std::endl;
			}
			else {
				// Extract
				std::cout << "  extracting: " << DF::w_to_u8(subfile.getName()) << std::endl;

				// Normalize slashes to the host's directory separator
				std::string destFileName = DF::w_to_u8(subfile.getName());
				std::replace_if(destFileName.begin(), destFileName.end(), [](char c) { return c == '/' || c == '\\'; }, static_cast<char>(std::filesystem::path::preferred_separator));

				// If flattening, change the slashes to underscores
				if (options.flatten)
					std::replace(destFileName.begin(), destFileName.end(), static_cast<char>(std::filesystem::path::preferred_separator), '_');

				// Get the absolute path to the output
				destFileName = std::filesystem::absolute(std::filesystem::path(options.dir) / destFileName).string();

				// Make sure it's not a malformed file trying to write outside of the output dir.
				if (destFileName.find(std::filesystem::absolute(options.dir).string()) != 0)
					destFileName = std::filesystem::absolute(std::filesystem::path(options.dir) / std::filesystem::path(destFileName).filename()).string();

				std::filesystem::create_directories(std::filesystem::path(destFileName).parent_path());
				subfile.CopyTo(DF::u8_to_w(destFileName));
			}

			if (options.recurse && depth <= options.max_depth)
				process_file(subfile, fileName + "|" + DF::w_to_u8(subfile.getName()), options, depth + 1);
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

int main(int argc, char* argv[]) {
	CLI::App app("Hyland Document Filters: Extractor");
	try {

		options_t options;

		app.add_option("--license", options.license_key, "license key for Document Filters");
		app.add_option("file_name", options.file_name, "Container file to process")->required();
		app.add_option("include_files", options.include_files, "Patterns of files to extract");
		app.add_flag("-r,--recurse", options.recurse, "Recursively process extracted subfiles");
		app.add_option("--max-depth", options.max_depth, "The maximum depth subfiles should be recursed into");
		app.add_flag("-l,--list", options.list, "List subfiles");
		app.add_option("-d,--dir", options.dir, "Extract files into directory");
		app.add_flag("--flatten-dirs", options.flatten, "Flatten directory names in output");

		CLI11_PARSE(app, argc, argv);

		DF::Api api(DocumentFiltersSamples::get_license_key(options.license_key), ".");

		DF::Extractor file = api.GetExtractor(options.file_name);
		process_file(file, options.file_name, options, 0);
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

