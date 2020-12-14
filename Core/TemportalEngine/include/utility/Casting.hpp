#pragma once

template <typename T>
inline T* as(void* obfuscated) { return reinterpret_cast<T*>(obfuscated); }

template <typename T, typename TWrapped>
inline T* extract(TWrapped *ptr) { return reinterpret_cast<T*>(ptr->get()); }

template <typename T, typename TWrapped>
inline T* extract_unsafe(TWrapped const* ptr) { return reinterpret_cast<T*>(ptr->get()); }
