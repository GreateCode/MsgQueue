//------------------------------------------------------------------------------
// 支持并发操作的先进先出队列
// push和pop可并行执行，但push和pop自身是串行执行的
//------------------------------------------------------------------------------

#ifndef _FENGHOU_CONCURRENT_QUEUE_H_
#define _FENGHOU_CONCURRENT_QUEUE_H_

#include <windows.h>
#include <cstdio>
#include <cassert>
#include <stdexcept>

// User Interface //////////////////////////////////////////////////////////////

namespace fenghou { namespace concurrent
{
// Classes /////////////////////////////////////////////////////////////////////

// @param	_Ty			元素类型
// @param	_BlockLen	每个队列块包含的元素个数
template<typename _Ty, size_t _BlockLen> class concurrent_queue
	{
	public:
		typedef concurrent_queue<_Ty, _BlockLen> _Myt;
		typedef _Ty value_type;
		typedef size_t size_type;

		concurrent_queue()
			: m_nHead(0), m_nTail(0), m_nLength(0), m_HeadLock(1), m_TailLock(1)
			{
			// 初始化内存池（无锁链表）
			m_pListHead = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT);
			if (m_pListHead == nullptr) throw runtime_error("Memory allocation failed.");
			InitializeSListHead(m_pListHead);

			// 默认分配两块内存
			m_pHead = __NewBlock();
			m_pHead->ItemEntry.Next = (SLIST_ENTRY *)__NewBlock();
			m_pTail = m_pHead;
			}

		~concurrent_queue()
			{
			while (m_pHead)
				{
				BLOCK * pNext = (BLOCK *)m_pHead->ItemEntry.Next;
				_aligned_free(m_pHead);
				m_pHead = pNext;
				}

			// 反初始化内存池（无锁链表）
			auto pListEntry = InterlockedFlushSList(m_pListHead);
			_aligned_free(m_pListHead);
			m_pListHead = nullptr;

			while (pListEntry != nullptr)
				{
				auto pNext = pListEntry->Next;
				_aligned_free(pListEntry);
				pListEntry = pNext;
				}
			}

		bool empty() const
			{
			return m_nLength == 0;
			}

		size_type size() const
			{
			return m_nLength;
			}

		void push(const value_type & _Val)
			{
			m_TailLock.wait();
			if (m_nTail == _BlockLen)
				{
				if (!m_pTail->ItemEntry.Next) m_pTail->ItemEntry.Next = (SLIST_ENTRY *)__NewBlock();
				m_pTail = (BLOCK *)m_pTail->ItemEntry.Next;
				m_nTail = 0;
				}
			m_pTail->gVal[m_nTail++] = _Val;
			++m_nLength;
			m_TailLock.post();
			}
		
		void push(value_type && _Val)
			{
			m_TailLock.wait();
			if (m_nTail == _BlockLen)
				{
				if (!m_pTail->ItemEntry.Next) m_pTail->ItemEntry.Next = (SLIST_ENTRY *)__NewBlock();
				m_pTail = (BLOCK *)m_pTail->ItemEntry.Next;
				m_nTail = 0;
				}
			m_pTail->gVal[m_nTail++] = std::move(_Val);
			++m_nLength;
			m_TailLock.post();
			}

		bool pop(value_type * _pVal)
			{
			m_HeadLock.wait();

			// 头部锁定期间，队列长度不会减少，因此可以在push的同时pop
			if (m_nLength == 0) {m_HeadLock.post(); return false;}

			*_pVal = m_pHead->gVal[m_nHead];

			if (++m_nHead == _BlockLen) {m_pHead = __DeleteBlock(m_pHead); m_nHead = 0;}

			--m_nLength;
			m_HeadLock.post();
			return true;
			}

	private:
		concurrent_queue(const _Myt & _Right) {}
		concurrent_queue(_Myt && _Right) {}
		_Myt & operator=(const _Myt & _Right) {}
		_Myt & operator=(_Myt && _Right) {}

		struct BLOCK
			{
			SLIST_ENTRY ItemEntry;
			_Ty gVal[_BlockLen];
			};

		// 返回传入块的下一块地址
		// 只会在pop时调用，而pop时头部已加锁，故本函数本身不会并发执行，但内存池会被pop和push同时使用
		BLOCK * __DeleteBlock(BLOCK * pBlock)
			{
			BLOCK * pNext = (BLOCK *)pBlock->ItemEntry.Next;
			InterlockedPushEntrySList(m_pListHead, &pBlock->ItemEntry);
			return pNext;
			}

		// 返回的块的下一块指针指向空地址
		// 只会在push时调用，而push时尾部已加锁，故本函数本身不会并发执行，但内存池会被pop和push同时使用
		BLOCK * __NewBlock()
			{
			BLOCK * p;

			auto pListEntry = InterlockedPopEntrySList(m_pListHead);
			if (pListEntry == nullptr)
				{
				p = (BLOCK *)_aligned_malloc(sizeof(BLOCK), MEMORY_ALLOCATION_ALIGNMENT);
				if (p == nullptr) throw runtime_error("Memory allocation failed.");
				}
			else
				p = (BLOCK *)pListEntry;

			p->ItemEntry.Next = nullptr;
			return p;
			}

		// 头部指向第一个元素所在位置，尾部指向最后一个元素后一个位置
		BLOCK * m_pHead, * m_pTail;
		size_type m_nHead, m_nTail;
		std::atomic<size_type> m_nLength;
		// 如果需要同时锁头尾，必须保证先锁头再锁尾，防止死锁
		boost::interprocess::interprocess_semaphore m_HeadLock, m_TailLock;
		PSLIST_HEADER m_pListHead;
	};

// Functions ///////////////////////////////////////////////////////////////////
// Global Variables ////////////////////////////////////////////////////////////
}}

#endif // _FENGHOU_CONCURRENT_QUEUE_H_