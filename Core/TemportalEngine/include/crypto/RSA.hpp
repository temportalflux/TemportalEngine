#pragma once

#include "crypto/Crypto.hpp"
#include "Singleton.hpp"

NS_CRYPTO

class RSA : public Crypto, public Singleton<RSA>
{



};

NS_END
