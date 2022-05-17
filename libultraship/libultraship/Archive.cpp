#include "Archive.h"
#include "Resource.h"
#include "File.h"
#include "spdlog/spdlog.h"
#include "Utils/StringHelper.h"
#include "Lib/StrHash64.h"
#include <filesystem>

namespace Ship {
	Archive::Archive(const std::string& MainPath, bool enableWriting) : Archive(MainPath, "", enableWriting)
	{
		mainMPQ = nullptr;
	}

	Archive::Archive(const std::string& MainPath, const std::string& PatchesPath, bool enableWriting, bool genCRCMap) : MainPath(MainPath), PatchesPath(PatchesPath) {
		mainMPQ = nullptr;
		Load(enableWriting, genCRCMap);
	}

	Archive::~Archive() {
		Unload();
	}

	bool Archive::IsMainMPQValid()
	{
		return mainMPQ != nullptr;
	}

	std::shared_ptr<Archive> Archive::CreateArchive(const std::string& archivePath, int fileCapacity)
	{
		auto archive = std::make_shared<Archive>(archivePath, true);

		TCHAR* t_filename = new TCHAR[archivePath.size() + 1];
		t_filename[archivePath.size()] = 0;
		std::copy(archivePath.begin(), archivePath.end(), t_filename);

		bool success = SFileCreateArchive(t_filename, MPQ_CREATE_LISTFILE | MPQ_CREATE_ATTRIBUTES | MPQ_CREATE_ARCHIVE_V2, fileCapacity, &archive->mainMPQ);
		int error = GetLastError();

		delete[] t_filename;

		if (success)
		{
			archive->mpqHandles[archivePath] = archive->mainMPQ;
			return archive;
		}
		else
		{
			SPDLOG_ERROR("({}) We tried to create an archive, but it has fallen and cannot get up.");
			return nullptr;
		}
	}

	std::shared_ptr<File> Archive::LoadFile(const std::string& filePath, bool includeParent, std::shared_ptr<File> FileToLoad) {
		HANDLE fileHandle = NULL;

		if (FileToLoad == nullptr) {
			FileToLoad = std::make_shared<File>();
			FileToLoad->path = filePath;
		}

		if (!SFileOpenFileEx(mainMPQ, filePath.c_str(), 0, &fileHandle)) {
			SPDLOG_ERROR("({}) Failed to open file {} from mpq archive {}", GetLastError(), filePath.c_str(), MainPath.c_str());
			std::unique_lock<std::mutex> Lock(FileToLoad->FileLoadMutex);
			FileToLoad->bHasLoadError = true;
			return FileToLoad;
		}

		DWORD dwFileSize = SFileGetFileSize(fileHandle, 0);
		std::shared_ptr<char[]> fileData(new char[dwFileSize]);
		DWORD dwBytes;

		if (!SFileReadFile(fileHandle, fileData.get(), dwFileSize, &dwBytes, NULL)) {
			SPDLOG_ERROR("({}) Failed to read file {} from mpq archive {}", GetLastError(), filePath.c_str(), MainPath.c_str());
			if (!SFileCloseFile(fileHandle)) {
				SPDLOG_ERROR("({}) Failed to close file {} from mpq after read failure in archive {}", GetLastError(), filePath.c_str(), MainPath.c_str());
			}
			std::unique_lock<std::mutex> Lock(FileToLoad->FileLoadMutex);
			FileToLoad->bHasLoadError = true;
			return FileToLoad;
		}

		if (!SFileCloseFile(fileHandle)) {
			SPDLOG_ERROR("({}) Failed to close file {} from mpq archive {}", GetLastError(), filePath.c_str(), MainPath.c_str());
		}

		std::unique_lock<std::mutex> Lock(FileToLoad->FileLoadMutex);
		FileToLoad->parent = includeParent ? shared_from_this() : nullptr;
		FileToLoad->buffer = fileData;
		FileToLoad->dwBufferSize = dwFileSize;
		FileToLoad->bIsLoaded = true;

		return FileToLoad;
	}

