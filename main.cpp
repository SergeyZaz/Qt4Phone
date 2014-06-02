#include "qt4phonedlg.h"
#include <QtGui/QApplication>
#include <QTextCodec>
#include <QMessageBox>
#include <QObject>

/////////////////////////////////////////////////////////////////////////////////////////////////
CheckOneApp::CheckOneApp()
{
}

CheckOneApp::~CheckOneApp()
{
	fWork = false;
	delete shmem;
	sem->release();
	wait(500);
}

bool CheckOneApp::isRun()
{
	sem = new QSystemSemaphore("Qt4PhoneSem", 1);

	bool isRunning = false;

	shmem = new QSharedMemory("Qt4PhoneShM");
	shmem->attach();
	shmem->detach();

	if (shmem->attach())
	{
		QMessageBox::critical(0, QObject::tr("Завершение работы"),
                              QObject::tr("Невозможно загрузить приложение.\nПриложение уже запущено!"));
		sem->release();
		isRunning = true;
	}
	else
	{
		shmem->create(1);
		isRunning = false;
		
		start();
	}
	return isRunning;
}
	
void CheckOneApp::run()
{
	sem->acquire();

	fWork = true;
	while(fWork)
	{
		if(sem->acquire())
			emit signalShow();
		msleep(100);
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////

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


	QtPhoneDlg *w = new QtPhoneDlg;
	
	/////////////////////////////////////////////////////////////////////////////////////////
	CheckOneApp checkOneApp;
	if(checkOneApp.isRun())
	{
		delete w;
		return;
	}
	int rc = QObject::connect(&checkOneApp, SIGNAL(signalShow()), w, SLOT(showSlot())); 
	/////////////////////////////////////////////////////////////////////////////////////////

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


	rc =  a.exec();
	delete w;
}
