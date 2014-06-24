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

