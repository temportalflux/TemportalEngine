#pragma once

#include "TemportalEnginePCH.hpp"

#define NS_CRYPTO namespace crypto {

NS_CRYPTO

/*
	Using AES and RSA: https://www.precisely.com/blog/data-security/aes-vs-rsa-encryption-differences
	Implementing with OpenSSL: https://shanetully.com/2012/06/openssl-rsa-aes-and-c/
*/

class Crypto
{

public:
	Crypto();
	virtual ~Crypto();

protected:

private:
	void* mpEncryptCtx;
	void* mpDecryptCtx;

};

NS_END
