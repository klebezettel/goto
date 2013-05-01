#include "debugutils.h"

namespace Utils {
namespace DebugUtils {

DebugOutput debug()
{
#if DEBUG_OUTPUT
    static std::ofstream stream(DEBUG_OUTPUT_FILEPATH);
    return DebugOutput(&stream);
#else
    return DebugOutput();
#endif
}

} // namespace DebugUtils
} // namespace Utils
