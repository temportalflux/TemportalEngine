#include "crypto/RSA.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"

#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <sstream>

using namespace crypto;

logging::Logger RSA_LOG = DeclareLog("crypto:RSA", LOG_INFO);

std::shared_ptr<crypto::RSA> Singleton<crypto::RSA>::gpInstance = nullptr;

void logErrors()
{
	while (true)
	{
		auto errCode = ERR_get_error();
		if (errCode == 0) break;
		auto errStr = std::string(256, '\0');
		ERR_error_string_n(errCode, errStr.data(), errStr.length());
		RSA_LOG.log(LOG_ERR, errStr.c_str());
	}
}

std::optional<std::string> bioToString(BIO *bio)
{
	ui32 length = BIO_pending(bio);
	auto str = std::string(length, '\0');
	if (BIO_read(bio, str.data(), length) <= 0) return std::nullopt;
	return str;
}

RSAKey::RSAKey() : mpInternal(nullptr), mbIsPrivate(false)
{
}

RSAKey::RSAKey(RSAKey const& other) { *this = other; }
RSAKey& RSAKey::operator=(RSAKey const& other)
{
	if (this->mpInternal == nullptr) this->mpInternal = EVP_PKEY_new();
	auto* rsa = EVP_PKEY_get1_RSA((EVP_PKEY*)other.mpInternal);
	EVP_PKEY_set1_RSA((EVP_PKEY*)this->mpInternal, RSAPrivateKey_dup(rsa));
	RSA_free(rsa);
	this->mbIsPrivate = other.mbIsPrivate;
	return *this;
}

RSAKey::RSAKey(RSAKey &&other) { *this = std::move(other); }
RSAKey& RSAKey::operator=(RSAKey &&other)
{
	this->mpInternal = other.mpInternal;
	other.mpInternal = nullptr;
	this->mbIsPrivate = other.mbIsPrivate;
	other.mbIsPrivate = false;
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
	assert(pCtx != nullptr);

	if (bSuccess && EVP_PKEY_keygen_init(pCtx) <= 0)
	{
		bSuccess = false;
		logErrors();
	}
	if (bSuccess && EVP_PKEY_CTX_set_rsa_keygen_bits(pCtx, 2048) <= 0)
	{
		bSuccess = false;
		logErrors();
	}
	
	EVP_PKEY *pKeyPair = nullptr;
	if (bSuccess && EVP_PKEY_keygen(pCtx, &pKeyPair) <= 0)
	{
		bSuccess = false;
		logErrors();
	}
	this->mpInternal = pKeyPair;

	EVP_PKEY_CTX_free(pCtx);
	this->mbIsPrivate = true;
	return bSuccess;
}

bool RSAKey::fromPublicString(std::string const& str, RSAKey &outKey)
{
	bool bSuccess = true;
	auto length = (ui32)str.length();
	BIO *bio = BIO_new(BIO_s_mem());
	if (BIO_write(bio, str.data(), length) != length)
	{
		bSuccess = false;
		logErrors();
	}
	if (bSuccess) PEM_read_bio_PUBKEY(bio, (EVP_PKEY**)(&outKey.mpInternal), nullptr, nullptr);
	BIO_free_all(bio);
	outKey.mbIsPrivate = false;
	return bSuccess;
}

bool RSAKey::fromPrivateString(std::string const& str, RSAKey &outKey)
{
	bool bSuccess = true;
	auto length = (ui32)str.length();
	BIO *bio = BIO_new(BIO_s_mem());
	if (BIO_write(bio, str.data(), length) != length)
	{
		bSuccess = false;
		logErrors();
	}
	if (bSuccess) PEM_read_bio_PrivateKey(bio, (EVP_PKEY**)(&outKey.mpInternal), nullptr, nullptr);
	BIO_free_all(bio);
	outKey.mbIsPrivate = true;
	return bSuccess;
}

bool RSAKey::isPrivate() const { return this->mbIsPrivate; }

