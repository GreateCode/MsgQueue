//------------------------------------------------------------------------------
// Explanation
//------------------------------------------------------------------------------

#ifndef _FENGHOU_TEST_H_
#define _FENGHOU_TEST_H_

#include <windows.h>
#include <cstdio>
#include <cassert>
#include <tchar.h>
#include <stdexcept>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <iostream>
#include <queue>
#include <chrono>
#include <atomic>

#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include "concurrent_queue.h"

using namespace std;
using namespace chrono;

////////////////////////////////////////////////////////////////////////////////

struct WorkTimeInfo {steady_clock::time_point begin, end;};

class MethodTimeInfo
	{
	public:
		MethodTimeInfo() {}
		MethodTimeInfo(size_t nProducerAmount, size_t nConsumerAmount)
			: vProducer(nProducerAmount), vConsumer(nConsumerAmount) {}

		vector<WorkTimeInfo> vProducer;
		vector<WorkTimeInfo> vConsumer;
		size_t nMsgAmount;
		int64_t nCheckSum;
	};

////////////////////////////////////////////////////////////////////////////////

void ShowTimeInfo(const MethodTimeInfo & info);

////////////////////////////////////////////////////////////////////////////////

MethodTimeInfo TestA(size_t nMsgAmount, size_t nProducerAmount, size_t nConsumerAmount)
	{
	assert(nProducerAmount > 0 && nConsumerAmount > 0 && nMsgAmount > nProducerAmount);

	queue<int> MsgQueue;
	mutex m;
	condition_variable cond_var;
	boost::interprocess::interprocess_semaphore ProducerStart = 0;
	size_t nMsgInQueue = 0;
	vector<thread> vProducer(nProducerAmount);
	vector<thread> vConsumer(nConsumerAmount);
	MethodTimeInfo info(nProducerAmount, nConsumerAmount);
	atomic<size_t> nMsgProcessed = 0;
	atomic<int64_t> nCheckSum = 0;

	auto Producer = [&](size_t nThread)
		{
		ProducerStart.wait();
		info.vProducer[nThread].begin = steady_clock::now();

		const size_t nThreadMsg = nMsgAmount / vProducer.size();
		const size_t nMsgBase = nThreadMsg * nThread;
		for (size_t i = 0; i < nThreadMsg; ++i)
			{
			;	// prepare message
			unique_lock<mutex> lock(m);
			MsgQueue.push(nMsgBase + i);
			++nMsgInQueue;
			cond_var.notify_one();
			}
		info.vProducer[nThread].end = steady_clock::now();
		};

	// 消息为负数表示终止消费者线程
	auto Consumer = [&](size_t nThread)
		{
		size_t nMsgGot = 0;

		while (true)
			{
			int msg;
				{
				unique_lock<mutex> lock(m);
				while (nMsgInQueue == 0) {cond_var.wait(lock);}
				msg = MsgQueue.front();
				MsgQueue.pop();
				--nMsgInQueue;
				}
			if (nMsgGot++ == 0) info.vConsumer[nThread].begin = steady_clock::now();
			if (msg < 0) break;
			nCheckSum += msg;	// process message
			++nMsgProcessed;
			}

		info.vConsumer[nThread].end = steady_clock::now();
		};

	for (size_t i = 0; i < vConsumer.size(); ++i) {vConsumer[i] = thread(Consumer, i);}
	for (size_t i = 0; i < vProducer.size(); ++i) {vProducer[i] = thread(Producer, i);}

	for (size_t i = 0; i < vProducer.size(); ++i) {ProducerStart.post();}

	for (auto & t : vProducer) {t.join();}

		{
		unique_lock<mutex> lock(m);
		for (size_t i = 0; i < vConsumer.size(); ++i) {MsgQueue.push(-1);}
		nMsgInQueue += vConsumer.size();
		cond_var.notify_all();
		}

	for (auto & t : vConsumer) {t.join();}

	info.nMsgAmount = nMsgProcessed;
	info.nCheckSum = nCheckSum;
	return std::move(info);
	}


