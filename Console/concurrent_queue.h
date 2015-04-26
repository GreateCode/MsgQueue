//------------------------------------------------------------------------------
// ֧�ֲ����������Ƚ��ȳ�����
// push��pop�ɲ���ִ�У���push��pop�����Ǵ���ִ�е�
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

// @param	_Ty			Ԫ������
// @param	_BlockLen	ÿ�����п������Ԫ�ظ���
template<typename _Ty, size_t _BlockLen> class concurrent_queue
	{
	public:
		typedef concurrent_queue<_Ty, _BlockLen> _Myt;
		typedef _Ty value_type;
		typedef size_t size_type;

		concurrent_queue()
			: m_nHead(0), m_nTail(0), m_nLength(0), m_HeadLock(1), m_TailLock(1)
			{
			// ��ʼ���ڴ�أ���������
			m_pListHead = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT);
			if (m_pListHead == nullptr) throw runtime_error("Memory allocation failed.");
			InitializeSListHead(m_pListHead);

			// Ĭ�Ϸ��������ڴ�
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

			// ����ʼ���ڴ�أ���������
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

			// ͷ�������ڼ䣬���г��Ȳ�����٣���˿�����push��ͬʱpop
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

		// ���ش�������һ���ַ
		// ֻ����popʱ���ã���popʱͷ���Ѽ������ʱ����������Ტ��ִ�У����ڴ�ػᱻpop��pushͬʱʹ��
		BLOCK * __DeleteBlock(BLOCK * pBlock)
			{
			BLOCK * pNext = (BLOCK *)pBlock->ItemEntry.Next;
			InterlockedPushEntrySList(m_pListHead, &pBlock->ItemEntry);
			return pNext;
			}

		// ���صĿ����һ��ָ��ָ��յ�ַ
		// ֻ����pushʱ���ã���pushʱβ���Ѽ������ʱ����������Ტ��ִ�У����ڴ�ػᱻpop��pushͬʱʹ��
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

		// ͷ��ָ���һ��Ԫ������λ�ã�β��ָ�����һ��Ԫ�غ�һ��λ��
		BLOCK * m_pHead, * m_pTail;
		size_type m_nHead, m_nTail;
		std::atomic<size_type> m_nLength;
		// �����Ҫͬʱ��ͷβ�����뱣֤����ͷ����β����ֹ����
		boost::interprocess::interprocess_semaphore m_HeadLock, m_TailLock;
		PSLIST_HEADER m_pListHead;
	};

// Functions ///////////////////////////////////////////////////////////////////
// Global Variables ////////////////////////////////////////////////////////////
}}

#endif // _FENGHOU_CONCURRENT_QUEUE_H_