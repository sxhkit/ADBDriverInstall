#include "InstallDriverMan.h"
#include <SetupAPI.h>
#include <Cfgmgr32.h>
#include <Newdev.h>
#include <map>
#include <set>
#include "DeviceMan.h"

InstallDriverMan::InstallDriverMan():_hdevInfo(NULL), _isInsert(false)
{
    init();
}

InstallDriverMan::InstallDriverMan(std::wstring dbcc_name):
_dbcc_name(dbcc_name),
_hdevInfo(NULL),
_isInsert(true)
{
    _pid = DeviceMan::getInstance()->getPid(_dbcc_name);
    _vid = DeviceMan::getInstance()->getVid(_dbcc_name);
    _unique = DeviceMan::getInstance()->getUnique(_dbcc_name);
    init();
}

InstallDriverMan::~InstallDriverMan()
{
}

void InstallDriverMan::init()
{
    ScanForHardwareChange();
    collectDeviceInfo();
}

void InstallDriverMan::tryInstallDriver()
{
    if (!isEmptyInstall())
    {
        installDriver();
        ScanForHardwareChange();
    }
}

bool InstallDriverMan::getDevDetailInfo(HDEVINFO devinfo, PSP_DEVINFO_DATA data, DWORD type, std::wstring &info)
{
    DWORD dataType = 0;
    DWORD buffsize = 0;
    bool ret = false;
    if (SetupDiGetDeviceRegistryProperty(devinfo, data, type, &dataType, NULL, NULL, &buffsize)
        || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        DWORD size = buffsize + 1;
        DWORD dataSize = 0;
        BOOL bSuccess = FALSE;
        wchar_t * buf = new wchar_t[size];
        wmemset(buf, 0, size);

        bSuccess = SetupDiGetDeviceRegistryProperty(devinfo, data, type, &dataType, (PBYTE)buf, size, &dataSize);
        if (bSuccess) {
            info = buf;
            ret = true;
        }

        delete[] buf;
        buf = NULL;
    }
    return ret;
}

bool InstallDriverMan::getDevInstanceId(HDEVINFO hdev, PSP_DEVINFO_DATA data, std::wstring &instanceId)
{
    DWORD size = 0;
    bool bSuccess = false;
    if (SetupDiGetDeviceInstanceId(hdev, data, NULL, NULL, &size) ||
        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        DWORD len = size + 4;
        DWORD requiredSize = 0;
        wchar_t *pInstanceId = new wchar_t[len];
        wmemset(pInstanceId, 0, len);
        if (SetupDiGetDeviceInstanceId(hdev, data, pInstanceId, len, &requiredSize))
        {
            instanceId = pInstanceId;
            bSuccess = true;
        }
        delete[] pInstanceId;
        pInstanceId = NULL;
    }
    return bSuccess;
}

