#include "utils.h"

#define CURL_STATICLIB

#include "curl/curl.h"
#include "openssl/sha.h"
#include "zip.h"

#include <chrono>
using namespace std::chrono;

using namespace std::filesystem;

std::wstring currentFileName;

void SetWindowData(std::wstring text, int progress);
int GetCurrentProgress();

steady_clock::time_point startTime = high_resolution_clock::now();

std::wstring ExeName()
{
    wchar_t buf[MAX_PATH];
    GetModuleFileName(nullptr, buf, MAX_PATH);
    return buf;
}

std::wstring ExePath() {
    TCHAR buffer[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    return std::wstring(buffer).substr(0, pos);
}

bool DirOrFileExists(const std::wstring& dirName_in)
{
    DWORD ftyp = GetFileAttributesW(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE)
        return true;   // this is a directory!

    return false;    // this is not a directory!
}

std::string WideStringToString(const std::wstring& wide_string)
{
    if (wide_string.empty())
    {
        return "";
    }

    const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
    {
        throw std::runtime_error("WideCharToMultiByte() failed: " + std::to_string(size_needed));
    }

    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), &result.at(0), size_needed, nullptr, nullptr);
    return result;
}

std::vector<std::string> Split(const std::string& s, char delim)
{
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
        // elems.push_back(std::move(item)); // if C++11 (based on comment from @mchiasson)
    }
    return elems;
}

std::string GetHexRepresentation(const unsigned char* Bytes, size_t Length) {
    std::string ret;
    ret.reserve(Length * 2);
    for (const unsigned char* ptr = Bytes; ptr < Bytes + Length; ++ptr) {
        char buf[3];
        sprintf_s(buf, "%02x", (*ptr) & 0xff);
        ret += buf;
    }
    return ret;
}

std::string GetFileHash(std::wstring path)
{
    HANDLE hFile = NULL;

    hFile = CreateFile(path.c_str(),
        GENERIC_READ | SYNCHRONIZE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        return "";
    }

    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);

    OVERLAPPED overlapped;

    SHA_CTX ctx;
    SHA1_Init(&ctx);

    bool doneReading = false;
    uint64_t fileOffset = 0;

    double lastProgress = 0.0;

    while (!doneReading)
    {
        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.OffsetHigh = fileOffset >> 32;
        overlapped.Offset = fileOffset;

        char buffer[131072];
        if (ReadFile(hFile, buffer, sizeof(buffer), NULL, &overlapped) == FALSE)
        {
            if (GetLastError() != ERROR_IO_PENDING && GetLastError() != ERROR_HANDLE_EOF)
            {
                return "";
            }

            if (GetLastError() == ERROR_HANDLE_EOF)
            {
                break;
            }

            while (true)
            {
                HANDLE pHandles[1];
                pHandles[0] = hFile;

                DWORD waitResult = MsgWaitForMultipleObjects(1, pHandles, FALSE, INFINITE, QS_ALLINPUT);

                if (waitResult == WAIT_OBJECT_0)
                {
                    break;
                }

                MSG msg;
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        DWORD bytesRead;
        BOOL olResult = GetOverlappedResult(hFile, &overlapped, &bytesRead, FALSE);
        DWORD err = GetLastError();

        SHA1_Update(&ctx, (uint8_t*)buffer, bytesRead);

        if (bytesRead < sizeof(buffer) || (!olResult && err == ERROR_HANDLE_EOF))
        {
            doneReading = true;
        }

        fileOffset += bytesRead;
    }

    CloseHandle(hFile);

    uint8_t outHash[20];
    SHA1_Final(outHash, &ctx);

    std::string hash = GetHexRepresentation(outHash, 20);
    std::cout << hash << "\n";

    return hash;
}

size_t WriteData(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int ProgressFunc(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded)
{
    int totaldotz = 210;
    double fractiondownloaded = NowDownloaded / TotalToDownload;
    int newProgress = (int)round(fractiondownloaded * totaldotz);

    if (newProgress > GetCurrentProgress())
    {
        duration<double> diff = high_resolution_clock::now() - startTime;
        if (diff.count() > 0.5)
        {
            SetWindowData(L"Downloading "+ currentFileName +L" " + std::to_wstring(int(fractiondownloaded * 100)) + L"%", newProgress);

            startTime = high_resolution_clock::now();
        }
    }

    return 0;
}

void DownloadFile(std::wstring path, std::string url, std::wstring name)
{
    CURL* curl;
    FILE* fp;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) 
   {
        currentFileName = name;
        SetWindowData(L"Downloading " + currentFileName, 0);
        _wfopen_s(&fp, path.c_str(), L"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        // Internal CURL progressmeter must be disabled if we provide our own callback
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
        // Install the callback function
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, ProgressFunc);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, TRUE);
        res = curl_easy_perform(curl);
        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);
    }
}