MethodTimeInfo TestB(size_t nMsgAmount, size_t nProducerAmount, size_t nConsumerAmount)
	{
	assert(nProducerAmount > 0 && nConsumerAmount > 0 && nMsgAmount > nProducerAmount);

	queue<int> MsgQueue;
	boost::interprocess::interprocess_semaphore Mutex = 1, ProducerStart = 0, TaskAmount = 0;
	vector<thread> vProducer(nProducerAmount);
	vector<thread> vConsumer(nConsumerAmount);
	MethodTimeInfo info(nProducerAmount, nConsumerAmount);
	atomic<size_t> nMsgProcessed = 0;
	atomic<int64_t> nCheckSum = 0;

	auto Producer = [&](size_t nThread)
		{
		ProducerStart.wait();
		info.vProducer[nThread].begin = steady_clock::now();

		const size_t nThreadMsg = nMsgAmount / vProducer.size();
		const size_t nMsgBase = nThreadMsg * nThread;
		for (size_t i = 0; i < nThreadMsg; ++i)
			{
			;	// prepare message
			Mutex.wait();
			MsgQueue.push(nMsgBase + i);
			Mutex.post();
			TaskAmount.post();
			}
		info.vProducer[nThread].end = steady_clock::now();
		};

	// 消息为负数表示终止消费者线程
	auto Consumer = [&](size_t nThread)
		{
		size_t nMsgGot = 0;

		while (true)
			{
			TaskAmount.wait();
			Mutex.wait();
			int msg = MsgQueue.front();
			MsgQueue.pop();
			Mutex.post();

			if (nMsgGot++ == 0) info.vConsumer[nThread].begin = steady_clock::now();
			if (msg < 0) break;
			nCheckSum += msg;	// process message
			++nMsgProcessed;
			}

		info.vConsumer[nThread].end = steady_clock::now();
		};

	for (size_t i = 0; i < vConsumer.size(); ++i) {vConsumer[i] = thread(Consumer, i);}
	for (size_t i = 0; i < vProducer.size(); ++i) {vProducer[i] = thread(Producer, i);}

	for (size_t i = 0; i < vProducer.size(); ++i) {ProducerStart.post();}

	for (auto & t : vProducer) {t.join();}

	// 结束消费者线程
	Mutex.wait();
	for (size_t i = 0; i < vConsumer.size(); ++i) {MsgQueue.push(-1);}
	Mutex.post();
	for (size_t i = 0; i < vConsumer.size(); ++i) {TaskAmount.post();}

	for (auto & t : vConsumer) {t.join();}

	info.nMsgAmount = nMsgProcessed;
	info.nCheckSum = nCheckSum;
	return std::move(info);
	}


