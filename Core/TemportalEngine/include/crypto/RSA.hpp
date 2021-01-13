#pragma once

#include "crypto/Crypto.hpp"
#include "Singleton.hpp"

NS_CRYPTO

class RSAKey
{
public:
	RSAKey();
	RSAKey(RSAKey const& other) = delete;
	RSAKey(RSAKey &&other);
	RSAKey& operator=(RSAKey &&other);
	~RSAKey();

	bool generate();
	static bool fromPublicString(std::string const& str, RSAKey &outKey);
	static bool fromPrivateString(std::string const& str, RSAKey &outKey);
	std::optional<std::string> publicKey() const;
	std::optional<std::string> privateKey() const;

private:
	void* mpInternal;

};

class RSA : public Crypto, public Singleton<RSA>
{

public:
	



};

NS_END
