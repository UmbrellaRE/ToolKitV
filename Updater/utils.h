#pragma once

#include <iostream>
#include <string>
#include <stdio.h>
#include <shlobj_core.h>
#include <filesystem>

#include <fcntl.h>
#include <thread>
#include <io.h>

std::wstring ExeName();
std::wstring ExePath();
bool DirOrFileExists(const std::wstring& dirName_in);
std::string WideStringToString(const std::wstring& wide_string);
std::vector<std::string> Split(const std::string& s, char delim);
std::string GetFileHash(std::wstring path);
void DownloadFile(std::wstring path, std::string url, std::wstring name);
std::string GetUrl(std::string const& url);
bool UnpackArchive(std::wstring path, std::wstring zipName);