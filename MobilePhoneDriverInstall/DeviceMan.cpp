#include "DeviceMan.h"
#include <fstream>
#include <string>
#if (_MSC_VER >= 1800)
#include <regex>
#endif
#include <wtypes.h>

DeviceMan::DeviceMan()
{
	getExePath();
    getEffectivetComId();
    initVidSet();
}

DeviceMan ::~DeviceMan()
{

}
void DeviceMan::getExePath()
{
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileName(GetModuleHandle(NULL), path, MAX_PATH);
    wchar_t *pos = wcsrchr(path, L'\\');
    if (pos)
    {
        *pos = L'\0';
    }
    _exe_path = path;
}

void DeviceMan::initVidSet()
{
    _vid_set.clear();
    std::wstring file = _exe_path + L"\\drivers\\adb_usb.ini";
    std::wfstream fin(file.c_str());
    if (!fin) {
        return;
    }
    std::wstring s;
    while (!fin.eof())
    {
        getline(fin, s);
        _vid_set.insert(s);
    }
    fin.close();
}

std::wstring DeviceMan::getResult(std::wstring data)
{
    std::wstring res;
    if (data.empty()) {
        return res;
    }
#if (_MSC_VER >= 1800)
	std::wstring s(data);
	std::wsmatch m;
	std::wregex e(L"USB\\\\(.+)");

	if (std::regex_search(s, m, e)) {
		res = m[0];
	}
#else
	int pos = data.find(L"USB\\");
	res = data.substr(pos);
#endif
    return res;
}

bool DeviceMan::isWin64()
{
    typedef VOID(WINAPI *LPFN_GetNativeSystemInfo)(__out LPSYSTEM_INFO lpSystemInfo);
    LPFN_GetNativeSystemInfo fnGetNativeSystemInfo = (LPFN_GetNativeSystemInfo)
        GetProcAddress(GetModuleHandleW(L"kernel32"), "GetNativeSystemInfo");
    if (fnGetNativeSystemInfo)
    {
        SYSTEM_INFO stInfo = { 0 };
        fnGetNativeSystemInfo(&stInfo);
        if (stInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64
            || stInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
            return true;
        }
    }
    return false;
}

void DeviceMan::getEffectivetComId()
{
    std::set<std::wstring> set_x86;
    std::set<std::wstring> set_x64;
    std::wstring section_x86 = L"Google.NTx86";
    std::wstring section_x64 = L"Google.NTamd64";
    std::wstring singleInterface = L"%SingleAdbInterface%";
    std::wstring mutilInterface = L"%CompositeAdbInterface%";
    std::wstring path = _exe_path + L"\\drivers\\android_driver_32\\android_winusb.inf";

    wchar_t szTemp[MAX_PATH] = { 0 };
    GetPrivateProfileString(section_x86.c_str(), singleInterface.c_str(), NULL, szTemp, MAX_PATH, path.c_str());
    set_x86.insert(getResult(szTemp));
    GetPrivateProfileString(section_x86.c_str(), mutilInterface.c_str(), NULL, szTemp, MAX_PATH, path.c_str());
    path = _exe_path + L"\\drivers\\android_driver_64\\android_winusb.inf";
    set_x86.insert(getResult(szTemp));

    
    GetPrivateProfileString(section_x64.c_str(), singleInterface.c_str(), NULL, szTemp, MAX_PATH, path.c_str());
    set_x64.insert(getResult(szTemp));
    GetPrivateProfileString(section_x64.c_str(), mutilInterface.c_str(), NULL, szTemp, MAX_PATH, path.c_str());
    set_x64.insert(getResult(szTemp));

    _validComId.insert(std::make_pair(L"X86", set_x86));
    _validComId.insert(std::make_pair(L"X64", set_x64));
}

bool DeviceMan::isCompatibleIdConformance(std::wstring data) {
    std::set<std::wstring> set;
    if (DeviceMan::getInstance()->isWin64()) {
        set = _validComId[L"X64"];
    }
    else {
        set = _validComId[L"X86"];
    }
    std::set<std::wstring>::iterator it = set.begin();
    bool bOK = false;
    for (it; it != set.end(); it++)
    {
        if (_wcsicmp((*it).c_str(), data.c_str()) == 0)
        {
            bOK = true;
            break;
        }
    }
    return bOK;
}

bool DeviceMan::isMoblie(std::wstring vid)
{
    bool isMobile = false;
    std::set<std::wstring>::iterator it = _vid_set.find(vid);
    if (it!= _vid_set.end())
    {
        isMobile = true;
    }
    return isMobile;
}

std::wstring DeviceMan::getPid(std::wstring data)
{
    std::wstring pid;
    if (data.empty()) {
        return pid;
    }
#if (_MSC_VER >= 1800)
	std::wstring s(data);
	std::wsmatch m;
    std::wregex e(L"[Pp][Ii][Dd][_]{0,1}([a-fA-F\\d]{4})");

	if (std::regex_search(s, m, e)) {
		pid = m[1];
	}
#else
	int pos = data.find(L"Pid_");
	if (pos >= 0 )
	{
		pid = data.substr(pos + 4,4);
	}
#endif
    return pid;
}

std::wstring DeviceMan::getVid(std::wstring data)
{
    std::wstring vid;
    if (data.empty()) {
        return vid;
    }


#if (_MSC_VER >= 1800)
	std::wstring s(data);
	std::wsmatch m;
    std::wregex e(L"[Vv][Ii][Dd][_]{0,1}([a-fA-F\\d]{4})");

    if (std::regex_search(s, m, e)) {
        vid = L"0x";
        vid += m[1];
	}
#else
	int pos = data.find(L"Vid_");
	if (pos >= 0)
	{
		vid = std::wstring(L"0x") + data.substr(pos + 4,4);
	}
#endif
    return vid;
}

std::wstring DeviceMan::getUnique(std::wstring data)
{
    std::wstring unique;
    if (data.empty()) {
        return unique;
    }
#if (_MSC_VER >= 1800)
	std::wstring s(data);
	std::wsmatch m;
	std::wregex e(L"#([\\w]+)#");

	if (std::regex_search(s, m, e)) {
		unique = m[1];
	}
#else
	int end = data.rfind(L"#");
	int start = data.substr(0,end).rfind(L"#");

	unique = data.substr(start + 1,end - start - 2);
#endif 
    return unique;
}
