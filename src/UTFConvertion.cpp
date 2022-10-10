#ifdef WIN32
#include "UTFConvertion.h"

// Piece of code found on riptutorial.com (https://riptutorial.com/cplusplus/example/4190/conversion-to-std--wstring)

#include <codecvt>

std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> strconverter;

namespace SAL
{
std::wstring UTFConvertion::toWString(const std::string& str)
{
	return strconverter.from_bytes(str);
}

std::string UTFConvertion::toString(const std::wstring& wstr)
{
	return strconverter.to_bytes(wstr);
}
}
#endif // WIN32