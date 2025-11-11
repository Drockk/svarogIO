#pragma once

#include <gsl/assert>
#include <cassert>

namespace svarog::core {
#ifdef SVAROG_ENABLE_CONTRACTS_IN_RELEASE
    #define SVAROG_EXPECTS(cond) Expects(cond)
    #define SVAROG_ENSURES(cond) Ensures(cond)
#else
    #ifdef NDEBUG
        #define SVAROG_EXPECTS(cond)
        #define SVAROG_ENSURES(cond)
    #else
        #define SVAROG_EXPECTS(cond) Expects(cond)
        #define SVAROG_ENSURES(cond) Ensures(cond)
    #endif
#endif

#define SVAROG_ASSERT(cond) assert(cond)
} // namespace svarog::core
