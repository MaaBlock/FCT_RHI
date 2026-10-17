#pragma once
#include <atomic>
#include <FCT_Base/RefCount.h>
#include "../DebugTools/MemoryCheak.h"

namespace FCT {
	class RefCount {
	public:
		RefCount() : m_refCount(1) {}
		RefCount(size_t refCount) : m_refCount(refCount) {}
		virtual ~RefCount() {};
		void addRef() {
			++m_refCount;
		}
		void release() {
			if (m_refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
				deleteThis();
			}
		}
		size_t count() const {
			return m_refCount.load(std::memory_order_relaxed);
		}
		operator bool() const
		{
			return m_refCount > 0;
		}
	protected:
		virtual void deleteThis() {
			FCT_DELETE(this);
		}
		std::atomic<size_t> m_refCount;
	};

	inline void safeAddRef(RefCount* obj) {
		if (obj) obj->addRef();
	}
	inline void safeRelease(RefCount*& obj) {
		if (obj) obj->release();
		obj = nullptr;
	}
}