// Single translation unit that compiles the tinyexr implementation, backed by the
// project's vendored zlib (TINYEXR_USE_MINIZ 0). zlib.h comes from
// Libraries/source/zlib-1.3.1 (added to the include path in the vcxproj).
#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 0
#include <zlib.h>
#include <tinyexr.h>
