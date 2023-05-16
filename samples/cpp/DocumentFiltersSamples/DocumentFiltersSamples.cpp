#include "DocumentFiltersSamples.h"
#include <cstdlib>
#include <stdexcept>

namespace DocumentFiltersSamples
{
    std::string get_license_key()
    {
        const char* s = std::getenv("DF_LICENSE_CODE");
        if (!s)
            throw std::runtime_error("DF_LICENSE_CODE environment variable must be set");
        return s;
    }
}