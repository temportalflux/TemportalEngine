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

/**
 * Generic base-class for both `Input` and `Output` archives.
 */
class Archive
{
public:
	Archive();
	virtual ~Archive();
	
	/**
	 * Begins archive management.
	 * Will initialize and allocate the archiving library to manage some archive.
	 */
	virtual void start() = 0;

	/**
	 * Ends archive management.
	 * Will release any allocated memory for the library.
	 */
	virtual void finish() = 0;

	/**
	 * Opens an archive at a provided filepath.
	 * Returns false if the archive failed to load.
	 * Can only be called after `start` and before `finish`.
	 */
	virtual bool open(std::filesystem::path const& path);
	
	/**
	 * Closes an archive file descriptor.
	 * Can only be called after `open` and before `finish`.
	 */
	virtual void close();

protected:
	/**
	 * The internal library pointer (provided by `libarchive`).
	 */
	LibArchive* mpInternal;

private:
	/**
	 * The path of the currently open archive file.
	 */
	std::filesystem::path mPath;

};

/**
 * An implementation of `Archive` for outputting data to an archive created by `libarchive`.
 */
class OutputArchive : public Archive
{

public:

	/**
	 * An entry created for writing to the archive.
	 */
	class Entry
	{
		friend class OutputArchive;

	public:
		
		/**
		 * Sets the relative path in the archive that the entry will be written to.
		 * Must be called before `finishHeader` (it is a required field).
		 */
		Entry& setPath(std::filesystem::path const& path);

		/**
		 * Sets the size of the uncompressed file entry in the archive.
		 * Must be called before `finishHeader` (it is a required field).
		 */
		Entry& setSize(ui64 const& size);

		/**
		 * Sets the file-type of the file in the archive.
		 * Defaults to `eRegular`.
		 * If a non-regular file needs to be specified, this function must be called before `finishHeader`.
		 */
		Entry& setType(EArchiveFileType type);

		/**
		 * Sets any permissions on the file-entry.
		 * Permissions are empty by default, but most file writing will want to set
		 * the `eRead`, `eReadNamedAttributes`, and `eReadAttributes` permissions.
		 * Can only be called before `finishHeader`.
		 */
		Entry& addPermission(EPermission perm);

		/**
		 * Writes any metadata about the file to the archive.
		 * Once this is called, `append` MUST be called to write data to the archive.
		 * If this is never called, the entry is never written to the archive.
		 * Can only be called before `append` and `finish`.
		 */
		Entry& finishHeader();

		/**
		 * Appends data to the file in the archive.
		 * This function can be called multiple times, so long as the total size
		 * of bytes written is not greater than the size provided by `setSize`.
		 * Can only be called before `finish`.
		 * You must call this function if `finishHeader` is called.
		 */
		Entry& append(void* data, uSize const& size);

		/**
		 * Finalizes the file entry in the archive.
		 * Can only be called after `finishHeader` is called.
		 * Must be called only after the total number of bytes written by calls to `append`
		 * is equal to the number of bytes provided to `setSize` for the header.
		 * Once this function is called, no more operations on the file may be performed.
		 * You must use `OutputArchive.startEntry` to start a new entry.
		 */
		void finish();

	private:
		/**
		 * The owning output archive.
		 */
		OutputArchive* mpArchive;
		/**
		 * The `libarchive` pointer to the entry.
		 */
		LibArchiveEntry* mpInternal;

		/**
		 * A set of binary flags controlled by `EPermission` and `addPermission`.
		 */
		ui16 mPermissionFlags;

		/**
		 * True while the entry has performed `finishHeader` but not `finish`.
		 */
		bool mbIsInProgress;
		
		/**
		 * The total number of bytes written by `append`.
		 */
		uSize mSizeWritten;

		Entry(OutputArchive* pArchive);
		~Entry();

		/**
		 * Initializes the `mpInternal` entry, clears data, and initializes defaults.
		 */
		Entry& start();

		/**
		 * Clears any data in the local `mpInternal` entry.
		 */
		void clear();
		
		/**
		 * Frees the internal entry.
		 */
		void end();

	};

	OutputArchive();
	~OutputArchive();

	/**
	 * Begins archive management.
	 * Will initialize and allocate the archiving library to manage some archive.
	 */
	void start() override;

	/**
	 * Sets the format of the archive.
	 * Can only be called after `start` and before any archive is opened for writing.
	 */
	void setFormat(EArchiveFormat format);

	/**
	 * Ends archive management.
	 * Will release any allocated memory for the library.
	 */
	void finish() override;

	/**
	 * Opens an archive at a provided filepath.
	 * Returns false if the archive failed to load.
	 * Can only be called after `start` and before `finish`.
	 */
	bool open(std::filesystem::path const& path) override;
	
	/**
	 * Prepares an entry to be written to the archive.
	 * This does not actually insert an entry into the archive (see `Entry::finish`).
	 * If this is called multiple times, the entry pointer may be equivalent to previous pointers,
	 * but all data will have been cleared.
	 * Cannot be called if a previous call has finished the header but not finished the body
	 * (`Entry::finishHeader` has been called, but not `Entry::finish`).
	 */
	Entry& startEntry();
	
	/**
	 * Closes an archive file descriptor.
	 * Can only be called after `open` and before `finish`.
	 */
	void close() override;

private:
	/**
	 * The current entry being prepared.
	 */
	Entry mPendingEntry;

};

/**
 * An implementation of `Archive` for reading data from an archive.
 */
class InputArchive : public Archive
{

public:
	InputArchive();
	~InputArchive();

	/**
	 * Begins archive management.
	 * Will initialize and allocate the archiving library to manage some archive.
	 */
	void start() override;

	/**
	 * Ends archive management.
	 * Will release any allocated memory for the library.
	 */
	void finish() override;

	/**
	 * Opens an archive at a provided filepath.
	 * Returns false if the archive failed to load.
	 * Can only be called after `start` and before `finish`.
	 */
	bool open(std::filesystem::path const& path) override;

	/**
	 * Closes an archive file descriptor.
	 * Can only be called after `open` and before `finish`.
	 */
	void close() override;

	/**
	 * Returns true if there is an entry that can be read.
	 * If this is the first call, this will check the beginning of the archive.
	 * If this is a subsequent call, this will check for any entries after the current entry.
	 */
	bool nextEntry();

	/**
	 * Returns the relative internal file path to the current entry.
	 * Can only be called if `nextEntry` returns true.
	 */
	std::string entryPath() const;

	/**
	 * Returns the number of bytes for the current entry.
	 * Can only be called if `nextEntry` returns true.
	 */
	uSize entrySize() const;

	/**
	 * Copies the number of bytes returned by `entrySize` to `dst` for the current entry.
	 * Can only be called if `nextEntry` returns true.
	 */
	void copyEntryTo(void* dst) const;

private:
	LibArchiveEntry* mpPendingEntry;

};

void archiveTestWrite();
void archiveTestRead();

NS_END