	std::shared_ptr<File> Archive::LoadPatchFile(const std::string& filePath, bool includeParent, std::shared_ptr<File> FileToLoad) {
		HANDLE fileHandle = NULL;
		HANDLE mpqHandle = NULL;

		if (FileToLoad == nullptr) {
			FileToLoad = std::make_shared<File>();
			FileToLoad->path = filePath;
		}

		for(auto [path, handle] : mpqHandles) {
			if (SFileOpenFileEx(mpqHandle, filePath.c_str(), 0, &fileHandle)) {
				std::unique_lock Lock(FileToLoad->FileLoadMutex);
				FileToLoad->bHasLoadError = false;
				mpqHandle = handle;
			}
		}

		if(!mpqHandle) {
			FileToLoad->bHasLoadError = true;
			return FileToLoad;
		}

		DWORD dwFileSize = SFileGetFileSize(fileHandle, 0);
		std::shared_ptr<char[]> fileData(new char[dwFileSize]);
		DWORD dwBytes;

		if (!SFileReadFile(fileHandle, fileData.get(), dwFileSize, &dwBytes, NULL)) {
			SPDLOG_ERROR("({}) Failed to read file {} from mpq archive {}", GetLastError(), filePath.c_str(), MainPath.c_str());
			if (!SFileCloseFile(fileHandle)) {
				SPDLOG_ERROR("({}) Failed to close file {} from mpq after read failure in archive {}", GetLastError(), filePath.c_str(), MainPath.c_str());
			}
			std::unique_lock<std::mutex> Lock(FileToLoad->FileLoadMutex);
			FileToLoad->bHasLoadError = true;
			return FileToLoad;
		}

		if (!SFileCloseFile(fileHandle)) {
			SPDLOG_ERROR("({}) Failed to close file {} from mpq archive {}", GetLastError(), filePath.c_str(), MainPath.c_str());
		}

		std::unique_lock<std::mutex> Lock(FileToLoad->FileLoadMutex);
		FileToLoad->parent = includeParent ? shared_from_this() : nullptr;
		FileToLoad->buffer = fileData;
		FileToLoad->dwBufferSize = dwFileSize;
		FileToLoad->bIsLoaded = true;

		return FileToLoad;
	}

	bool Archive::AddFile(const std::string& path, uintptr_t fileData, DWORD dwFileSize) {
		HANDLE hFile;
#ifdef _WIN32
		SYSTEMTIME sysTime;
		GetSystemTime(&sysTime);
		FILETIME t;
		SystemTimeToFileTime(&sysTime, &t);
		ULONGLONG stupidHack = static_cast<uint64_t>(t.dwHighDateTime) << (sizeof(t.dwHighDateTime) * 8) | t.dwLowDateTime;
#else
		time_t stupidHack;
		time(&stupidHack);
#endif
		if (!SFileCreateFile(mainMPQ, path.c_str(), stupidHack, dwFileSize, 0, MPQ_FILE_COMPRESS, &hFile)) {
			SPDLOG_ERROR("({}) Failed to create file of {} bytes {} in archive {}", GetLastError(), dwFileSize, path.c_str(), MainPath.c_str());
			return false;
		}

		if (!SFileWriteFile(hFile, (void*)fileData, dwFileSize, MPQ_COMPRESSION_ZLIB)) {
			SPDLOG_ERROR("({}) Failed to write {} bytes to {} in archive {}", GetLastError(), dwFileSize, path.c_str(), MainPath.c_str());
			if (!SFileCloseFile(hFile)) {
				SPDLOG_ERROR("({}) Failed to close file {} after write failure in archive {}", GetLastError(), path.c_str(), MainPath.c_str());
			}
			return false;
		}

		if (!SFileFinishFile(hFile)) {
			SPDLOG_ERROR("({}) Failed to finish file {} in archive {}", GetLastError(), path.c_str(), MainPath.c_str());
			if (!SFileCloseFile(hFile)) {
				SPDLOG_ERROR("({}) Failed to close file {} after finish failure in archive {}", GetLastError(), path.c_str(), MainPath.c_str());
			}
			return false;
		}
		// SFileFinishFile already frees the handle, so no need to close it again.

		addedFiles.push_back(path);
		hashes[CRC64(path.c_str())] = path;

		return true;
	}

	bool Archive::RemoveFile(const std::string& path) {
		// TODO: Notify the resource manager and child Files

		if (!SFileRemoveFile(mainMPQ, path.c_str(), 0)) {
			SPDLOG_ERROR("({}) Failed to remove file {} in archive {}", GetLastError(), path.c_str(), MainPath.c_str());
			return false;
		}

		return true;
	}

