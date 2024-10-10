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
#include <iostream>
#include <string>
#include <stdexcept>

namespace DF = Hyland::DocFilters;

enum class BarCodeType { Qr, Dm, Pdf417, Code39, Code93, Code128, GS1 };

struct options_t {
	std::string license_key;
	std::string text;
	BarCodeType type = BarCodeType::Qr;
	std::string caption;
	int width = 350;
	int height = 350;
	std::string output_file_name = "output.png";
	int margin = 8;
};

std::map<std::string, BarCodeType> barcode_type_map
{
		{"qr", BarCodeType::Qr},
		{"dm", BarCodeType::Dm},
		{"pdf417", BarCodeType::Pdf417},
		{"code39", BarCodeType::Code39},
		{"code93", BarCodeType::Code93},
		{"code128", BarCodeType::Code128},
		{"gs1", BarCodeType::GS1}
};

int main(int argc, char* argv[]) {
	CLI::App app("Hyland Document Filters: CreateBarcode");
	try {

		options_t options;

		app.add_option("-l,--license", options.license_key, "license key for Document Filters");
		app.add_option("text", options.text, "Barcode Text")->required();
		app.add_option("-t,--type", options.type, "Type of barcode")->transform(CLI::CheckedTransformer(barcode_type_map, CLI::ignore_case));
		app.add_option("-c,--caption", options.caption, "Caption to include on barcode");
		app.add_option("--width", options.width, "Width of barcode");
		app.add_option("--height", options.height, "Height of barcode");
		app.add_option("-o,--output", options.output_file_name, "Output filename");
		app.add_option("-m,--margin", options.margin, "Margin around barcode");

		CLI11_PARSE(app, argc, argv);

		DF::Api api(DocumentFiltersSamples::get_license_key(options.license_key), ".");

		auto canvas = api.MakeOutputCanvas(options.output_file_name, DocumentFiltersSamples::extension_to_canvas(std::filesystem::path(options.output_file_name).extension().string()));
		canvas.BlankPage(options.width, options.height);

		DF::AnnotationBarcode barcode;
		barcode.subType = options.type == BarCodeType::Qr ? L"qr" :
			options.type == BarCodeType::Dm ? L"dm" :
			options.type == BarCodeType::Pdf417 ? L"pdf417" :
			options.type == BarCodeType::Code39 ? L"code39" :
			options.type == BarCodeType::Code93 ? L"code93" :
			options.type == BarCodeType::Code128 ? L"code128" : L"gs1";
		barcode.caption = DF::u8_to_w(options.caption);
		barcode.text = DF::u8_to_w(options.text);
		barcode.rect = DF::Rect::ltrb(options.margin, options.margin, options.width - options.margin, options.height - options.margin);

		canvas.Annotate(barcode);
		canvas.Close();

		std::cout << "Barcode created: " << options.output_file_name << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