std::optional<std::string> RSAKey::publicKey() const
{
	std::optional<std::string> ret = std::nullopt;
	BIO *bio = BIO_new(BIO_s_mem());
	if (PEM_write_bio_PUBKEY(bio, (EVP_PKEY*)this->mpInternal) > 0)
	{
		ret = bioToString(bio);
	}
	else
	{
		logErrors();
	}
	BIO_free_all(bio);
	return ret;
}

std::optional<std::string> RSAKey::privateKey() const
{
	assert(this->isPrivate());
	std::optional<std::string> ret = std::nullopt;
	BIO *bio = BIO_new(BIO_s_mem());
	if (PEM_write_bio_PrivateKey(
		bio, (EVP_PKEY*)this->mpInternal,
		nullptr, nullptr, 0, nullptr, nullptr
	) > 0)
	{
		ret = bioToString(bio);
	}
	else
	{
		logErrors();
	}
	BIO_free_all(bio);
	return ret;
}

template <uSize TSize>
void strToBinary(std::string const& src, std::array<ui8, TSize> &dst)
{
	assert(src.length() == dst.size());
	memcpy_s(dst.data(), sizeof(dst), src.c_str(), sizeof(dst));
}

template <uSize TSize>
std::string binaryToStr(std::array<ui8, TSize> const& src)
{
	auto dst = std::string(src.size(), '\0');
	assert(src.size() == dst.length());
	memcpy_s(dst.data(), sizeof(src), src.data(), sizeof(src));
	return dst;
}

RSAKey::PublicData RSAKey::publicData() const
{
	PublicData out;

	auto str = this->publicKey();
	auto content = utility::split(*str, '\n');
	assert(content.size() == out.n.size() + 3 /*header, exponent, footer*/);
	uIndex slot = 1; // start after the header
	for (uIndex i = 0; i < out.n.size(); ++i)
	{
		strToBinary(content[slot++], out.n[i]);
	}
	strToBinary(content[slot++], out.e);

	return out;
}

void RSAKey::fromPublicData(PublicData const& data, RSAKey &outKey)
{
	std::stringstream ss;
	ss << "-----BEGIN PUBLIC KEY-----\n";
	for (auto const& row : data.n)
	{
		ss << binaryToStr(row).c_str() << '\n';
	}
	ss << binaryToStr(data.e).c_str() << '\n';
	ss << "-----END PUBLIC KEY-----\n";
	std::string str = ss.str();
	auto bSuccessful = outKey.fromPublicString(str, outKey);
	assert(bSuccessful);
}

void RSAKey::encrypt(std::vector<ui8> &data) const
{
	auto* rsa = EVP_PKEY_get0_RSA((EVP_PKEY*)this->mpInternal);
	auto outData = std::vector<ui8>(RSA_size(rsa));
	i32 result = RSA_public_encrypt(
		RSA_size(rsa) - 42,
		data.data(), outData.data(),
		rsa,
		RSA_PKCS1_OAEP_PADDING
	);
	if (result == -1)
	{
		logErrors();
		assert(false);
	}
	data = outData;
}

void RSAKey::decrypt(std::vector<ui8> &data) const
{
	assert(this->isPrivate());
	auto* rsa = EVP_PKEY_get0_RSA((EVP_PKEY*)this->mpInternal);
	auto outData = std::vector<ui8>(RSA_size(rsa));
	i32 result = RSA_private_decrypt(
		RSA_size(rsa),
		data.data(), outData.data(),
		rsa,
		RSA_PKCS1_OAEP_PADDING
	);
	if (result == -1)
	{
		logErrors();
		assert(false);
	}
	data = outData;
}

std::string toPacketDebugString(crypto::RSAKey::PublicData const& value)
{
	// sprintf(hexstr+i*2, "%02x", array[i]);
	return std::to_string(sizeof(value)) + " bytes";
}

void network::write(network::Buffer &buffer, std::string name, crypto::RSAKey::PublicData const& value)
{
	buffer.setNamed(name, toPacketDebugString(value));
	buffer.writeRaw(value);
}

void network::read(network::Buffer &buffer, std::string name, crypto::RSAKey::PublicData &value)
{
	buffer.readRaw(value);
	buffer.setNamed(name, toPacketDebugString(value));
}
