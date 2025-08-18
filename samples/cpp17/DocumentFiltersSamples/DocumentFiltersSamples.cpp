#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_LEAN_AND_MEAN
#include "DocumentFiltersSamples.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>

#ifdef  _WIN32 
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif // ! _WIN32


namespace DocumentFiltersSamples
{
    namespace
    {
#ifndef _WIN32
        void setStdinEcho(bool enable) {
            struct termios tty;
            tcgetattr(STDIN_FILENO, &tty);
            if (!enable)
                tty.c_lflag &= ~ECHO;
            else
                tty.c_lflag |= ECHO;

            (void)tcsetattr(STDIN_FILENO, TCSANOW, &tty);
        }
#endif

        std::wstring prompt_for_password()
        {
            std::string password;

#ifndef _WIN32
			setStdinEcho(false);
			std::getline(std::cin, password);
            setStdinEcho(false);
#else
            char ch;
            while ((ch = _getch()) != '\r') {  // _getch() reads a single character without echoing
                if (ch == '\b') {  // Handle backspace
                    if (!password.empty()) 
                        password.pop_back();
                }
                else {
                    password.push_back(ch);
                }
            }
#endif
            std::cout << std::endl;

			return Hyland::DocFilters::u8_to_w(password);
        }
    }

    std::string get_license_key(const std::string& user_provided_key)
    {
        if (!user_provided_key.empty())
            return user_provided_key;

        const char* s = std::getenv("DF_LICENSE_KEY");
        if (!s)
        {
#ifdef DOCUMENT_FILTERS_LICENSE_KEY
            return DOCUMENT_FILTERS_LICENSE_KEY;
#else
            return "";
#endif
        }
        return s;
    }

    Hyland::DocFilters::Extractor& handle_password_prompt(Hyland::DocFilters::Extractor& doc)
    {
		doc.setPasswordCallback([](const std::wstring& file)->std::wstring {
			if (file.empty())
                std::cerr << "Password: ";
            else
                std::cerr << "Password required for " << Hyland::DocFilters::w_to_u8(file) << ": ";
			return prompt_for_password();
        });
        return doc;
    }


    Hyland::DocFilters::CanvasType extension_to_canvas(std::string ext)
    {
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".pdf") return Hyland::DocFilters::CanvasType::PDF;
        if (ext == ".ps") return Hyland::DocFilters::CanvasType::PS;
        if (ext == ".tif" || ext == ".tiff") return Hyland::DocFilters::CanvasType::TIF;
        if (ext == ".xml") return Hyland::DocFilters::CanvasType::XML;
        if (ext == ".html" || ext == ".htm") return Hyland::DocFilters::CanvasType::HTML;
        if (ext == ".bmp") return Hyland::DocFilters::CanvasType::BMP;
        if (ext == ".brk") return Hyland::DocFilters::CanvasType::BRK;
        if (ext == ".dcx") return Hyland::DocFilters::CanvasType::DCX;
        if (ext == ".eps") return Hyland::DocFilters::CanvasType::EPS;
        if (ext == ".gif") return Hyland::DocFilters::CanvasType::GIF;
        if (ext == ".html") return Hyland::DocFilters::CanvasType::HTML;
        if (ext == ".jpk") return Hyland::DocFilters::CanvasType::JPEG2000;
        if (ext == ".jpg" || ext == ".jpeg") return Hyland::DocFilters::CanvasType::JPG;
        if (ext == ".json") return Hyland::DocFilters::CanvasType::JSON;
        if (ext == ".markdown" || ext == ".md") return Hyland::DocFilters::CanvasType::MARKDOWN;
        if (ext == ".pbm") return Hyland::DocFilters::CanvasType::PBM;
        if (ext == ".pcx") return Hyland::DocFilters::CanvasType::PCX;
        if (ext == ".pdf") return Hyland::DocFilters::CanvasType::PDF;
        if (ext == ".pgm") return Hyland::DocFilters::CanvasType::PGM;
        if (ext == ".png") return Hyland::DocFilters::CanvasType::PNG;
        if (ext == ".ppm") return Hyland::DocFilters::CanvasType::PPM;
        if (ext == ".ps") return Hyland::DocFilters::CanvasType::PS;
        if (ext == ".svg") return Hyland::DocFilters::CanvasType::SVG;
        if (ext == ".tga") return Hyland::DocFilters::CanvasType::TGA;
        if (ext == ".tif") return Hyland::DocFilters::CanvasType::TIF;
        if (ext == ".webp") return Hyland::DocFilters::CanvasType::WEBP;
        if (ext == ".websafe") return Hyland::DocFilters::CanvasType::WEBSAFE;
        if (ext == ".xml") return Hyland::DocFilters::CanvasType::XML;
        if (ext == ".xps") return Hyland::DocFilters::CanvasType::XPS;

        throw std::invalid_argument("Unknown format " + ext);
    }
}