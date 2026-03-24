#ifndef WTNS_UTILS
#define WTNS_UTILS

#include <gmp.h>
#include <cstdint>

#include "binfile_utils.hpp"

namespace WtnsUtils {

    class Header {
    public:
        uint32_t n8;
        mpz_t prime;

        uint32_t nVars;

        Header();
        ~Header();
    };

    std::unique_ptr<Header> loadHeader(BinFileUtils::BinFile *f);

}

#endif // ZKEY_UTILS_H