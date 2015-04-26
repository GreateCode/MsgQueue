// INCLUDES ////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <cstdio>
#include <tchar.h>
#include <stdexcept>
#include <string>
#include <iostream>

#include "Common/Common.h"

#include "Test.h"

using namespace std;

// DEFINES /////////////////////////////////////////////////////////////////////
// MACROS //////////////////////////////////////////////////////////////////////
// TYPES ///////////////////////////////////////////////////////////////////////
// CLASS ///////////////////////////////////////////////////////////////////////
// PROTOTYPES //////////////////////////////////////////////////////////////////

bool Init();
void Uninit();
void OnWork();
void Test(size_t nMsgAmount, size_t nProducerAmount, size_t nConsumerAmount);

// EXTERNALS ///////////////////////////////////////////////////////////////////
// GLOBALS /////////////////////////////////////////////////////////////////////
// FUNCTIONS ///////////////////////////////////////////////////////////////////

int _tmain(int argc, TCHAR * argv[], TCHAR * envp[])
	{
	try {
		if (!Init()) throw runtime_error("Failed to initialize application.");
		OnWork();
		}
	catch (logic_error & e) {cout << e.what() << endl;}
	catch (runtime_error & e) {cout << e.what() << endl;}
	catch (...) {cout << "Other exception.\n";}

	return 0;
	}


//------------------------------------------------------------------------------
// 初始化工作
//------------------------------------------------------------------------------
bool Init()
	{
	atexit(Uninit);					// 退出前的收尾工作
	tcout.imbue(locale("chs"));		// 为iostream设置本地信息
	tcerr.imbue(locale("chs"));

	return true;
	}


//------------------------------------------------------------------------------
// 退出前的收尾工作
// 与创建顺序相反（有些确实不能按照相反的顺序来，必须留心）
//------------------------------------------------------------------------------
void Uninit()
	{
	system("pause");
	}


//------------------------------------------------------------------------------
// 工作主函数，如有需要请自行加上参数
//------------------------------------------------------------------------------
void OnWork()
	{
	size_t nMsgAmount = 1024 * 1024;

	//Test(nMsgAmount, 1, 1);
	//Test(nMsgAmount, 2, 1);
	//Test(nMsgAmount, 4, 1);
	//Test(nMsgAmount, 8, 1);
	//Test(nMsgAmount, 16, 1);
	//Test(nMsgAmount, 32, 1);
	//Test(nMsgAmount, 64, 1);

	//Test(nMsgAmount, 1, 2);
	//Test(nMsgAmount, 2, 2);
	//Test(nMsgAmount, 4, 2);
	//Test(nMsgAmount, 8, 2);
	//Test(nMsgAmount, 16, 2);
	//Test(nMsgAmount, 32, 2);
	//Test(nMsgAmount, 64, 2);

	//Test(nMsgAmount, 1, 4);
	//Test(nMsgAmount, 2, 4);
	//Test(nMsgAmount, 4, 4);
	//Test(nMsgAmount, 8, 4);
	Test(nMsgAmount, 16, 4);
	//Test(nMsgAmount, 32, 4);
	//Test(nMsgAmount, 64, 4);
	}


void Test(size_t nMsgAmount, size_t nProducerAmount, size_t nConsumerAmount)
	{
	MethodTimeInfo info;

	tcout << _T("消息数: ") << nMsgAmount << _T("\t") <<
		_T("生产者线程数: ") << nProducerAmount << _T("\t") <<
		_T("消费者线程数: ") << nConsumerAmount << endl << endl;

	tcout << _T("开始测试方法A (mutex + condition_variable) ...");
	info = TestA(nMsgAmount, nProducerAmount, nConsumerAmount);
	tcout << _T("完成。");
	ShowTimeInfo(info);

	tcout << _T("开始测试方法B (semaphore) ...");
	info = TestB(nMsgAmount, nProducerAmount, nConsumerAmount);
	tcout << _T("完成。");
	ShowTimeInfo(info);

	tcout << _T("开始测试方法C (lock-free list) ...");
	info = TestC(nMsgAmount, nProducerAmount, nConsumerAmount);
	tcout << _T("完成。");
	ShowTimeInfo(info);

	tcout << _T("开始测试方法D (half-lock queue) ...");
	info = TestD(nMsgAmount, nProducerAmount, nConsumerAmount);
	tcout << _T("完成。");
	ShowTimeInfo(info);

	tcout << endl;
	}
