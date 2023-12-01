#include "Updater.h"
#include "utils.h"
#include <string>
#include <shlobj_core.h>
#include <filesystem>
#include <wrl.h>

using namespace Microsoft;

void SetWindowData(std::wstring text, int progress);

static std::wstring GetFolderPath(const KNOWNFOLDERID& folderId)
{
    PWSTR path;
    if (SUCCEEDED(SHGetKnownFolderPath(folderId, 0, nullptr, &path)))
    {
        std::wstring pathStr = path;

        CoTaskMemFree(path);

        return pathStr;
    }


    return L"";
}

std::wstring GetRootPath()
{
    std::wstring appDataPath = GetFolderPath(FOLDERID_LocalAppData);

    if (!appDataPath.empty())
    {
        appDataPath += L"\\" PRODUCT_NAME;

        CreateDirectory(appDataPath.c_str(), nullptr);
    }

    return appDataPath;
}

static void CreateUninstallEntryIfNeeded(std::wstring filename)
{
    auto setUninstallString = [](const std::wstring& name, const std::wstring& value)
    {
        RegSetKeyValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\UmbrellaRE_" PRODUCT_NAME, name.c_str(), REG_SZ, value.c_str(), (value.length() * 2) + 2);
    };

    auto setUninstallDword = [](const std::wstring& name, DWORD value)
    {
        RegSetKeyValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\UmbrellaRE_" PRODUCT_NAME, name.c_str(), REG_DWORD, &value, sizeof(value));
    };

    setUninstallString(L"DisplayName", PRODUCT_NAME);
    setUninstallString(L"DisplayIcon", filename + std::wstring(L",0"));
    setUninstallString(L"HelpLink", L"https://umbrella.re/");
    setUninstallString(L"InstallLocation", GetRootPath());
    setUninstallString(L"Publisher", L"Umbrella.re");
    setUninstallString(L"UninstallString", filename + std::wstring(L" -uninstall app"));
    setUninstallString(L"URLInfoAbout", L"https://umbrella.re/");
    setUninstallDword(L"NoModify", 1);
    setUninstallDword(L"NoRepair", 1);
}

bool PerformInstallation(bool updateServerAvaliable)
{
	auto rootPath = GetRootPath();

	if (rootPath.empty())
	{
		return false;
	}

	std::wstring installPath = rootPath + L"\\Application";

	if (!DirOrFileExists(installPath))
	{
		CreateDirectory(installPath.c_str(), nullptr);
	}

	std::string downloadUrl = API_URL DOWNLOAD;
	if (!updateServerAvaliable)
	{
		downloadUrl = BACKUP_DOWNLOAD_URL;
	}

	SetWindowData(L"Downloading app...", 0);

	DownloadFile(installPath + L"\\ToolKitV.zip", downloadUrl, L"ToolKitV.zip");

	SetWindowData(L"Unpacking app...", 0);

	UnpackArchive(installPath, L"\\ToolKitV.zip");

	DeleteFile((installPath + L"\\ToolKitV.zip").c_str());

	// the executable goes to the target
	auto targetExePath = rootPath + L"\\" PRODUCT_NAME L".exe";

	// actually copy the executable to the target
	wchar_t thisFileName[512];
	GetModuleFileName(GetModuleHandle(NULL), thisFileName, sizeof(thisFileName) / 2);

	if (!CopyFile(thisFileName, targetExePath.c_str(), FALSE))
	{
		return false;
	}

	// create shortcuts
	{
		CoInitialize(NULL);

		WRL::ComPtr<IShellLink> shellLink;
		HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)shellLink.ReleaseAndGetAddressOf());

		if (SUCCEEDED(hr))
		{
			shellLink->SetPath(targetExePath.c_str());
			shellLink->SetDescription(PRODUCT_NAME L" is a utility from Umbrella.re team");
			shellLink->SetIconLocation(targetExePath.c_str(), 0);

			WRL::ComPtr<IPersistFile> persist;
			hr = shellLink.As(&persist);

			if (SUCCEEDED(hr))
			{
				persist->Save((GetFolderPath(FOLDERID_Programs) + L"\\" PRODUCT_NAME L".lnk").c_str(), TRUE);
				persist->Save((GetFolderPath(FOLDERID_Desktop) + L"\\" PRODUCT_NAME L".lnk").c_str(), TRUE);
			}
		}

		CoUninitialize();
	}

	CreateUninstallEntryIfNeeded(targetExePath);

	return true;
}


bool Uninstall(const wchar_t* directory)
{
	CoInitialize(NULL);

	WRL::ComPtr<IFileOperation> ifo;
	HRESULT hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_INPROC_SERVER, IID_IFileOperation, (void**)&ifo);

	if (FAILED(hr))
	{
		MessageBox(NULL, L"Failed to create instance of IFileOperation.", L"Umbrella.re", MB_OK | MB_ICONSTOP);
		return false;
	}

	ifo->SetOperationFlags(FOF_NOCONFIRMATION);

	auto addDelete = [ifo](const std::wstring& item)
	{
		WRL::ComPtr<IShellItem> shitem;
		if (FAILED(SHCreateItemFromParsingName(item.c_str(), NULL, IID_IShellItem, (void**)&shitem)))
		{
			return false;
		}

		ifo->DeleteItem(shitem.Get(), NULL);
		return true;
	};

	addDelete(directory);
	addDelete(GetFolderPath(FOLDERID_Programs) + L"\\" PRODUCT_NAME L".lnk");
	addDelete(GetFolderPath(FOLDERID_Desktop) + L"\\" PRODUCT_NAME L".lnk");
	
	auto rootPath = GetRootPath();

	if (!rootPath.empty())
	{
		addDelete(rootPath + L"\\Application");
	}

	hr = ifo->PerformOperations();

	if (FAILED(hr))
	{
		MessageBox(NULL, L"Failed to uninstall", L"Umbrella.re", MB_OK | MB_ICONSTOP);
		return false;
	}

	BOOL aborted = FALSE;
	ifo->GetAnyOperationsAborted(&aborted);

	if (aborted)
	{
		MessageBox(NULL, L"The uninstall operation was canceled. Some files may still remain. Please remove these files manually.", L"Umbrella.re", MB_OK | MB_ICONSTOP);
	}

	RegDeleteKey(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\UmbrellaRE_" PRODUCT_NAME);

	return true;
}