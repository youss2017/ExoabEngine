#pragma once
#include <cstdint>
#include <cassert>
#include <memory>

namespace egx {

	// template<typename T>
	// using ref = std::shared_ptr<T>;

#if 1
	template<typename T>
	class ref {
	public:
		inline ref<T>(T* base) noexcept {
			this->base = base;
			refCount = new int32_t;
			*refCount = 1;
		}

		inline ref<T>(T* base, bool SelfReference) noexcept {
			this->base = (T*)base;
			if (!SelfReference) {
				refCount = new int32_t;
				*refCount = 1;
			}
			else
				refCount = nullptr;
		}

		inline ref<T>() noexcept {
			this->base = nullptr;
			refCount = nullptr;
		}

		inline ref<T>(const ref& cp) noexcept {
			this->base = cp.base;
			this->refCount = cp.refCount;
			*this->refCount += 1;
		}

		inline ref<T>(ref&& move) noexcept :
			base(std::exchange(move.base, nullptr)),
			refCount(std::exchange(move.refCount, nullptr))
		{}

		inline ref<T>& operator=(const ref& cp) noexcept {
			if (cp.base == nullptr || &cp == this) {
				return *this;
			}
			this->base = cp.base;
			this->refCount = cp.refCount;
			*this->refCount += 1;
			return *this;
		}

		inline ref<T>& operator=(ref&& move) noexcept {
			if (this == &move) return *this;

			if (refCount) {
				*refCount -= 1;
				if (*refCount == 0) {
					delete (T*)base;
					delete refCount;
				}
			}

			base = std::exchange(move.base, nullptr);
			refCount = std::exchange(move.refCount, nullptr);

			return *this;
		}

		constexpr T* operator->() const noexcept {
			return base;
		}

		constexpr T& operator*() const noexcept {
			return *base;
		}

		constexpr  T* operator()() const noexcept {
			return base;
		}

		inline ~ref<T>() {
			if (refCount == nullptr) return;
			*refCount -= 1;
			// ref<T> is not thread safe
			// because refCount could drop below zero and deconstructor while not be called.
			// if we allow refCount < 0 to call deconstructor then multiple threads will call
			// the deconstructor
			assert(*refCount >= 0 && "Reference count is below 0. Are you using threading. ref<T> is not thread safe.");
			if (*refCount == 0) {
				delete (T*)base;
				delete refCount;
			}
		}

		inline bool IsValidRef() const noexcept { return (base != nullptr) && (*refCount > 0); }

		inline int32_t RefCount() const noexcept { return *refCount; }
		inline int32_t AddRef() noexcept { *refCount += 1; return *refCount; }

		inline bool DebugDeconstruction() {
			bool success = *refCount == 1;
			delete (T*)base;
			delete refCount;
			base = nullptr;
			refCount = nullptr;
			return success;
		}

	public:
		T* base;
	protected:
		int32_t* refCount;
	};
#endif

}
