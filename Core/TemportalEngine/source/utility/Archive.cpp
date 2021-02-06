#include "utility/Archive.hpp"

#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

using namespace utility;

OutputArchive::OutputArchive()
	: mpInternal(nullptr)
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

void OutputArchive::open(std::filesystem::path const& path)
{
	assert(this->mpInternal != nullptr);
	this->mPath = path;
	archive_write_open_filename(this->mpInternal, path.string().c_str());
}

void OutputArchive::close()
{
	assert(this->mpInternal != nullptr);
	this->mPendingEntry.end();
	archive_write_close(this->mpInternal);
	this->mPath.clear();
}

void OutputArchive::setFormat(EArchiveFormat format)
{
	assert(this->mpInternal != nullptr);
	switch (format)
	{
	case EArchiveFormat::e7Zip:
		archive_write_set_format_7zip(this->mpInternal);
		break;
	}
}

void OutputArchive::setCompression(EArchiveCompression compression)
{
	assert(this->mpInternal != nullptr);
	switch (compression)
	{
	case EArchiveCompression::eGZip:
		archive_write_add_filter_none(this->mpInternal);
		break;
	}
}

Archive* OutputArchive::get()
{
	return this->mpInternal;
}

OutputArchiveEntry& OutputArchive::startEntry()
{
	assert(this->mpInternal != nullptr);
	return this->mPendingEntry.start();
}

OutputArchiveEntry::OutputArchiveEntry(OutputArchive* pArchive)
	: mpArchive(pArchive)
	, mpInternal(nullptr)
{
}

OutputArchiveEntry::~OutputArchiveEntry()
{
	this->end();
}

OutputArchiveEntry& OutputArchiveEntry::start()
{
	if (this->mpInternal == nullptr)
	{
		this->mpInternal = archive_entry_new();
	}
	else
	{
		this->clear();
	}
	return *this;
}

OutputArchiveEntry& OutputArchiveEntry::setPath(std::filesystem::path const& path)
{
	archive_entry_set_pathname_utf8(this->mpInternal, path.string().c_str());
	return *this;
}

OutputArchiveEntry& OutputArchiveEntry::setSize(ui64 const& size)
{
	archive_entry_set_size(this->mpInternal, (i64)size);
	return *this;
}

OutputArchiveEntry& OutputArchiveEntry::setType(EArchiveFileType type)
{
	archive_entry_set_filetype(this->mpInternal, (ui16)type);
	return *this;
}

OutputArchiveEntry& OutputArchiveEntry::addPermission(EPermission perm)
{
	this->mPermissionFlags |= ui16(perm);
	archive_entry_set_perm(this->mpInternal, this->mPermissionFlags);
	return *this;
}

OutputArchiveEntry& OutputArchiveEntry::finishHeader()
{
	archive_write_header(this->mpArchive->get(), this->mpInternal);
	return *this;
}

OutputArchiveEntry& OutputArchiveEntry::append(void* data, uSize const& size)
{
	assert(this->mSizeWritten + size <= (uSize)archive_entry_size(this->mpInternal));
	archive_write_data(this->mpArchive->get(), data, size);
	this->mSizeWritten += size;
	return *this;
}

void OutputArchiveEntry::finish()
{
	archive_write_finish_entry(this->mpArchive->get());
	this->clear();
}

void OutputArchiveEntry::clear()
{
	archive_entry_clear(this->mpInternal);
	this->mPermissionFlags = 0;
	this->mSizeWritten = 0;
}

void OutputArchiveEntry::end()
{
	if (this->mpInternal != nullptr)
	{
		archive_entry_free(this->mpInternal);
		this->mpInternal = nullptr;
	}
}

void archiveTestWrite()
{
	std::string file1Contents = "this is a test file in a zip/pak :)";
	std::string file2Contents = "this is a second file\nthat has 2 lines";

	utility::OutputArchive archive;
	archive.start();

	archive.setFormat(utility::EArchiveFormat::e7Zip);
	archive.setCompression(utility::EArchiveCompression::eGZip);

	archive.open("archive-test.pak");

	archive.startEntry()
		.setType(utility::EArchiveFileType::eRegular)
		.setPath("file1.txt")
		.addPermission(utility::EPermission::eRead)
		.addPermission(utility::EPermission::eReadNamedAttributes)
		.addPermission(utility::EPermission::eReadAttributes)
		.setSize(file1Contents.size())
		.finishHeader()
		.append(file1Contents.data(), file1Contents.size())
		.finish();

	archive.startEntry()
		.setType(utility::EArchiveFileType::eRegular)
		.setPath("file2.txt")
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

void archiveTestRead()
{

}
