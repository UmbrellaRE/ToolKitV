#pragma once

std::wstring GetRootPath();
bool Uninstall(const wchar_t* directory);
bool PerformInstallation(bool updateServerAvaliable);