bool InstallDriverMan::collectDeviceInfo()
{
    _hdevInfo = SetupDiGetClassDevs(NULL, L"USB", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    SP_DEVINFO_DATA devIno_data;
    devIno_data.cbSize = sizeof(SP_DEVINFO_DATA);
    for (int i = 0; SetupDiEnumDeviceInfo(_hdevInfo, i, &devIno_data); i++)
    {
        DriverInfo info;
        if (!getDevDetailInfo(_hdevInfo, &devIno_data, SPDRP_HARDWAREID, info.hardwareId))
        {
            continue;
        } 

        //如果是插入消息触发的那么要进行PID 和VID 比较
        if (_isInsert)
        {
            std::wstring vid = DeviceMan::getInstance()->getVid(info.hardwareId);
            std::wstring pid = DeviceMan::getInstance()->getPid(info.hardwareId);
            if (_wcsicmp(vid.c_str(), _vid.c_str()) != 0 ||
                _wcsicmp(pid.c_str(), _pid.c_str()) != 0)
            {
                continue;;
            }
		}else {
			std::wstring vid = DeviceMan::getInstance()->getVid(info.hardwareId);
			if (vid.empty() || !DeviceMan::getInstance()->isMoblie(vid))
			{
				continue;
			}
		}

        if (!getDevDetailInfo(_hdevInfo, &devIno_data, SPDRP_COMPATIBLEIDS, info.compatibleId))
        {
            continue;
        }

        if (!DeviceMan::getInstance()->isCompatibleIdConformance(info.compatibleId))
        {
            continue;
		}

        if (!getDevInstanceId(_hdevInfo, &devIno_data, info.instanceId))
        {
            continue;
        }
		//判断驱动是否安装成功
		bool isInstalled = false;
		std::wstring driver;
		if (!getDevDetailInfo(_hdevInfo, &devIno_data, SPDRP_DRIVER, driver))
		{
			DWORD error = GetLastError();
			if (error == ERROR_INVALID_DATA)
			{
				isInstalled = false;
			}
		} else {
			if (isInstalledDriver(info.instanceId))
			{
				isInstalled = true;
			}
		}
		info.bInstalled = isInstalled;
        _list.push_back(info);
    }
    return false;
}

bool InstallDriverMan::isInstalledDriver(std::wstring &instanceId)
{
    DEVINST devInst = NULL;
    bool isInstalled = false;
    if (CM_Locate_DevNode(&devInst, const_cast<LPTSTR>(instanceId.c_str()), CM_LOCATE_DEVNODE_NORMAL) == CR_SUCCESS)
    {

        DWORD status = 0;
        DWORD problemNumber;

        if (CM_Get_DevNode_Status(&status, &problemNumber, devInst, 0) == CR_SUCCESS)
        {
            if (!(status&DN_HAS_PROBLEM))	//判断设备是否存在问题,代表驱动已安装
            {
                //设备无异常,就是说驱动正常 
                isInstalled = true;
            }
        }
    }
    return isInstalled;
}

bool InstallDriverMan::installDriver()
{
    std::wstring path;
    if (!DeviceMan::getInstance()->isWin64()) {
            path = DeviceMan::getInstance()->getCurrentPath() +
            L"\\drivers\\android_driver_32\\android_winusb.inf";
    } else {
        path = DeviceMan::getInstance()->getCurrentPath() +
            L"\\drivers\\android_driver_64\\android_winusb.inf";
    }
    startInstallWDMDriver(path);
    return true;
}

bool InstallDriverMan::startInstallWDMDriver(std::wstring inf_file)
{
    HDEVINFO             hDevInfo = NULL;
    GUID                 guid = { 0L };
    SP_DEVINSTALL_PARAMS spDevInst = { 0L };

    TCHAR  szClass[MAX_CLASS_NAME_LEN] = { 0L };

    if (!SetupDiGetINFClass(inf_file.c_str(), &guid, szClass, MAX_CLASS_NAME_LEN, 0))
    {
        return false;
    }
    hDevInfo = SetupDiGetClassDevs(&guid, 0L, 0L, DIGCF_PRESENT | DIGCF_ALLCLASSES | DIGCF_PROFILE);
    if (!hDevInfo)
    {
        return false;
    }
    spDevInst.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (!SetupDiGetDeviceInstallParams(hDevInfo, 0L, &spDevInst))
    {
        return false;
    }

    spDevInst.Flags = DI_ENUMSINGLEINF;
    spDevInst.FlagsEx = DI_FLAGSEX_ALLOWEXCLUDEDDRVS;
    wcscpy(spDevInst.DriverPath, inf_file.c_str());
    if (!SetupDiSetDeviceInstallParams(hDevInfo, 0, &spDevInst))
    {
        return false;
    }

    if (!SetupDiBuildDriverInfoList(hDevInfo, 0, SPDIT_CLASSDRIVER))
    {
        return false;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return startInstallClassDriver(inf_file);
}

bool InstallDriverMan::startInstallClassDriver(std::wstring inf_file)
{
    GUID            guid = { 0 };
    SP_DEVINFO_DATA spDevData = { 0 };
    HDEVINFO        hDevInfo = 0L;
    TCHAR            className[MAX_CLASS_NAME_LEN] = { 0 };
    BOOL            bRebootRequired;

    if (!SetupDiGetINFClass(inf_file.c_str(), &guid,
        className, MAX_CLASS_NAME_LEN, 0)) {
        return false;
    }

    hDevInfo = SetupDiCreateDeviceInfoList(&guid, 0);
    if (hDevInfo == (void*)-1) {
        return false;
    }
    spDevData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(hDevInfo, className, 
        &guid, 0L, 0L, DICD_GENERATE_ID, &spDevData)) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return false;
    }

    std::list<DriverInfo>::iterator it = _list.begin();
    for (; it != _list.end(); it++) {
        if ((*it).bInstalled)
            continue;
        std::wstring hardwareid;
        hardwareid = (*it).hardwareId;

        if (!SetupDiSetDeviceRegistryProperty(hDevInfo, &spDevData, SPDRP_HARDWAREID,
            (PBYTE)hardwareid.c_str(), (DWORD)hardwareid.length() * 2)) {
            continue;
        }

        bRebootRequired = 0;
        //依据解析出的ID和.inf文件安装驱动
        if (!UpdateDriverForPlugAndPlayDevices(NULL, hardwareid.c_str(), 
			inf_file.c_str(), INSTALLFLAG_FORCE , &bRebootRequired)) {
            DWORD dwErrorCode = GetLastError();
            //安装失败移除设备
            if (!SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, &spDevData))
                SetupDiDestroyDeviceInfoList(hDevInfo);
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return true;
}

bool InstallDriverMan::isEmptyInstall()
{
    bool bEmpty = true;
    std::list<DriverInfo>::iterator it = _list.begin();
    for (it;it !=_list.end(); it++)
    {
        if (!(*it).bInstalled)
        {
            bEmpty = false;
            break;
        }
    }
    return bEmpty;
}

bool InstallDriverMan::ScanForHardwareChange()
{
    DEVINST     devInst;
    CONFIGRET   status;

    status = CM_Locate_DevNode(&devInst, NULL, CM_LOCATE_DEVNODE_NORMAL);
    if (status != CR_SUCCESS)
    {
        return   FALSE;
    }
    status = CM_Reenumerate_DevNode(devInst, CM_REENUMERATE_NORMAL);
    if (status != CR_SUCCESS)
    {
        return   FALSE;
    }
    return   TRUE;
}