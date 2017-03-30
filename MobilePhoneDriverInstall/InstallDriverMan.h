#pragma once
#include <string>
#include "windows.h"
#include <SetupAPI.h>
#include <list>
typedef struct DriverInfo
{
    bool bInstalled;
    std::wstring compatibleId;
    std::wstring hardwareId;
    std::wstring instanceId;
    DriverInfo()
    {
        bInstalled = false;
    }
}DriverInfo,*PDriverInfo;

class InstallDriverMan
{
public:
    InstallDriverMan();
    InstallDriverMan(std::wstring dbcc_name);
    ~InstallDriverMan();
    
    void tryInstallDriver();
private:
    std::wstring _dbcc_name;
    std::wstring _pid;
    std::wstring _vid;
    std::wstring _unique;
    std::list<DriverInfo> _list;
    HDEVINFO _hdevInfo;
    bool _isInsert;
    
private:
    bool collectDeviceInfo();
    bool getDevDetailInfo(HDEVINFO devinfo, PSP_DEVINFO_DATA data, DWORD type,std::wstring &info);
    bool getDevInstanceId(HDEVINFO hdev, PSP_DEVINFO_DATA data, std::wstring &instanceId);
    bool isInstalledDriver(std::wstring &instanceId);
    bool installDriver();
    bool isEmptyInstall();

    bool startInstallWDMDriver(std::wstring inf_file);
    bool startInstallClassDriver(std::wstring inf_file);
    void init();
    bool ScanForHardwareChange();
};

