#ifndef SIMPLE_AUDIO_LIBRARY_UTFCONVERTION_H_
#define SIMPLE_AUDIO_LIBRARY_UTFCONVERTION_H_

#ifdef WIN32
#include <string>

namespace SAL
{
/*
Helper function to convert UTF8 string to UTF16 wstring and vice versa on Windows.
*/
class UTFConvertion
{
public:
	/*
	Converting a UTF8 string to a UTF16 wstring
	*/
	static std::wstring toWString(const std::string& str);

	/*
	Converting a UTF16 wstring to a UTF8 string
	*/
	static std::string toString(const std::wstring& wstr);
};
}
#endif // WIN32

#endif // SIMPLE_AUDIO_LIBRARY_UTFCONVERTION_H_