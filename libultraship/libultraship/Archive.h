#pragma once

#undef _DLL

#include <string>

#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include "Resource.h"
//#include "Lib/StrHash64.h"
#ifdef __WIIU__
#include <StormLib.h>
#else
#include "Lib/StormLib/StormLib.h"
#endif

namespace Ship
{
	class File;

	class Archive : public std::enable_shared_from_this<Archive>
	{
	public:
		Archive(const std::string& MainPath, bool enableWriting);
		Archive(const std::string& MainPath, const std::string& PatchesPath, bool enableWriting, bool genCRCMap = true);
		~Archive();

		bool IsMainMPQValid();

		static std::shared_ptr<Archive> CreateArchive(const std::string& archivePath, int fileCapacity);
		
		std::shared_ptr<File> LoadFile(const std::string& filePath, bool includeParent = true, std::shared_ptr<File> FileToLoad = nullptr);
		std::shared_ptr<File> LoadPatchFile(const std::string& filePath, bool includeParent = true, std::shared_ptr<File> FileToLoad = nullptr);

		bool AddFile(const std::string& path, uintptr_t fileData, DWORD dwFileSize);
		bool RemoveFile(const std::string& path);
		bool RenameFile(const std::string& oldPath, const std::string& newPath);
		std::vector<std::string> ListFiles(const std::string& searchMask);
		bool HasFile(const std::string& searchMask);
		std::string HashToString(uint64_t hash);
	protected:
		bool Load(bool enableWriting, bool genCRCMap);
		bool Unload();
	private:
		std::string MainPath;
		std::string PatchesPath;
		std::unordered_map<std::string, HANDLE> mpqHandles;
		std::vector<std::string> addedFiles;
		std::unordered_map<uint64_t, std::string> hashes;
		std::vector<std::string> filenameList;
		HANDLE mainMPQ;

		bool LoadMainMPQ(bool enableWriting, bool genCRCMap);
		bool LoadPatchMPQs();
		bool LoadPatchMPQ(const std::string& path);
	};
}