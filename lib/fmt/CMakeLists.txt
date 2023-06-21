

# This is somewhat of a hack, maybe actually include this via findPackage (?)
add_library( fmt STATIC 
	"include/fmt/chrono.h"
	"include/fmt/color.h"
	"include/fmt/core.h"
	"include/fmt/format-inl.h"
	"include/fmt/format.h"
	"include/fmt/locale.h" 
	"include/fmt/ostream.h"
	"include/fmt/posix.h"
	"include/fmt/printf.h"
	"include/fmt/ranges.h"
	"include/fmt/os.h"

	"src/format.cc"
	"src/os.cc"
)

target_include_directories( fmt PUBLIC include )
