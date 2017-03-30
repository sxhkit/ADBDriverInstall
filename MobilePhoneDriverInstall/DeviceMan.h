#pragma once
#include <set>
#include "string.h"
#include <map>
class DeviceMan
{
    DeviceMan();
public:
    ~DeviceMan();
    static DeviceMan * getInstance()
    {
        static DeviceMan deviceMan;
        return &deviceMan;
    }

    std::wstring getResult(std::wstring data);
    bool isWin64();
    bool isCompatibleIdConformance(std::wstring data);
    bool isMoblie(std::wstring vid);
    std::wstring getPid(std::wstring data);
    std::wstring getVid(std::wstring data);
    std::wstring getUnique(std::wstring data);
	std::wstring getCurrentPath()
	{
		if (_exe_path.empty())
		{
			getExePath();
		}
		return _exe_path;
	}
private:
    void initVidSet();
    void getEffectivetComId();
	void getExePath();

private:
    std::map<std::wstring, std::set<std::wstring>> _validComId;
    std::set<std::wstring> _vid_set;
	std::wstring _exe_path;
};