	bool Archive::RenameFile(const std::string& oldPath, const std::string& newPath) {
		// TODO: Notify the resource manager and child Files

		if (!SFileRenameFile(mainMPQ, oldPath.c_str(), newPath.c_str())) {
			SPDLOG_ERROR("({}) Failed to rename file {} to {} in archive {}", GetLastError(), oldPath.c_str(), newPath.c_str(), MainPath.c_str());
			return false;
		}

		return true;
	}

#if 1
	// Converts ASCII characters to uppercase
	// Converts slash (0x2F) to backslash (0x5C)
	static const unsigned char AsciiToUpperTable[256] =
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x5C,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
		0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
		0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
		0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
		0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
		0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
		0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
		0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
	};

	static bool SFileCheckWildCard(const char * szString, const char * szWildCard)
	{
		const char * szWildCardPtr;

		for(;;)
		{
			// If there is '?' in the wildcard, we skip one char
			while(szWildCard[0] == '?')
			{
				if(szString[0] == 0)
					return false;

				szWildCard++;
				szString++;
			}

			// Handle '*'
			szWildCardPtr = szWildCard;
			if(szWildCardPtr[0] != 0)
			{
				if(szWildCardPtr[0] == '*')
				{
					while(szWildCardPtr[0] == '*')
						szWildCardPtr++;

					if(szWildCardPtr[0] == 0)
						return true;

					if(AsciiToUpperTable[(int) szWildCardPtr[0]] == AsciiToUpperTable[(int) szString[0]])
					{
						if(SFileCheckWildCard(szString, szWildCardPtr))
							return true;
					}
				}
				else
				{
					if(AsciiToUpperTable[(int) szWildCardPtr[0]] != AsciiToUpperTable[(int) szString[0]])
						return false;

					szWildCard = szWildCardPtr + 1;
				}

				if(szString[0] == 0)
					return false;
				szString++;
			}
			else
			{
				return (szString[0] == 0) ? true : false;
			}
		}
	}
