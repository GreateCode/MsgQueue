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
// ��ʼ������
//------------------------------------------------------------------------------
bool Init()
	{
	atexit(Uninit);					// �˳�ǰ����β����
	tcout.imbue(locale("chs"));		// Ϊiostream���ñ�����Ϣ
	tcerr.imbue(locale("chs"));

	return true;
	}


//------------------------------------------------------------------------------
// �˳�ǰ����β����
// �봴��˳���෴����Щȷʵ���ܰ����෴��˳�������������ģ�
//------------------------------------------------------------------------------
void Uninit()
	{
	system("pause");
	}


//------------------------------------------------------------------------------
// ������������������Ҫ�����м��ϲ���
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

	tcout << _T("��Ϣ��: ") << nMsgAmount << _T("\t") <<
		_T("�������߳���: ") << nProducerAmount << _T("\t") <<
		_T("�������߳���: ") << nConsumerAmount << endl << endl;

	tcout << _T("��ʼ���Է���A (mutex + condition_variable) ...");
	info = TestA(nMsgAmount, nProducerAmount, nConsumerAmount);
	tcout << _T("��ɡ�");
	ShowTimeInfo(info);

	tcout << _T("��ʼ���Է���B (semaphore) ...");
	info = TestB(nMsgAmount, nProducerAmount, nConsumerAmount);
	tcout << _T("��ɡ�");
	ShowTimeInfo(info);

	tcout << _T("��ʼ���Է���C (lock-free list) ...");
	info = TestC(nMsgAmount, nProducerAmount, nConsumerAmount);
	tcout << _T("��ɡ�");
	ShowTimeInfo(info);

	tcout << _T("��ʼ���Է���D (half-lock queue) ...");
	info = TestD(nMsgAmount, nProducerAmount, nConsumerAmount);
	tcout << _T("��ɡ�");
	ShowTimeInfo(info);

	tcout << endl;
	}
