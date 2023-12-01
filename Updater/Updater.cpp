#include "openssl/sha.h"
#include "tinyxml2.h"

#include "Updater.h"
#include "utils.h"
#include "installer.h"
#include "framework.h"
#include "resource.h"

using namespace std::filesystem;

#define MAX_LOADSTRING 100
#define VERSION "1.2.0"

HINSTANCE hInst;
HWND hWnd;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HBITMAP hLogoImage;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void InstallOrProceed();
void SetWindowData(std::wstring text, int progress);
std::string GetFileHash(std::wstring path);

int currentProgress = 0;
std::wstring currentText;
bool isWindowClosing = false;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (argc >= 3)
    {
        if (!_wcsicmp(argv[1], L"-uninstall"))
        {
            if (Uninstall(argv[2]))
            {
                MessageBox(NULL, PRODUCT_NAME L" successfuly uninstalled.", L"Umbrella.re", MB_OK);
            }
            LocalFree(argv);
            return 0;
        }
    }

    LocalFree(argv);

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDS_TITLE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDS_TITLE));

    MSG msg;

    int tickCount = 0;
    bool isThreadStarted = false;
    std::thread t(InstallOrProceed);

    while (!isWindowClosing && GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DestroyWindow(hWnd);

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(3 + 1);
    //wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_UMBRELLALAUNCHER);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));

    return RegisterClassExW(&wcex);
}

void DownloadCacheFile(std::wstring appPath, std::string stringFileName)
{
    std::wstring fileName = std::wstring(stringFileName.begin(), stringFileName.end());
    std::wstring downloadPath = appPath + fileName;
    std::string downloadUrl = CDN_URL DOWNLOAD_FILE + stringFileName;

    std::replace(downloadUrl.begin(), downloadUrl.end(), '\\', '/');

    DownloadFile(downloadPath, downloadUrl, fileName);
}

void CheckFilesToUpdate(const tinyxml2::XMLDocument& doc, std::wstring appPath)
{
    std::vector<std::vector<std::string>> caches = {};
    std::vector<std::string> findedFiles = {};

    tinyxml2::XMLNode* root = (tinyxml2::XMLDocument*)doc.RootElement();
    if (root == nullptr) return;

    tinyxml2::XMLElement* list = root->FirstChildElement("Application");
    if (list == nullptr) return; //Fails here

    tinyxml2::XMLElement* element = list->FirstChildElement("Item");
    while (element)
    {
        std::string path = element->Attribute("path");
        std::string hashsum = element->Attribute("hashsum");
        caches.push_back({ path, hashsum });

        element = element->NextSiblingElement("Item");
    }

    std::vector<std::vector<std::string>> files = {};
    for (const auto& entry : recursive_directory_iterator(appPath)) {
        auto elemPath = entry.path().string();
        auto relativePath = elemPath.substr(appPath.length());
        std::string hash = GetFileHash(entry.path());
        if (hash != "")
        {
            files.push_back({ relativePath, hash });
        }
    }

    for (auto& cache : caches)
    {
        bool fileFinded = false;

        for (auto& file : files)
        {
            if (cache[0] == file[0])
            {
                fileFinded = true;
                if (cache[1] != file[1])
                {
                    DownloadCacheFile(appPath, cache[0]);
                }
            }
        }

        if (!fileFinded)
        {
            if (cache[0].find("\\") != -1)
            {
                std::vector<std::string> folders = Split(cache[0], '\\');
                int i = 0;
                for (auto& folder : folders)
                {
                    if (folders.size() - 1 == i) break;

                    create_directory(WideStringToString(appPath) + folder);
                    i++;
                }
            }
            DownloadCacheFile(appPath, cache[0]);
        }
    }
}

bool IsUpdaterNeedToUpdate(const tinyxml2::XMLDocument& doc)
{
    std::wstring path = ExePath();
    std::wstring name = ExeName();

    tinyxml2::XMLNode* root = (tinyxml2::XMLDocument*)doc.RootElement();
    if (root == nullptr) return false;

    tinyxml2::XMLElement* ulist = root->FirstChildElement("Updater");
    if (ulist == nullptr) return false; //Fails here

    tinyxml2::XMLElement* updater = ulist->FirstChildElement("Item");

    std::string version = updater->Attribute("version");

    if (version != VERSION)
    {
        _wrename(name.c_str(), (path + L"\\" PRODUCT_NAME L"_old.exe").c_str());

        SetWindowData(L"Updating app...", 0);

        DownloadFile(path + L"\\" PRODUCT_NAME L".exe", API_URL GET_UPDATER, PRODUCT_NAME L".exe");

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        CreateProcessW(
            NULL,
            (LPWSTR)(path + L"\\" + PRODUCT_NAME L".exe").c_str(),
            NULL,
            NULL,
            FALSE,
            0, NULL,
            (LPWSTR)path.c_str(),
            &si,
            &pi
        );

        isWindowClosing = true;

        return true;
    }

    return false;
}

