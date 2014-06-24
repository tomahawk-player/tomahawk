# taglib changed filenames to be a char/wchar struct on some platforms, need to check for it
macro (CHECK_STD_FUNCTIONAL CXX_STD_FUNCTIONAL)
	include (CheckCXXSourceCompiles)
	check_cxx_source_compiles(
	"
    #include <functional>
    using std::function;
    int main()
    {
    return 0;
    }" ${CXX_STD_FUNCTIONAL})
endmacro()

