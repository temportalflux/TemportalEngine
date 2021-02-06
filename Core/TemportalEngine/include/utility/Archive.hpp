#pragma once

#include "TemportalEnginePCH.hpp"

using Archive = struct archive;
using ArchiveEntry = struct archive_entry;

NS_UTILITY

enum class EArchiveFormat
{
	e7Zip,
};

enum class EArchiveCompression
{
	eGZip,
};

// Mirrors `AE_IF<shortname>` in `libarchive/archive_entry.h`
enum class EArchiveFileType : ui16
{
	eRegular = 0100000,
	eDirectory = 0040000,

	eLink = 0120000,
	eSocket = 0140000,

	eMT = 0170000,
	eChr = 0020000,
	eBlk = 0060000,
	eIfo = 0010000,
};

// Mirrors the "Permission bits" in `libarchive/archive_entry.h`
enum class EPermission : ui16
{
	eExecute = 0x00000001,
	eWrite = 0x00000002,
	eRead = 0x00000004,
	eReadData = 0x00000008,
	eListDirectory = 0x00000008,
	eWriteData = 0x00000010,
	eAddFile = 0x00000010,
	eAppendData = 0x00000020,
	eAddSubdirectory = 0x00000020,
	eReadNamedAttributes = 0x00000040,
	eWriteNamedAttributes = 0x00000080,
	eDeleteChild = 0x00000100,
	eReadAttributes = 0x00000200,
	eWriteAttributes = 0x00000400,
	eDelete = 0x00000800,
	eReadACL = 0x00001000,
	eWriteACL = 0x00002000,
	eWriteOwner = 0x00004000,
	eSync = 0x00008000,
};

class OutputArchive;

class OutputArchiveEntry
{
	friend class OutputArchive;

public:
	OutputArchiveEntry& setPath(std::filesystem::path const& path);
	OutputArchiveEntry& setSize(ui64 const& size);
	OutputArchiveEntry& setType(EArchiveFileType type);
	OutputArchiveEntry& addPermission(EPermission perm);
	OutputArchiveEntry& finishHeader();

	OutputArchiveEntry& append(void* data, uSize const& size);

	void finish();

private:
	OutputArchive* mpArchive;
	ArchiveEntry* mpInternal;
	ui16 mPermissionFlags;
	uSize mSizeWritten;

	OutputArchiveEntry(OutputArchive* pArchive);
	~OutputArchiveEntry();
	OutputArchiveEntry& start();
	void clear();
	void end();

};

class OutputArchive
{
	friend class OutputArchiveEntry;

public:
	OutputArchive();
	~OutputArchive();

	void setFormat(EArchiveFormat format);
	void setCompression(EArchiveCompression compression);
	
	void start();
	void finish();

	void open(std::filesystem::path const& path);
	void close();

	OutputArchiveEntry& startEntry();

private:
	std::filesystem::path mPath;
	Archive* mpInternal;
	OutputArchiveEntry mPendingEntry;

	Archive* get();

};

void archiveTestWrite();
void archiveTestRead();

NS_END
