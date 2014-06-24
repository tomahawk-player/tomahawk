macro (CHECK_STD_TR1_FUNCTIONAL CXX_STD_TR1_FUNCTIONAL)
	include (CheckCXXSourceCompiles)
	check_cxx_source_compiles(
	"
    #include <tr1/functional>
    using std::tr1::function;
    int main()
    {
    return 0;
    }" ${CXX_STD_TR1_FUNCTIONAL})
endmacro()

