#pragma once

#include "TemportalEnginePCH.hpp"

using LibArchive = struct archive;
using LibArchiveEntry = struct archive_entry;

NS_UTILITY

enum class EArchiveFormat
{
	e7Zip,
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

class Archive
{
public:
	Archive();
	virtual ~Archive();
	
	virtual void start() = 0;
	virtual void finish() = 0;

	virtual bool open(std::filesystem::path const& path);
	virtual void close();

protected:
	LibArchive* mpInternal;

private:
	std::filesystem::path mPath;

};

class OutputArchive : public Archive
{
	friend class OutputArchiveEntry;

public:

	class Entry
	{
		friend class OutputArchive;

	public:
		Entry& setPath(std::filesystem::path const& path);
		Entry& setSize(ui64 const& size);
		Entry& setType(EArchiveFileType type);
		Entry& addPermission(EPermission perm);
		Entry& finishHeader();

		Entry& append(void* data, uSize const& size);

		void finish();

	private:
		OutputArchive* mpArchive;
		LibArchiveEntry* mpInternal;
		ui16 mPermissionFlags;
		uSize mSizeWritten;

		Entry(OutputArchive* pArchive);
		~Entry();
		Entry& start();
		void clear();
		void end();

	};

	OutputArchive();
	~OutputArchive();

	void setFormat(EArchiveFormat format);
	
	void start() override;
	void finish() override;

	bool open(std::filesystem::path const& path) override;
	void close() override;

	Entry& startEntry();

private:
	Entry mPendingEntry;

	LibArchive* get();

};

class InputArchive : public Archive
{

public:
	InputArchive();
	~InputArchive();

	void start() override;
	void finish() override;

	bool open(std::filesystem::path const& path) override;
	void close() override;

	bool nextEntry();
	std::string entryPath() const;
	uSize entrySize() const;
	void copyEntryTo(void* dst) const;

private:
	LibArchiveEntry* mpPendingEntry;

};

void archiveTestWrite();
void archiveTestRead();

NS_END