void InstallOrProceed() {
    std::wstring rootPath = GetRootPath();

    if (rootPath.empty())
    {
        MessageBox(NULL, L"Can't find or create root folder in AppData.", L"Umbrella.re", MB_OK | MB_ICONSTOP);

        isWindowClosing = true;

        return;
    }

    std::wstring appPath = rootPath + L"\\Application";
    std::wstring exeName = L"\\" PRODUCT_NAME L".exe";

    if (DirOrFileExists(rootPath + L"\\" PRODUCT_NAME L"_old.exe"))
    {
        DeleteFile((rootPath + L"\\" PRODUCT_NAME L"_old.exe").c_str());
    }

    SetWindowData(L"Checking update server...", 0);

    bool updateServerAvaliable = IsUrlValid(API_URL GET_CACHES);

    if (DirOrFileExists(appPath) && DirOrFileExists(appPath + exeName) && DirOrFileExists(rootPath + exeName))
    {
        if (ExePath() != rootPath)
        {
            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            CreateProcessW(
                NULL,
                (LPWSTR)(rootPath + exeName).c_str(),
                NULL,
                NULL,
                FALSE,
                0, NULL,
                (LPWSTR)rootPath.c_str(),
                &si,
                &pi
            );

            isWindowClosing = true;

            return;
        }

        if (updateServerAvaliable) {
            std::string data = GetUrl(API_URL GET_CACHES);

            tinyxml2::XMLDocument doc;
            doc.Parse(data.c_str());

            if (IsUpdaterNeedToUpdate(doc)) {
                return;
            }

            SetWindowData(L"Checking updates...", 0);

            CheckFilesToUpdate(doc, appPath + L"\\");
        }

        SetWindowData(L"Starting app...", 100);

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        CreateProcessW(
            NULL,
            (LPWSTR)(appPath + exeName + L" -startedFromUpdater").c_str(),
            NULL,
            NULL,
            FALSE,
            0, NULL,
            (LPWSTR)appPath.c_str(),
            &si,
            &pi
        );

        isWindowClosing = true;
    }
    else
    {
        if (PerformInstallation(updateServerAvaliable))
        {
            InstallOrProceed();
        }
    }
}

void SetWindowData(std::wstring text, int progress)
{
    currentText = text;
    currentProgress = progress;

    RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

int GetCurrentProgress()
{
    return currentProgress;
}

void CloseWindow()
{
    DestroyWindow(hWnd);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        760, 340, 400, 400, nullptr, nullptr, hInstance, nullptr);

    ::DWORD styles = ::GetWindowLongW(hWnd, GWL_STYLE);
    styles &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    styles = ::SetWindowLongW(hWnd, GWL_STYLE, styles);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            SetClassLong(hWnd, GCLP_HBRBACKGROUND, (LONG)CreateSolidBrush(RGB(28, 29, 33)));
            break;
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                HDC hdc_x = CreateCompatibleDC(NULL);

                RECT rect;
                GetWindowRect(hWnd, &rect);

                HBITMAP hLogo = (HBITMAP)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
                SelectObject(hdc_x, hLogo);

                BitBlt(hdc, 0, 0, rect.right - rect.left, 500, hdc_x, 0, 0, SRCCOPY);

                HBRUSH hBrushAnother = CreateSolidBrush(RGB(24, 24, 24));

                SelectObject(hdc, hBrushAnother);

                RoundRect(hdc, 95, 356, 305, 362, 6, 6);

                DeleteObject(hBrushAnother);

                HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));

                SelectObject(hdc, hBrush);

                RoundRect(hdc, 95, 356, 95 + currentProgress, 362, 6, 6);

                DeleteObject(hBrush);

                HFONT hFont = CreateFont(13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Arial");
                DeleteDC(hdc_x);
                SelectObject(hdc, hFont);

                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkMode(hdc, TRANSPARENT);

                TextOut(hdc, 155, 336, currentText.c_str(), wcslen(currentText.c_str()));

                ReleaseDC(hWnd, hdc);
                EndPaint(hWnd, &ps);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}