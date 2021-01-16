#pragma once

#include "crypto/Crypto.hpp"
#include "Singleton.hpp"
#include "network/data/NetworkDataBuffer.hpp"

NS_CRYPTO

class RSAKey
{
public:
	struct PublicData
	{
		using Segment = std::array<ui8, 64>;
		std::array<Segment, 6> n;
		std::array<ui8, 8> e;
	};

	RSAKey();
	RSAKey(RSAKey const& other);
	RSAKey& operator=(RSAKey const& other);
	RSAKey(RSAKey &&other);
	RSAKey& operator=(RSAKey &&other);
	~RSAKey();

	bool generate();
	bool isPrivate() const;

	std::optional<std::string> publicKey() const;
	static bool fromPublicString(std::string const& str, RSAKey &outKey);
	
	std::optional<std::string> privateKey() const;
	static bool fromPrivateString(std::string const& str, RSAKey &outKey);

	PublicData publicData() const;
	static void fromPublicData(PublicData const& data, RSAKey &outKey);

	template <typename T>
	std::vector<ui8> encrypt(T const& raw) const
	{
		auto data = std::vector<ui8>(sizeof(T) / sizeof(ui8));
		memcpy_s(data.data(), sizeof(T), &raw, sizeof(T));
		this->encrypt(data);
		return data;
	}

	template <typename T>
	T decrypt(std::vector<ui8> &encrypted) const
	{
		T out;
		this->decrypt(encrypted);
		memcpy_s(&out, sizeof(T), encrypted.data(), sizeof(T));
		return out;
	}

	void encrypt(std::vector<ui8> &data) const;
	void decrypt(std::vector<ui8> &data) const;
	
private:
	void* mpInternal;
	bool mbIsPrivate;

};

class RSA : public Crypto, public Singleton<RSA>
{

public:

};

NS_END

NS_NETWORK
void write(Buffer &buffer, std::string name, crypto::RSAKey::PublicData const& value);
void read(Buffer &buffer, std::string name, crypto::RSAKey::PublicData &value);
NS_END
