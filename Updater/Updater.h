#pragma once

// // При включении SDKDDKVer.h будет задана самая новая из доступных платформ Windows.
// Если вы планируете сборку приложения для предыдущей версии платформы Windows, включите WinSDKVer.h и
// задайте желаемую платформу в макросе _WIN32_WINNT, прежде чем включать SDKDDKVer.h.
#include <SDKDDKVer.h>

#define API_URL "https://api.umbrella.re"
#define CDN_URL "https://cdn.umbrella.re"
#define DOWNLOAD "/toolkitv/download"
#define DOWNLOAD_FILE "/toolkitv/files/"
#define GET_UPDATER "/toolkitv/updater"
#define GET_CACHES "/toolkitv/getCurrentFileCaches"
#define BACKUP_DOWNLOAD_URL "https://github.com/UmbrellaRE/ToolKitV/releases/latest/download/ToolKitV.zip"
#define PRODUCT_NAME L"ToolKitV"