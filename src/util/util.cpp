#include "util/util.hpp"

#include <memory>

std::unique_ptr<UtilContext> util;

MainContext::MainContext()
    : VERBOSE(false) {}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Util
