#include "crypto/Crypto.hpp"

#include <openssl/evp.h>

using namespace crypto;

Crypto::Crypto()
	: mpEncryptCtx(EVP_CIPHER_CTX_new())
	, mpDecryptCtx(EVP_CIPHER_CTX_new())
{
}

Crypto::~Crypto()
{
	EVP_CIPHER_CTX_free((EVP_CIPHER_CTX*)this->mpEncryptCtx);
	EVP_CIPHER_CTX_free((EVP_CIPHER_CTX*)this->mpDecryptCtx);
}
