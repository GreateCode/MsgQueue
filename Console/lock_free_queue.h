//------------------------------------------------------------------------------
// 支持并发操作的无锁先进先出队列
//------------------------------------------------------------------------------

#ifndef _FENGHOU_LOCK_FREE_QUEUE_H_
#define _FENGHOU_LOCK_FREE_QUEUE_H_

#include <windows.h>
#include <cstdio>
#include <cassert>
#include <stdexcept>

// User Interface //////////////////////////////////////////////////////////////

namespace fenghou { namespace concurrent
{
// Classes /////////////////////////////////////////////////////////////////////

template<typename _Ty>
class lock_free_queue
	{
	public:
		;

	private:
		;
	};

// Functions ///////////////////////////////////////////////////////////////////
// Global Variables ////////////////////////////////////////////////////////////
}}

#endif // _FENGHOU_LOCK_FREE_QUEUE_H_