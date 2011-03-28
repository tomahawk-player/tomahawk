# taglib changed filenames to be a char/wchar struct on some platforms, need to check for it
macro (CHECK_TAGLIB_FILENAME TAGLIB_FILENAME_COMPLEX)
	include (CheckCXXSourceCompiles)
	set (CMAKE_REQUIRED_FLAGS ${TAGLIB_CFLAGS})
	set (CMAKE_REQUIRED_INCLUDES ${TAGLIB_INCLUDES})
	set (CMAKE_REQUIRED_LIBRARIES ${TAGLIB_LIBRARIES})
	check_cxx_source_compiles(
	"#include <tfile.h>
	int main()
	{
		TagLib::FileName fileName1(\"char\");
		TagLib::FileName fileName2(L\"wchar\");
		return 0;
	}" ${TAGLIB_FILENAME_COMPLEX})
endmacro (CHECK_TAGLIB_FILENAME)