// *** 此无锁链表只能后进先出！ ***
MethodTimeInfo TestC(size_t nMsgAmount, size_t nProducerAmount, size_t nConsumerAmount)
	{
	assert(nProducerAmount > 0 && nConsumerAmount > 0 && nMsgAmount > nProducerAmount);

	struct LIST_ITEM
		{
		SLIST_ENTRY ItemEntry;
		int Message;
		};

	PSLIST_HEADER pListHead;
	boost::interprocess::interprocess_semaphore ProducerStart = 0;
	atomic<bool> bConsumerStop = false;
	vector<thread> vProducer(nProducerAmount);
	vector<thread> vConsumer(nConsumerAmount);
	MethodTimeInfo info(nProducerAmount, nConsumerAmount);
	atomic<size_t> nMsgProcessed = 0;
	atomic<int64_t> nCheckSum = 0;

	// 初始化无锁链表
	pListHead = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT);
	if (pListHead == nullptr) throw runtime_error("Memory allocation failed.");
	InitializeSListHead(pListHead);

	//
	auto Producer = [&](size_t nThread)
		{
		ProducerStart.wait();
		info.vProducer[nThread].begin = steady_clock::now();

		const size_t nThreadMsg = nMsgAmount / vProducer.size();
		const size_t nMsgBase = nThreadMsg * nThread;
		for (size_t i = 0; i < nThreadMsg; ++i)
			{
			;	// prepare message
			auto pListItem = (LIST_ITEM *)_aligned_malloc(sizeof(LIST_ITEM), MEMORY_ALLOCATION_ALIGNMENT);
			if (pListItem == nullptr) throw runtime_error("Memory allocation failed.");
			pListItem->Message = nMsgBase + i;
			InterlockedPushEntrySList(pListHead, &pListItem->ItemEntry);
			}
		info.vProducer[nThread].end = steady_clock::now();
		};

	// 取消息时若队列为空，则该消息为结束线程
	auto Consumer = [&](size_t nThread)
		{
		size_t nMsgGot = 0;

		while (true)
			{
			auto pListEntry = InterlockedPopEntrySList(pListHead);
			if (pListEntry == nullptr)
				{
				if (bConsumerStop) break;
				Sleep(1);
				continue;
				}
			auto pListItem = (LIST_ITEM *)pListEntry;
			int msg = pListItem->Message;
			_aligned_free(pListItem);

			if (nMsgGot++ == 0) info.vConsumer[nThread].begin = steady_clock::now();
			nCheckSum += msg;	// process message
			++nMsgProcessed;
			}

		info.vConsumer[nThread].end = steady_clock::now();
		};

	for (size_t i = 0; i < vConsumer.size(); ++i) {vConsumer[i] = thread(Consumer, i);}
	for (size_t i = 0; i < vProducer.size(); ++i) {vProducer[i] = thread(Producer, i);}

	for (size_t i = 0; i < vProducer.size(); ++i) {ProducerStart.post();}

	for (auto & t : vProducer) {t.join();}

	bConsumerStop = true;
	for (auto & t : vConsumer) {t.join();}

	// 反初始化无锁链表
	auto pListEntry = InterlockedFlushSList(pListHead);
	_aligned_free(pListHead);
	pListHead = nullptr;

	while (pListEntry != nullptr)
		{
		auto pNext = pListEntry->Next;
		_aligned_free(pListEntry);
		pListEntry = pNext;
		}

	//
	info.nMsgAmount = nMsgProcessed;
	info.nCheckSum = nCheckSum;
	return std::move(info);
	}


MethodTimeInfo TestD(size_t nMsgAmount, size_t nProducerAmount, size_t nConsumerAmount)
	{
	assert(nProducerAmount > 0 && nConsumerAmount > 0 && nMsgAmount > nProducerAmount);

	fenghou::concurrent::concurrent_queue<int, 1000> MsgQueue;

	boost::interprocess::interprocess_semaphore ProducerStart = 0;
	vector<thread> vProducer(nProducerAmount);
	vector<thread> vConsumer(nConsumerAmount);
	MethodTimeInfo info(nProducerAmount, nConsumerAmount);
	atomic<size_t> nMsgProcessed = 0;
	atomic<int64_t> nCheckSum = 0;

	auto Producer = [&](size_t nThread)
		{
		ProducerStart.wait();
		info.vProducer[nThread].begin = steady_clock::now();

		const size_t nThreadMsg = nMsgAmount / vProducer.size();
		const size_t nMsgBase = nThreadMsg * nThread;
		for (size_t i = 0; i < nThreadMsg; ++i)
			{
			;	// prepare message
			MsgQueue.push(nMsgBase + i);
			}
		info.vProducer[nThread].end = steady_clock::now();
		};

	// 消息为负数表示终止消费者线程
	auto Consumer = [&](size_t nThread)
		{
		size_t nMsgGot = 0;

		while (true)
			{
			int msg;
			if ( !MsgQueue.pop(&msg) )
				{
				Sleep(1);
				continue;
				}
			
			if (nMsgGot++ == 0) info.vConsumer[nThread].begin = steady_clock::now();
			if (msg < 0) break;
			nCheckSum += msg;	// process message
			++nMsgProcessed;
			}

		info.vConsumer[nThread].end = steady_clock::now();
		};

	for (size_t i = 0; i < vConsumer.size(); ++i) {vConsumer[i] = thread(Consumer, i);}
	for (size_t i = 0; i < vProducer.size(); ++i) {vProducer[i] = thread(Producer, i);}

	for (size_t i = 0; i < vProducer.size(); ++i) {ProducerStart.post();}

	for (auto & t : vProducer) {t.join();}

	// 结束消费者线程
	for (size_t i = 0; i < vConsumer.size(); ++i) {MsgQueue.push(-1);}

	for (auto & t : vConsumer) {t.join();}

	info.nMsgAmount = nMsgProcessed;
	info.nCheckSum = nCheckSum;
	return std::move(info);
	}


