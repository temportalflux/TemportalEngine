#include "crypto/AES.hpp"

using namespace crypto;

std::shared_ptr<AES> Singleton<AES>::gpInstance = nullptr;
