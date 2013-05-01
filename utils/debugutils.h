#ifndef ASSERTDEBUG_H
#define ASSERTDEBUG_H

#include <fstream>

#define DEBUG_OUTPUT 1
#define DEBUG_OUTPUT_FILEPATH "/tmp/goto_debug.log"

namespace Utils {
namespace DebugUtils {

/// Ncurses apps cannot just print to stdout/stderr, so print to a file
class DebugOutput
{
#if DEBUG_OUTPUT
    std::ofstream *stream;
#endif
public:
#if DEBUG_OUTPUT
    explicit DebugOutput(std::ofstream *stream = 0) : stream(stream) {}
    ~DebugOutput() { *stream << std::endl << std::flush; }
#else
    explicit DebugOutput(std::ofstream * = 0) {}
#endif

#if DEBUG_OUTPUT
#define OUT_OPERATOR(type) \
    DebugOutput &operator<<(const type value) \
    { \
        *stream << value << ' '; \
        return *this; \
    }
#else
#define OUT_OPERATOR(type) \
    DebugOutput &operator<<(const type) { return *this; }
#endif

    OUT_OPERATOR(std::string&)
    OUT_OPERATOR(std::wostream&) // for endl
    OUT_OPERATOR(char*)
    OUT_OPERATOR(bool)
    OUT_OPERATOR(int)
};


/// debug(): Return DebugOutput object for printing. Usage: debug() << "hello";
DebugOutput debug();

} // namespace DebugUtils
} // namespace Utils

/// assert(condition): Ensure condition is true, otherwise print the failed condition with debug().
#define assert(condition) assert_helper(condition, __FILE__, __LINE__)
#define assert_helper(condition, file, line) \
    if (!(condition)) { \
        Utils::DebugUtils::debug() << file << line << "Assertion failed: " << #condition; \
    }

#endif // ASSERTDEBUG_H
