#include "utility/Archive.hpp"

#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

using namespace utility;

Archive::Archive()
	: mpInternal(nullptr)
{

}

Archive::~Archive()
{

}

bool Archive::open(std::filesystem::path const& path)
{
	this->mPath = path;
	return true;
}

void Archive::close()
{
	this->mPath.clear();
}

OutputArchive::OutputArchive()
	: Archive()
	, mPendingEntry(this)
{
}

OutputArchive::~OutputArchive()
{
	this->finish();
}

void OutputArchive::start()
{
	assert(this->mpInternal == nullptr);
	this->mpInternal = archive_write_new();
}

void OutputArchive::finish()
{
	if (this->mpInternal != nullptr)
	{
		archive_write_free(this->mpInternal);
		this->mpInternal = nullptr;
	}
}

bool OutputArchive::open(std::filesystem::path const& path)
{
	assert(this->mpInternal != nullptr);
	Archive::open(path);
	return archive_write_open_filename(this->mpInternal, path.string().c_str()) == ARCHIVE_OK;
}

void OutputArchive::close()
{
	assert(this->mpInternal != nullptr);
	this->mPendingEntry.end();
	archive_write_close(this->mpInternal);
	Archive::close();
}

void OutputArchive::setFormat(EArchiveFormat format)
{
	assert(this->mpInternal != nullptr);
	switch (format)
	{
	case EArchiveFormat::e7Zip:
		archive_write_set_format_7zip(this->mpInternal);
		archive_write_add_filter_none(this->mpInternal);
		break;
	}
}

OutputArchive::Entry& OutputArchive::startEntry()
{
	assert(this->mpInternal != nullptr);
	assert(!this->mPendingEntry.mbIsInProgress);
	return this->mPendingEntry.start();
}

OutputArchive::Entry::Entry(OutputArchive* pArchive)
	: mpArchive(pArchive)
	, mpInternal(nullptr)
	, mbIsInProgress(false)
	, mSizeWritten(0)
{
}

OutputArchive::Entry::~Entry()
{
	this->end();
}

OutputArchive::Entry& OutputArchive::Entry::start()
{
	if (this->mpInternal == nullptr)
	{
		this->mpInternal = archive_entry_new();
	}
	else
	{
		this->clear();
	}
	this->setType(EArchiveFileType::eRegular);
	return *this;
}

OutputArchive::Entry& OutputArchive::Entry::setPath(std::filesystem::path const& path)
{
	archive_entry_set_pathname_utf8(this->mpInternal, path.string().c_str());
	return *this;
}

OutputArchive::Entry& OutputArchive::Entry::setSize(ui64 const& size)
{
	archive_entry_set_size(this->mpInternal, (i64)size);
	return *this;
}

OutputArchive::Entry& OutputArchive::Entry::setType(EArchiveFileType type)
{
	archive_entry_set_filetype(this->mpInternal, (ui16)type);
	return *this;
}

OutputArchive::Entry& OutputArchive::Entry::addPermission(EPermission perm)
{
	this->mPermissionFlags |= ui16(perm);
	archive_entry_set_perm(this->mpInternal, this->mPermissionFlags);
	return *this;
}

OutputArchive::Entry& OutputArchive::Entry::finishHeader()
{
	archive_write_header(this->mpArchive->mpInternal, this->mpInternal);
	this->mbIsInProgress = true;
	return *this;
}

OutputArchive::Entry& OutputArchive::Entry::append(void* data, uSize const& size)
{
	assert(this->mSizeWritten + size <= (uSize)archive_entry_size(this->mpInternal));
	archive_write_data(this->mpArchive->mpInternal, data, size);
	this->mSizeWritten += size;
	return *this;
}

void OutputArchive::Entry::finish()
{
	assert(this->mSizeWritten == (uSize)archive_entry_size(this->mpInternal));
	archive_write_finish_entry(this->mpArchive->mpInternal);
	this->clear();
}

void OutputArchive::Entry::clear()
{
	archive_entry_clear(this->mpInternal);
	this->mPermissionFlags = 0;
	this->mSizeWritten = 0;
	this->mbIsInProgress = false;
}

void OutputArchive::Entry::end()
{
	if (this->mpInternal != nullptr)
	{
		archive_entry_free(this->mpInternal);
		this->mpInternal = nullptr;
	}
}

InputArchive::InputArchive()
	: Archive()
	, mpPendingEntry(nullptr)
{

}

InputArchive::~InputArchive()
{
	this->finish();
}

void InputArchive::start()
{
	assert(this->mpInternal == nullptr);
	this->mpInternal = archive_read_new();
	archive_read_support_filter_all(this->mpInternal);
	archive_read_support_format_all(this->mpInternal);
}

void InputArchive::finish()
{
	if (this->mpInternal != nullptr)
	{
		archive_read_free(this->mpInternal);
		this->mpInternal = nullptr;
	}
}

bool InputArchive::open(std::filesystem::path const& path)
{
	assert(this->mpInternal != nullptr);
	Archive::open(path);
	return archive_read_open_filename(this->mpInternal, path.string().c_str(), 0) == ARCHIVE_OK;
}

void InputArchive::close()
{
	assert(this->mpInternal != nullptr);
	archive_read_close(this->mpInternal);
	Archive::close();
}

bool InputArchive::nextEntry()
{
	assert(this->mpInternal != nullptr);
	if (archive_read_next_header(this->mpInternal, &this->mpPendingEntry) == ARCHIVE_OK)
	{
		return true;
	}
	this->mpPendingEntry = nullptr;
	return false;
}

std::string InputArchive::entryPath() const
{
	assert(this->mpPendingEntry != nullptr);
	return archive_entry_pathname(this->mpPendingEntry);
}

uSize InputArchive::entrySize() const
{
	assert(this->mpPendingEntry != nullptr);
	return archive_entry_size(this->mpPendingEntry);
}

void InputArchive::copyEntryTo(void* dst) const
{
	assert(this->mpInternal != nullptr);
	assert(this->mpPendingEntry != nullptr);
	assert(dst != nullptr);
	archive_read_data(this->mpInternal, dst, this->entrySize());
}

void utility::archiveTestWrite()
{
	std::string file1Contents = "this is a test file in a zip/pak :)";
	std::string file2Contents = "this is a second file\nthat has 2 lines";

	utility::OutputArchive archive;
	archive.start();

	archive.setFormat(utility::EArchiveFormat::e7Zip);

	archive.open("archive-test.pak");

	archive.startEntry()
		.setPath("file1.txt")
		.addPermission(utility::EPermission::eRead)
		.addPermission(utility::EPermission::eReadNamedAttributes)
		.addPermission(utility::EPermission::eReadAttributes)
		.setSize(file1Contents.size())
		.finishHeader()
		.append(file1Contents.data(), file1Contents.size())
		.finish();

	archive.startEntry()
		.setPath("folder/file2.txt")
		.addPermission(utility::EPermission::eRead)
		.addPermission(utility::EPermission::eReadNamedAttributes)
		.addPermission(utility::EPermission::eReadAttributes)
		.setSize(file2Contents.size())
		.finishHeader()
		.append(file2Contents.data(), file2Contents.size())
		.finish();

	archive.close();

	archive.finish();
}

void utility::archiveTestRead()
{
	utility::InputArchive archive;
	archive.start();
	if (archive.open("archive-test.pak"))
	{
		while (archive.nextEntry())
		{
			auto const path = archive.entryPath();
			auto content = std::string(archive.entrySize(), '\0');
			archive.copyEntryTo(content.data());
		}
		archive.close();
	}
	archive.finish();
}
