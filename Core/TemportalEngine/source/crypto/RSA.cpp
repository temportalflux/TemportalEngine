#include "crypto/RSA.hpp"

#include <openssl/evp.h>
#include <openssl/pem.h>

using namespace crypto;

std::shared_ptr<crypto::RSA> Singleton<crypto::RSA>::gpInstance = nullptr;

std::optional<std::string> bioToString(BIO *bio)
{
	ui32 length = BIO_pending(bio);
	auto str = std::string(length, '\0');
	if (BIO_read(bio, str.data(), length) <= 0) return std::nullopt;
	return str;
}

RSAKey::RSAKey() : mpInternal(nullptr)
{
}

RSAKey::RSAKey(RSAKey &&other) { *this = std::move(other); }
RSAKey& RSAKey::operator=(RSAKey &&other)
{
	this->mpInternal = other.mpInternal;
	other.mpInternal = nullptr;
	return *this;
}

RSAKey::~RSAKey()
{
	if (this->mpInternal != nullptr)
	{
		EVP_PKEY_free((EVP_PKEY*)this->mpInternal);
		this->mpInternal = nullptr;
	}
}

bool RSAKey::generate()
{
	assert(this->mpInternal == nullptr);

	bool bSuccess = true;
	EVP_PKEY_CTX *pCtx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);

	if (bSuccess && EVP_PKEY_keygen_init(pCtx) <= 0) bSuccess = false;
	if (bSuccess && EVP_PKEY_CTX_set_rsa_keygen_bits(pCtx, 2048) <= 0) bSuccess = false;
	
	EVP_PKEY *pKeyPair = nullptr;
	if (bSuccess && EVP_PKEY_keygen(pCtx, &pKeyPair) <= 0) bSuccess = false;
	this->mpInternal = pKeyPair;

	EVP_PKEY_CTX_free(pCtx);
	return bSuccess;
}

bool RSAKey::fromPublicString(std::string const& str, RSAKey &outKey)
{
	bool bSuccess = true;
	auto length = (ui32)str.length();
	BIO *bio = BIO_new(BIO_s_mem());
	if (BIO_write(bio, str.data(), length) != length) bSuccess = false;
	if (bSuccess) PEM_read_bio_PUBKEY(bio, (EVP_PKEY**)(&outKey.mpInternal), nullptr, nullptr);
	BIO_free_all(bio);
	return bSuccess;
}

bool RSAKey::fromPrivateString(std::string const& str, RSAKey &outKey)
{
	bool bSuccess = true;
	auto length = (ui32)str.length();
	BIO *bio = BIO_new(BIO_s_mem());
	if (BIO_write(bio, str.data(), length) != length) bSuccess = false;
	if (bSuccess) PEM_read_bio_PrivateKey(bio, (EVP_PKEY**)(&outKey.mpInternal), nullptr, nullptr);
	BIO_free_all(bio);
	return bSuccess;
}

std::optional<std::string> RSAKey::publicKey() const
{
	std::optional<std::string> ret = std::nullopt;
	BIO *bio = BIO_new(BIO_s_mem());
	if (PEM_write_bio_PUBKEY(bio, (EVP_PKEY*)this->mpInternal) > 0)
	{
		ret = bioToString(bio);
	}
	BIO_free_all(bio);
	return ret;
}

std::optional<std::string> RSAKey::privateKey() const
{
	std::optional<std::string> ret = std::nullopt;
	BIO *bio = BIO_new(BIO_s_mem());
	if (PEM_write_bio_PrivateKey(
		bio, (EVP_PKEY*)this->mpInternal,
		nullptr, nullptr, 0, nullptr, nullptr
	) > 0)
	{
		ret = bioToString(bio);
	}
	BIO_free_all(bio);
	return ret;
}