// write the data into a `std::string` rather than to a file.
std::size_t WriteDataAnother(void* buf, std::size_t size, std::size_t nmemb,
    void* userp)
{
    if (auto sp = static_cast<std::string*>(userp))
    {
        sp->append(static_cast<char*>(buf), size * nmemb);
        return size * nmemb;
    }

    return 0;
}

// To make the function thread safe you can use a smart pointer to
// hold your CURL session pointer.

// A deleter to use in the smart pointer for automatic cleanup
struct curl_dter {
    void operator()(CURL* curl) const
    {
        if (curl) curl_easy_cleanup(curl);
    }
};

// A smart pointer to automatically clean up out CURL session
using curl_uptr = std::unique_ptr<CURL, curl_dter>;

// download the URL into a `std::string`.
std::string GetUrl(std::string const& url)
{
    std::string data;

    if (auto curl = curl_uptr(curl_easy_init()))
    {
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteDataAnother);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &data);

        CURLcode ec;
        if ((ec = curl_easy_perform(curl.get())) != CURLE_OK)
            throw std::runtime_error(curl_easy_strerror(ec));

    }

    return data;
}

bool UnpackArchive(std::wstring path, std::wstring zipName)
{
    int err = 0;
    struct zip_file* zf;
    struct zip_stat sb;
    int fd;
    std::string zipPath = WideStringToString(path + zipName);
    std::string strPath = WideStringToString(path);
    zip* z = zip_open(zipPath.c_str(), 0, &err);

    int numEntries = zip_get_num_entries(z, 0);
    int totaldotz = 210;

    if (numEntries <= 0)
    {
        return false;
    }

    for (size_t i = 0; i < numEntries; i++)
    {
        if (zip_stat_index(z, i, 0, &sb) == 0)
        {
            int strLen = strlen(sb.name);
            if (sb.name[strLen - 1] == '/')
            {
                create_directory(strPath + "\\" + sb.name);
            }
            else
            {
                zf = zip_fopen_index(z, i, 0);
                _sopen_s(&fd, (strPath + "\\" + std::string(sb.name)).c_str(), O_RDWR | O_TRUNC | O_CREAT | O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);

                int sum = 0;
                byte* buf = new byte[100];
                while (sum != sb.size)
                {
                    int bLen = zip_fread(zf, buf, 100);
                    if (bLen >= 0) {
                        _write(fd, buf, bLen);
                        sum += bLen;
                    }
                }

                delete[] buf;

                _close(fd);

                zip_fclose(zf);

                double fractiondownloaded = double(i) / double(numEntries);
                int newProgress = (int)round(fractiondownloaded * totaldotz);

                if (newProgress > GetCurrentProgress()) {
                    duration<double> diff = high_resolution_clock::now() - startTime;
                    if (diff.count() > 0.5)
                    {
                        SetWindowData(L"Unpacking app... " + std::to_wstring(int(fractiondownloaded * 100)) + L"%", newProgress);

                        startTime = high_resolution_clock::now();
                    }
                }
            }
        }
    }

    zip_close(z);

    return true;
}