#endif

	std::vector<std::string> Archive::ListFiles(const std::string& searchMask) const {
#if 1
		std::vector<std::string> fileList;

		for (auto& file : filenameList) {
			if (SFileCheckWildCard(file.c_str(), searchMask.c_str())) {
				fileList.push_back(file);
			}
		}

		return fileList;
#else
		auto fileList = std::vector<SFILE_FIND_DATA>();
		SFILE_FIND_DATA findContext;
		HANDLE hFind;


		hFind = SFileFindFirstFile(mainMPQ, searchMask.c_str(), &findContext, nullptr);
		//if (hFind && GetLastError() != ERROR_NO_MORE_FILES) {
		if (hFind != nullptr) {
			fileList.push_back(findContext);

			bool fileFound;
			do {
				fileFound = SFileFindNextFile(hFind, &findContext);

				if (fileFound) {
					fileList.push_back(findContext);
				}
				else if (!fileFound && GetLastError() != ERROR_NO_MORE_FILES)
				//else if (!fileFound)
				{
					SPDLOG_ERROR("({}), Failed to search with mask {} in archive {}", GetLastError(), searchMask.c_str(), MainPath.c_str());
					if (!SListFileFindClose(hFind)) {
						SPDLOG_ERROR("({}) Failed to close file search {} after failure in archive {}", GetLastError(), searchMask.c_str(), MainPath.c_str());
					}
					return fileList;
				}
			} while (fileFound);
		}
		else if (GetLastError() != ERROR_NO_MORE_FILES) {
			SPDLOG_ERROR("({}), Failed to search with mask {} in archive {}", GetLastError(), searchMask.c_str(), MainPath.c_str());
			return fileList;
		}

		if (hFind != nullptr)
		{
			if (!SFileFindClose(hFind)) {
				SPDLOG_ERROR("({}) Failed to close file search {} in archive {}", GetLastError(), searchMask.c_str(), MainPath.c_str());
			}
		}

		return fileList;
#endif
	}

	bool Archive::HasFile(const std::string& filename) const {
		bool result = false;
		auto start = std::chrono::steady_clock::now();

		auto lst = ListFiles(filename);

		for (const auto& item : lst) {
			if (item == filename) {
				result = true;
				break;
			}
		}

		auto end = std::chrono::steady_clock::now();
		auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		return result;
	}

	const std::string* Archive::HashToString(uint64_t hash) const {
		auto it = hashes.find(hash);
		return it != hashes.end() ? &it->second : nullptr;
	}

	bool Archive::Load(bool enableWriting, bool genCRCMap) {
		return LoadMainMPQ(enableWriting, genCRCMap) && LoadPatchMPQs();
	}

	bool Archive::Unload()
	{
		bool success = true;
		for (const auto& mpqHandle : mpqHandles) {
			if (!SFileCloseArchive(mpqHandle.second)) {
				SPDLOG_ERROR("({}) Failed to close mpq {}", GetLastError(), mpqHandle.first.c_str());
				success = false;
			}
		}

		mainMPQ = nullptr;

		return success;
	}

	bool Archive::LoadPatchMPQs() {
		// OTRTODO: We also want to periodically scan the patch directories for new MPQs. When new MPQs are found we will load the contents to fileCache and then copy over to gameResourceAddresses
		if (PatchesPath.length() > 0) {
			if (std::filesystem::is_directory(PatchesPath)) {
				for (const auto& p : std::filesystem::recursive_directory_iterator(PatchesPath)) {
					if (StringHelper::IEquals(p.path().extension().string(), ".otr") || StringHelper::IEquals(p.path().extension().string(), ".mpq")) {
						SPDLOG_ERROR("Reading {} mpq patch", p.path().string().c_str());
						if (!LoadPatchMPQ(p.path().string())) {
							return false;
						}
					}
				}
			}
		}

		return true;
	}

	bool Archive::LoadMainMPQ(bool enableWriting, bool genCRCMap) {
		HANDLE mpqHandle = NULL;
#ifdef _WIN32
		std::wstring wfullPath = std::filesystem::absolute(MainPath).wstring();
#endif
		std::string fullPath = std::filesystem::absolute(MainPath).string();

		DWORD dwFlags = 0;

#if 1
		// parsing the listfile takes really long so just skip opening it
		// Handle listing files based on the listfile manually below
		dwFlags |= MPQ_OPEN_NO_LISTFILE;
#endif

		if (!enableWriting) {
			dwFlags |= MPQ_OPEN_READ_ONLY;
		}

#ifdef _MSC_VER
		if (!SFileOpenArchive(wfullPath.c_str(), 0, dwFlags, &mpqHandle)) {
#else
		if (!SFileOpenArchive(fullPath.c_str(), 0, dwFlags, &mpqHandle)) {
#endif
			SPDLOG_ERROR("({}) Failed to open main mpq file {}.", GetLastError(), fullPath.c_str());
			return false;
		}

		mpqHandles[fullPath] = mpqHandle;
		mainMPQ = mpqHandle;

		filenameList.clear();

		auto listFile = LoadFile("(listfile)", false);

		std::vector<std::string> lines = StringHelper::Split(std::string(listFile->buffer.get(), listFile->dwBufferSize), "\n");

		for (size_t i = 0; i < lines.size(); i++) {
			std::string line = StringHelper::Strip(lines[i], "\r");
			filenameList.push_back(line);
		}

		if (genCRCMap) {
			for (size_t i = 0; i < filenameList.size(); i++) {
				uint64_t hash = CRC64(filenameList[i].c_str());
				hashes[hash] = filenameList[i];
			}
		}

		return true;
	}

	bool Archive::LoadPatchMPQ(const std::string& path) {
		HANDLE patchHandle = NULL;
		std::string fullPath = std::filesystem::absolute(path).string();
		if (mpqHandles.contains(fullPath)) {
			return true;
		}

		std::wstring wPath = std::filesystem::absolute(path).wstring();

#ifdef _WIN32
		if (!SFileOpenArchive(wPath.c_str(), 0, MPQ_OPEN_READ_ONLY, &patchHandle)) {
#else
		if (!SFileOpenArchive(fullPath.c_str(), 0, MPQ_OPEN_READ_ONLY, &patchHandle)) {
#endif
			SPDLOG_ERROR("({}) Failed to open patch mpq file {} while applying to {}.", GetLastError(), path.c_str(), MainPath.c_str());
			return false;
		}
#ifdef _WIN32
		if (!SFileOpenPatchArchive(mainMPQ, wPath.c_str(), "", 0)) {
#else
		if (!SFileOpenPatchArchive(mainMPQ, fullPath.c_str(), "", 0)) {
#endif
			SPDLOG_ERROR("({}) Failed to apply patch mpq file {} to main mpq {}.", GetLastError(), path.c_str(), MainPath.c_str());
			return false;
		}

		mpqHandles[fullPath] = patchHandle;

		return true;
	}
}