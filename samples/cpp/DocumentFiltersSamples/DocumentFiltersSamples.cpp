#include "DocumentFiltersSamples.h"
#include "../DocumentFiltersLicense.h"
#include <cstdlib>
#include <stdexcept>

namespace DocumentFiltersSamples
{
    std::string get_license_key()
    {
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
}