void ShowTimeInfo(const MethodTimeInfo & info)
	{
	steady_clock::duration DurProduceThreadMax, DurProduceTotal,
		DurConsumeThreadMax, DurConsumeTotal, DurTotal;
	WorkTimeInfo TimeProduce, TimeConsume;

	if ( !info.vProducer.empty() )
		{
		TimeProduce.begin = info.vProducer[0].begin;
		TimeProduce.end = info.vProducer[0].end;
		DurProduceThreadMax = info.vProducer[0].end - info.vProducer[0].begin;
		for (size_t i = 1; i < info.vProducer.size(); ++i)
			{
			if (info.vProducer[i].begin < TimeProduce.begin) TimeProduce.begin = info.vProducer[i].begin;
			if (info.vProducer[i].end > TimeProduce.end) TimeProduce.end = info.vProducer[i].end;
			steady_clock::duration DurProduceThread = info.vProducer[i].end - info.vProducer[i].begin;
			if (DurProduceThread > DurProduceThreadMax) DurProduceThreadMax = DurProduceThread;
			}
		DurProduceTotal = TimeProduce.end - TimeProduce.begin;
		}

	if ( !info.vConsumer.empty() )
		{
		TimeConsume.begin = info.vConsumer[0].begin;
		TimeConsume.end = info.vConsumer[0].end;
		DurConsumeThreadMax = info.vConsumer[0].end - info.vConsumer[0].begin;
		for (size_t i = 1; i < info.vConsumer.size(); ++i)
			{
			if (info.vConsumer[i].begin < TimeConsume.begin) TimeConsume.begin = info.vConsumer[i].begin;
			if (info.vConsumer[i].end > TimeConsume.end) TimeConsume.end = info.vConsumer[i].end;
			steady_clock::duration DurConsumeThread = info.vConsumer[i].end - info.vConsumer[i].begin;
			if (DurConsumeThread > DurConsumeThreadMax) DurConsumeThreadMax = DurConsumeThread;
			}
		DurConsumeTotal = TimeConsume.end - TimeConsume.begin;
		}

	DurTotal = max(TimeProduce.end, TimeConsume.end) - min(TimeProduce.begin, TimeConsume.begin);

	//
	tcout << _T(" <") << info.nMsgAmount << _T("> <") << info.nCheckSum << _T(">") << endl << endl;

	tcout << _T("各生产线程所耗时间(ms): ") << endl;
	for (size_t i = 0; i < info.vProducer.size(); ++i)
		{
		auto dur = info.vProducer[i].end - info.vProducer[i].begin;
		tcout << duration_cast<milliseconds>(dur).count();
		if (i % 8 != 7 && i != info.vProducer.size() - 1) tcout << _T("\t");
		else tcout << endl;
		}
	tcout << endl;

	tcout << _T("各消费线程所耗时间(ms): ") << endl;
	for (size_t i = 0; i < info.vConsumer.size(); ++i)
		{
		auto dur = info.vConsumer[i].end - info.vConsumer[i].begin;
		tcout << duration_cast<milliseconds>(dur).count();
		if (i % 8 != 7 && i != info.vConsumer.size() - 1) tcout << _T("\t");
		else tcout << endl;
		}
	tcout << endl;

	tcout << _T("单线程最大生产时间(ms):\t") << duration_cast<milliseconds>(DurProduceThreadMax).count() << _T("\t") <<
		_T("总生产时间(ms):\t") << duration_cast<milliseconds>(DurProduceTotal).count() << endl;
	tcout << _T("单线程最大消费时间(ms):\t") << duration_cast<milliseconds>(DurConsumeThreadMax).count() << _T("\t") <<
		_T("总消费时间(ms):\t") << duration_cast<milliseconds>(DurConsumeTotal).count() << endl;
	tcout << _T("\t\t\t\t总时间(ms):\t") << duration_cast<milliseconds>(DurTotal).count() << endl;

	tcout << endl << endl;
	}

#endif // _FENGHOU_TEST_H_