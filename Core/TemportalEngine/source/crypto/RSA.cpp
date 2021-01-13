#include "crypto/RSA.hpp"

using namespace crypto;

std::shared_ptr<RSA> Singleton<RSA>::gpInstance = nullptr;
