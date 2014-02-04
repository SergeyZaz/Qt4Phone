#include "qt4phonedlg.h"
#include <QtGui/QApplication>
#include <QTextCodec>
#include <QMessageBox>
#include <QObject>
#include <QSystemSemaphore>

QSystemSemaphore *sem=NULL;


#ifdef WIN32
	#define PROGRAMM_NAME L"Qt4Phone"
#else
	#define PROGRAMM_NAME "Qt4Phone"
#endif
/*
HANDLE OneCopyApplicationWork = NULL;
bool CheckOneCopyApplicationWork()
{
	OneCopyApplicationWork = OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, PROGRAMM_NAME);
	if (OneCopyApplicationWork != NULL)
	{
		CloseHandle(OneCopyApplicationWork);
		OneCopyApplicationWork=NULL;
		return false;
	}
	OneCopyApplicationWork = CreateSemaphore(NULL, false, 1, PROGRAMM_NAME);
	return true;
} 
*/
void WhileThread::run()
{
	while(isRunning())
	{
		if(sem)
		{
			sem->acquire();
			emit signalShow();
		}
	}
}

class Qt4Phone : public PProcess
{
  PCLASSINFO(Qt4Phone, PProcess)

  public:
    Qt4Phone();
    ~Qt4Phone();

    virtual void Main();
};

PCREATE_PROCESS(Qt4Phone);

Qt4Phone::Qt4Phone()
  : PProcess("Qt4Phone", "Qt4Phone", 1, 0, AlphaCode, 0)
//  : PProcess("MyPhone3", "MyPhone3", 1, 0, AlphaCode, 0)
{
}

Qt4Phone::~Qt4Phone()
{
}


void Qt4Phone::Main()
{
	PArgList &args = GetArguments();
	int argCount = args.GetCount();
	const char **argV = (const char**)calloc(argCount+1, sizeof(const char*));
	bool fCreateConsole = false;
	for (int i = 0; i < argCount; i++)
	{
		argV[i] = (const char*)args.GetParameter(i);
		if(strcmp(argV[i], "-c")==0)
			fCreateConsole = true;
	}
	argV[argCount] = NULL;

	QTextCodec::setCodecForTr(QTextCodec::codecForName("windows-1251"));
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("windows-1251"));  

#ifdef _DEBUG
	fCreateConsole = true;
#endif

#ifdef _WIN32
	if(fCreateConsole)
	{
	#ifdef WIN32
		WCHAR title[256];
	#else
		char title[256];
	#endif
		if(GetConsoleTitle(title,sizeof(title))==0) 
		{
			AllocConsole();
			freopen("CONOUT$","wt", stdout);
			freopen("CONOUT$","wt", stderr);
			freopen("CONIN$","rt", stdin);
			GetConsoleTitle(title,sizeof(title));
			HWND hConsoleWindow = FindWindow(NULL,title);
			ShowWindow(hConsoleWindow,SW_SHOWMINNOACTIVE);
		}
	}
#endif

	QApplication a(argCount, (char**)argV);
/*
	sem = new QSystemSemaphore("Qt4PhoneWhile", 0, QSystemSemaphore::Open);
	if(!CheckOneCopyApplicationWork())
	{
		sem->release();
		//QMessageBox::critical(0, QObject::tr("Завершение работы"),
        //                      QObject::tr("Невозможно загрузить приложение.\nПриложение уже запущено!"));
        return;
	} 
*/
	QtPhoneDlg *w = new QtPhoneDlg;

	if (QSystemTrayIcon::isSystemTrayAvailable())
	{
		puts("SystemTray - Available");
 		QApplication::setQuitOnLastWindowClosed(false);
    }
	else
	{
		puts("SystemTray - NOT Available");
		a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
		w->show();
	}

	WhileThread *whileThread = new WhileThread;
	QObject::connect(whileThread, SIGNAL(signalShow()), w, SLOT(showSlot())); 
	whileThread->start();

	int rc =  a.exec();
	delete w;
}
