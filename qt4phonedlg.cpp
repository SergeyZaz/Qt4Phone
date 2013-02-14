#include <QTime>
#include <QMessageBox>
#include "qt4phonedlg.h"
#include "cvideodlg.h"
#include "csettingsdlg.h"
#include "global.h"
#include "caddrbook.h"

//#ifdef _WIN32
//	#include "mixer.h"
//#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

QtPhoneDlg::QtPhoneDlg(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	trayIcon = NULL;
	trayIconMenu = NULL;
	iconComboBox = NULL;

	if (QSystemTrayIcon::isSystemTrayAvailable())
	{
		trayIconMenu = new QMenu(this);
		trayIconMenu->addAction(ui.actRestore);
		trayIconMenu->addAction(ui.actCall);
		trayIconMenu->addAction(ui.actRefuse);
		trayIconMenu->addAction(ui.actShowVideo);
		trayIconMenu->addAction(ui.actExit);
		trayIcon = new QSystemTrayIcon(this);
		trayIcon->setContextMenu(trayIconMenu);
		connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			this, SLOT(slot_IconActivated(QSystemTrayIcon::ActivationReason)));
		//connect(trayIcon, SIGNAL(messageClicked()), this, SLOT(slot_OnCall()));

		connect(ui.actRestore, SIGNAL(triggered()), this, SLOT(show()));

		iconComboBox = new QComboBox;
		iconComboBox->addItem(QIcon(":/images/Resources/qcam_p1.png"), tr("Нет соединения"));
		iconComboBox->addItem(QIcon(":/images/Resources/teleph2.png"), tr("Идет разговор"));
		iconComboBox->addItem(QIcon(":/images/Resources/bell.png"), tr("Входящий звонок"));
		iconComboBox->setCurrentIndex(STATE_WORK);
		connect(iconComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slot_SetIcon(int))); 
		connect(this, SIGNAL(signal_SetIcon(int)), this, SLOT(slot_SetIcon(int))); 

		iconComboBox->setCurrentIndex(STATE_READY); 
		trayIcon->show(); 
	}

	connect(ui.actAbout, SIGNAL(triggered()), this, SLOT(about()));
	connect(ui.actExit, SIGNAL(triggered()), this, SLOT(slot_Close()));
	connect(ui.cmdCall, SIGNAL(clicked()), this, SLOT(slot_OnCall()));
	connect(ui.actCall, SIGNAL(triggered()), this, SLOT(slot_OnCall()));
	connect(ui.cmdRefuse, SIGNAL(clicked()), this, SLOT(slot_OnRefuse()));
	connect(ui.actRefuse, SIGNAL(triggered()), this, SLOT(slot_OnRefuse()));
	connect(ui.cmdSendMsg, SIGNAL(clicked()), this, SLOT(slot_SendMsg()));
	connect(ui.actSetup, SIGNAL(triggered()), this, SLOT(slot_OnSettings()));
	connect(ui.cmdContactsList, SIGNAL(clicked()), this, SLOT(openContacts()));
	connect(this, SIGNAL(signal_SetLevelVolume(unsigned int)), this, SLOT(slot_SetLevelVolume(unsigned int)));
	connect(this, SIGNAL(signal_SetLevelMic(unsigned int)), this, SLOT(slot_SetLevelMic(unsigned int)));

	ui.prBarSound->setValue(0);
	ui.prBarMic->setValue(0);
	connect(ui.cmdMuteSpeaker, SIGNAL(clicked(bool)), this, SLOT(slot_MuteSpeakerCmd(bool)));
	connect(ui.cmdMuteMicrophone, SIGNAL(clicked(bool)), this, SLOT(slot_MuteMicrophoneCmd(bool)));
	 
	m_pLblStat	= new QLabel();
	ui.statusbar->addPermanentWidget(m_pLblStat);
	
	//ui.txtAddress->setText("192.168.111.203");
	//ui.txtAddress->setText("192.168.1.103");

	m_endpoint = new CMyPhoneEndPoint;
	PConfig& config = m_endpoint->config; 

#if PTRACING	// If Compiled with PTlib Tracing support (like in OpenH323)
    myTraceFile = NULL;
    TraceLevel = -1;

	QStringList arg = QCoreApplication::arguments();
	for (int ii = 0; ii < arg.size(); ii++) 
	{  	
		if(arg[ii].startsWith("-d:"))
			TraceLevel = arg[ii].mid(3).toInt();
	}

	if(TraceLevel>=0)
		Tracing = OpenTraceFile(config);
#endif

	ui.actCall->setEnabled(false);
	ui.actRefuse->setEnabled(false);
	ui.cmdRefuse->setEnabled(false);
	ui.pnlMsg->setEnabled(false);
	ui.actShowVideo->setEnabled(false);
	ui.txtSendMsg->setPlainText("Ваши текстовые сообщения...");
	
	connect(ui.actShowVideo, SIGNAL(triggered(bool)), this, SLOT(slot_ShowVideoPanels(bool)));
	
	//////////////////////////// PConfig ////////////////////////////
	//curLang = GetLangFromTable(config.GetInteger(UILangugeConfigKey, -1));
	hideStat = config.GetBoolean(HideStatConfigKey, FALSE); // Can show statistic info
	indTimer.SetNotifier(PCREATE_NOTIFIER(OnUpdateIndicators));

	//config.SetInteger(VideoSourceConfigKey, -1);

	ringSoundFile = config.GetString(RingSoundFileConfigKey, "call.wav");
	ringSoundTimer.SetNotifier(PCREATE_NOTIFIER(OnRingSoundAgain));
	
	autohideVideoPan = config.GetBoolean(AutoVideoHideConfigKey, TRUE);
	showVideoPan = TRUE;
	if(autohideVideoPan)
		slot_ShowVideoPanels(false);  // hide Video panel in the begining
	hidePnP = FALSE;
	m_endpoint->localVideo = config.GetBoolean(VideoLocalConfigKey, TRUE);
	//hideSysMsg = config.GetBoolean(SysLogMsgHideConfigKey, FALSE);
	autoAddInAddr = config.GetBoolean(AutoAddCallersConfigKey, TRUE);



	m_endpoint->autoStartTransmitVideo = config.GetBoolean(AutoTransmitVideoConfigKey, TRUE);
	m_endpoint->autoStartReceiveVideo = config.GetBoolean(AutoReceiveVideoConfigKey, TRUE);
	m_endpoint->localVideo = config.GetBoolean(VideoLocalConfigKey, TRUE);
	bool m_LVflip = config.GetBoolean(VideoOutVFlipConfigKey, FALSE);
	bool m_RVflip = config.GetBoolean(VideoInVFlipConfigKey, FALSE);
	bool m_localFlip = config.GetBoolean(VideoFlipLocalConfigKey, FALSE);	
	int m_videoQTY = config.GetInteger(VideoQualityConfigKey, 15);
	int m_videoFPS = config.GetInteger(VideoFPSKey, 10);
	int m_videoInMaxBPS = config.GetInteger(VideoInMaxbandWidthKey, 320);
    int m_videoOutMaxBPS = config.GetInteger(VideoOutMaxbandWidthKey, 320);	
	int m_RecDevSrc = config.GetInteger(VideoSourceConfigKey, -1);
	int m_videoInSize = config.GetInteger(VideoInSizeConfigKey, 2);
	int m_videoOutSize = config.GetInteger(VideoOutSizeConfigKey, 2);
	
	m_endpoint->localFlip = m_localFlip;
	
	//config.SetInteger(VideoQualityConfigKey, m_videoQTY);
	//config.SetInteger(VideoFPSKey, m_videoFPS);
	//config.SetInteger(VideoInMaxbandWidthKey, m_videoInMaxBPS);
	//config.SetInteger(VideoOutMaxbandWidthKey, m_videoOutMaxBPS);
	//////////////////////////// PConfig ////////////////////////////

	connect(m_endpoint, SIGNAL(signal_RecvStats(const QString &)), this, SLOT(slot_RecvStats(const QString &)));
	connect(m_endpoint, SIGNAL(signal_OutputMsg(const QString &)), this, SLOT(slot_OutputStatus(const QString &)));
	connect(m_endpoint, SIGNAL(signal_OutputUsrMsg(const QString &)), this, SLOT(slot_OutputUserMsg(const QString &)));
	connect(m_endpoint, SIGNAL(signal_OnConnectionEstablished(const QString &)), this, SLOT(slot_OnConnectionEstablished(const QString &)));
	connect(m_endpoint, SIGNAL(signal_OnConnectionCleared(const QString &)), this, SLOT(slot_OnConnectionCleared(const QString &)));
	connect(m_endpoint, SIGNAL(signal_OnAnswerCall(const QString &)), this, SLOT(slot_OnAnswerCall(const QString &)));
	connect(m_endpoint, SIGNAL(signal_ShowVideoPanels(bool)), this, SLOT(slot_ShowVideoPanels(bool)));
	connect(m_endpoint, SIGNAL(signal_CreateConnection()), this, SLOT(slot_CreateConnection()));
	
	m_endpoint->Initialise(this);		
	
	// выставляю начальные уровни громкости и микрофона
	unsigned int vol;
	PSoundChannel sound;
	PString devName;
	PSoundChannel::Directions dir; 
	devName = m_endpoint->GetSoundChannelRecordDevice();
	dir = PSoundChannel::Recorder;
	if(sound.Open(devName, dir))
	{
		sound.GetVolume(vol);
		sound.Close();
		ui.sldMic->setValue(vol);
	}
	devName = m_endpoint->GetSoundChannelPlayDevice();
	dir = PSoundChannel::Player;
	if(sound.Open(devName, dir))
	{
		sound.GetVolume(vol);
		sound.Close();
		ui.sldSound->setValue(vol);
	}
/*
#ifdef _WIN32
	double val;
	val = GetVolumeX(Player, MIXERLINE_COMPONENTTYPE_DST_SPEAKERS);	
	ui.sldSound->setValue(val*100);
	val = GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_WAVEIN);
	ui.sldMic->setValue(val*100);
#endif
*/
	connect(ui.sldSound, SIGNAL(valueChanged(int)), this, SLOT(slot_ChangeSoundCmd(int)));
	connect(ui.sldMic, SIGNAL(valueChanged(int)), this, SLOT(slot_ChangeMicCmd(int)));
}

QtPhoneDlg::~QtPhoneDlg()
{
	if(trayIconMenu)
		delete trayIconMenu;
	if(trayIcon)
		delete trayIcon;
}

void QtPhoneDlg::closeEvent(QCloseEvent *ev)
{
	if (QSystemTrayIcon::isSystemTrayAvailable())
	{
		hide();
		ev->ignore();
	}
	else
	{
		if(!Close())
			ev->ignore();
	}
}
	
void QtPhoneDlg::slot_SetIcon(int index)
{
	if(iconComboBox && trayIcon)
	{
		QIcon icon = iconComboBox->itemIcon(index);

		trayIcon->setIcon(icon);

		//setWindowIcon(icon);

		trayIcon->setToolTip(iconComboBox->itemText(index));
	}

	m_state = (states)index;
}
 
void QtPhoneDlg::slot_IconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
	case QSystemTrayIcon::Trigger:
	case QSystemTrayIcon::DoubleClick:
		if(m_state==STATE_CALL)
			slot_OnCall();
		else
			showNormal();
		break;
	case QSystemTrayIcon::MiddleClick:
		break;
	default:
		break;
	}
}

void QtPhoneDlg::slot_Close()
{
	if(Close())
		QApplication::quit(); 
}

int QtPhoneDlg::Close()
{
	if(QMessageBox::question(this, tr("Закрытие приложения"), 
		tr("Вы действительно хотите выйти из приложения"), 
		tr("Да"), tr("Нет"), tr("Отмена"))!=0)
		return 0;

	if(m_endpoint->HasConnection(m_token))
		m_endpoint->ClearCall(m_token);
	delete m_endpoint;

	return 1;
}
 
#if PTRACING	// If Compiled with PTlib Tracing support (like in OpenH323)

bool QtPhoneDlg::OpenTraceFile(PConfig & config)
{
	// setting TRACE options
	//PTrace::SetLevel(config.GetInteger(TraceLevelConfigKey, 1));
	PTrace::SetLevel(TraceLevel);
	PTrace::SetOptions(PTrace::FileAndLine);
	PTrace::SetOptions(PTrace::TraceLevel);
	PTrace::SetOptions(PTrace::DateAndTime);
	
	PString traceFileName = "qtphone_trc.txt";
	
	// If already have a trace file, see if need to close it
	if (myTraceFile != NULL) {
		// If no change, do nothing more
		if (myTraceFile->GetFilePath() == PFilePath(traceFileName))
			return TRUE;
		
		PTrace::SetStream(NULL);
		delete myTraceFile;
		myTraceFile = NULL;
	}
	
	// Have stopped 
	if (traceFileName.IsEmpty())
		return TRUE;
	
	PTextFile * traceFile = new PTextFile;
	if (traceFile->Open(traceFileName, PFile::WriteOnly)) {
		myTraceFile = traceFile;
		PTrace::SetStream(traceFile);
		PProcess & process = PProcess::Current();
		PTRACE(0, process.GetName()
			<< " Version " << process.GetVersion(TRUE)
			<< " by " << process.GetManufacturer()
			<< " on " << process.GetOSClass() << ' ' << process.GetOSName()
			<< " (" << process.GetOSVersion() << '-' << process.GetOSHardware() << ')');
		
		return TRUE;
	}
	
	//  OutputStatusStr("ERROR! Trace failed.", S_SYSTEM);
	
	delete traceFile;
	return FALSE;
}

#endif

void QtPhoneDlg::slot_OnCall()
{
	//если пришли из клика по сообщению
	if(trayIcon && sender()==trayIcon && m_state==STATE_WORK)// пришло сообщение
	{
		show();
		return;
	}

	if(!m_endpoint->HasConnection(m_token))
	{
		QString m_destination = ui.txtAddress->text();

		CAddrBook::User2Addr(m_destination);

		QString curDest = m_destination;
		curDest.simplified();
		int iPos=-1;
		if((iPos=curDest.indexOf(" "))>0)
			curDest = curDest.mid(iPos);
		//ui.cmdCall->setEnabled(false);
		puts(curDest.toAscii().data());
		puts("m_token:");
		puts((const char *)m_token);
		m_endpoint->MakeCall((const char *)curDest.toAscii().data(), m_token);
	}
	else
	{
		ringSoundTimer.Stop();

		ui.cmdCall->setText(tr("Звонить"));

		H323Connection * connection = m_endpoint->FindConnectionWithLock(m_token);
		if (connection == NULL)
			ui.cmdCall->setEnabled(true);
		else
		{
			connection->AnsweringCall(H323Connection::AnswerCallNow);
			connection->Unlock();
		}
	}
}

void QtPhoneDlg::slot_CreateConnection()
{
	ui.cmdCall->setEnabled(false);
	ui.cmdRefuse->setEnabled(true);
	ui.actRefuse->setEnabled(true);
	ui.actCall->setEnabled(false);
}

void QtPhoneDlg::slot_OnConnectionEstablished(const QString &remotename)
{
	slot_SetIcon(STATE_WORK);

	//ui.sldSound->setEnabled(false);
	//ui.sldMic->setEnabled(false);

	ui.cmdCall->setEnabled(false);
	ui.cmdRefuse->setEnabled(true);
	ui.actRefuse->setEnabled(true);
	ui.actCall->setEnabled(false);
	ui.cmdRefuse->setText(tr("Отменить"));
	ui.actRefuse->setText(tr("Закончить разговор"));

	QString text = tr("Разговариваем с: ") + remotename;
	slot_OutputStatus(text);

	ui.txtSendMsg->clear();
	ui.pnlMsg->setEnabled(true);
	ui.actShowVideo->setEnabled(true);
	
	m_pLblStat->setText("");

	indTimer.RunContinuous(200);
}

void QtPhoneDlg::slot_OutputUserMsg(const QString &text)
{
	QListWidgetItem *pItem = new QListWidgetItem(text);
	ui.listMsg_2->addItem(pItem);
	ui.listMsg_2->setCurrentItem(pItem);

	if(trayIcon && !isVisible())
		trayIcon->showMessage(tr("Внимание"), text, QSystemTrayIcon::Information);
	//puts(text.toAscii().data());
}

void QtPhoneDlg::slot_OutputStatus(const QString &text)
{
	QListWidgetItem *pItem = new QListWidgetItem(text);
	ui.listMsg->addItem(pItem);
	ui.listMsg->setCurrentItem(pItem);
	//puts(text.toAscii().data());
}

QString ParseBytes(int ibutes)
{
	// add "b", "Kb", "Mb", "Gb" to the bytes
	QString sResult;
	char buff[10];
	if(ibutes< 1024)
	{
		sResult = QString("%1 b").arg(ibutes);
	}
	else if(ibutes/1024 < 1024)
	{
		sprintf(buff, "%.1f", (double)ibutes/1024.0);
		sResult = QString(buff) + " kb";
	}
	else if(ibutes/1024/1024 < 1024)
	{
		sprintf(buff, "%.1f", (double)ibutes/1024.0/1024.0);
		sResult = QString(buff) + " Mb";
	}
	else if(ibutes/1024/1024/1024 < 1024)
	{
		sprintf(buff, "%.1f", (double)ibutes/1024.0/1024.0/1024.0);
		sResult = QString(buff) + " Gb";
	}
	return sResult;
}

void QtPhoneDlg::ShowStats() const
{
	if(hideStat)
		return;

	QTime tCll;
	QString txt = QString("В: %1\tО: %2(%3/s)\tП: %4(%5/s)\tЗ: %6 ms")
		.arg(tCll.addSecs(m_endpoint->m_stat.iSecs).toString("HH:mm:ss"))
		.arg(ParseBytes(m_endpoint->m_stat.ibSent))
		.arg(ParseBytes(m_endpoint->m_stat.ibSent/(m_endpoint->m_stat.iSecs>0?m_endpoint->m_stat.iSecs:1)))
		.arg(ParseBytes(m_endpoint->m_stat.ibRcvd))
		.arg(ParseBytes(m_endpoint->m_stat.ibRcvd/(m_endpoint->m_stat.iSecs>0?m_endpoint->m_stat.iSecs:1)))
		.arg(m_endpoint->m_stat.iDelay);
		
	m_pLblStat->setText(txt);
}

void QtPhoneDlg::slot_OnConnectionCleared(const QString &remotename)
{
	slot_SetIcon(STATE_READY);

	//ui.sldSound->setEnabled(true);
	//ui.sldMic->setEnabled(true);

	ui.prBarSound->setValue(0);
	ui.prBarMic->setValue(0);

	m_token = "";

	indTimer.Stop();
	ringSoundTimer.Stop();
	
	if(autohideVideoPan && showVideoPan)	
		slot_ShowVideoPanels(false);  // hide Video panel if it's visible
	
	ui.cmdMuteSpeaker->setChecked(false);
	ui.cmdMuteMicrophone->setChecked(false);
	slot_MuteSpeakerCmd(false);
	slot_MuteMicrophoneCmd(false);

	ui.actCall->setEnabled(false);
	ui.cmdRefuse->setEnabled(false);
	ui.actRefuse->setEnabled(false);
	ui.cmdRefuse->setText(tr("Отказать"));
	ui.actRefuse->setText(tr("Отказать"));
	ui.cmdCall->setEnabled(true);
	ui.cmdCall->setText(tr("Звонить"));

	QString msg = QString("Соединение с %1 завершено.").arg(remotename);
	slot_OutputStatus(msg);

	m_pLblStat->setText("");

	// message window OFF
	ui.pnlMsg->setEnabled(false);
	ui.actShowVideo->setEnabled(false);
	ui.txtSendMsg->setPlainText("Ваши текстовые сообщения...");
	ui.listMsg_2->clear();
}

void QtPhoneDlg::slot_SetLevelVolume(unsigned int val)
{
	if(val > ui.prBarSound->maximum())
		ui.prBarSound->setMaximum(val);
	ui.prBarSound->setValue(val);
}

void QtPhoneDlg::slot_SetLevelMic(unsigned int val)
{
	if(val > ui.prBarMic->maximum())
		ui.prBarMic->setMaximum(val);
	ui.prBarMic->setValue(val);
}

void QtPhoneDlg::OnUpdateIndicators(PTimer &, INT)
{
	H323Connection * connection = m_endpoint->FindConnectionWithLock(m_token);
	if (connection == NULL)
		return;
	
	//static unsigned mS=0, mM=0;
	unsigned val;
	H323Channel * channel = connection->FindChannel(RTP_Session::DefaultAudioSessionID, TRUE);
	if (channel != NULL)
	{
		H323AudioCodec * codec = (H323AudioCodec *)channel->GetCodec();
		val = codec->GetAverageSignalLevel();
		//printf("SetCurrentLevel=%d\n", val);
		//if(mS<val)
		//	mS = val;
		emit signal_SetLevelVolume(val);
	}
	
	channel = connection->FindChannel(RTP_Session::DefaultAudioSessionID, FALSE);
	if (channel != NULL)
	{
		H323AudioCodec * codec = (H323AudioCodec *)channel->GetCodec();
/*
		PBoolean inTalkBurst;
		unsigned threshold;
		if (codec->GetSilenceDetectionMode(&inTalkBurst, &threshold) != H323AudioCodec::NoSilenceDetection)
		{
			//ui.prBarSound->setEnable(inTalkBurst);
			//printf("SetMarkerLevel=%d\n", threshold);
			//printf("SetActive=%d\n", inTalkBurst);
			//m_micindicator.SetMarkerLevel(threshold);
			//m_micindicator.SetActive(inTalkBurst);
		}
*/
		val = codec->GetAverageSignalLevel();
		//printf("\tSetCurrentMic=%d\n", val);
		//if(mM<val)
		//	mM = val;
		emit signal_SetLevelMic(val);
	}
	connection->Unlock();
}

QString GetSimpleAdr(QString fullAdr)
{
	// Try to extract IP from full h323 address
	int iFrom = fullAdr.indexOf('$');
	int iTo = fullAdr.indexOf(':');
	if(iFrom>=0)
		return fullAdr.mid(iFrom+1, iTo>=0?iTo-iFrom-1:NULL);
	else
		return fullAdr;
	
}

QString QtPhoneDlg::FindContactName(const H323Connection & connection)
{
	// Try to find contact in PhoneBook
	const char *goodName = (const char*)connection.GetRemotePartyName();
	QString callName = QString::fromUtf8(goodName);	
	QString callAdr = GetSimpleAdr(tr((const char*)connection.GetRemotePartyAddress()));

	CAddrBook::CheckAndAddUser(callName, callAdr);
/*
	QStringList allList = adrbook.GetSections();
	for(int ind=0; ind<allList.GetSize(); ind++)
		if(adrbook.GetString(allList[ind], "Address", "-noaddress-")==(LPCTSTR)callAdr)
			return (LPCTSTR)adrbook.GetString(allList[ind], "Name", (LPCTSTR)callName);
*/
	return callName;
}

void QtPhoneDlg::slot_OnAnswerCall(const QString &remotename)
{
	slot_SetIcon(STATE_CALL);

	QString caller = remotename + QString(" звонит.");
	
	if(trayIcon && !m_endpoint->m_fAutoAnswer)
		trayIcon->showMessage(tr("Внимание"), caller, QSystemTrayIcon::Information);

	ui.actCall->setEnabled(true);
	ui.cmdRefuse->setEnabled(true);
	ui.actRefuse->setEnabled(true);
	ui.cmdCall->setEnabled(true);
	ui.cmdCall->setText(tr("Ответить"));
	ui.cmdRefuse->setText(tr("Отказать"));
    slot_OutputStatus(caller);
}

void QtPhoneDlg::OnRingSoundAgain(PTimer &, INT)
{
	if (!ringSoundFile)
		PSound::PlayFile(ringSoundFile, FALSE);
	//ChangeIcon(CString(""));
}

void QtPhoneDlg::slot_OnRefuse() 
{
	if(autohideVideoPan && showVideoPan)	
		slot_ShowVideoPanels(false);  // hide Video panel if it's visible

	if(!m_endpoint->HasConnection(m_token))
	{
		H323Connection * connection = m_endpoint->FindConnectionWithLock(m_token);
		if (connection == NULL)
		{
			//ui.cmdCall->setEnabled(true);
		}
		else 
		{
			connection->AnsweringCall(H323Connection::AnswerCallDenied);
			connection->Unlock();
		}
	}
	else
	{
		m_endpoint->ClearCall(m_token);
	}
}

void QtPhoneDlg::SendUserInput(const QString &strusermsg)
{
//puts("1---------------");
	H323Connection * connection = m_endpoint->FindConnectionWithLock(m_token);
	if (connection == NULL)
		return;
	
//puts("2---------------");
	connection->SendUserInput(PString(strusermsg.toAscii().data()));
//puts("3---------------");
	connection->Unlock();

//puts("4---------------");
	QString msg = QString("-> Вы сказали: ""%1""").arg(strusermsg);
	slot_OutputUserMsg(msg);
}

void QtPhoneDlg::slot_SendMsg()
{
	QString msg = ui.txtSendMsg->toPlainText();
	SendUserInput(msg);
	ui.txtSendMsg->clear();
}

void QtPhoneDlg::slot_OnSettings()
{
	CSettingsDlg setupDlg(m_endpoint);
	if(setupDlg.exec() && !m_endpoint->HasConnection(m_token))
	{
		// Reload Endpoint capabilities
		m_endpoint->LoadCapabilities();
		ringSoundFile = m_endpoint->config.GetString(RingSoundFileConfigKey, "call.wav");
	}
}

void QtPhoneDlg::slot_ShowVideoPanels(bool show)
{
	if(sender()!=ui.actShowVideo)
	{
		ui.actShowVideo->blockSignals(true);
		ui.actShowVideo->setChecked(show);
		ui.actShowVideo->blockSignals(false);
	}

	if(m_endpoint->m_vdlg)
	{
		m_endpoint->m_vdlg->m_mutex.lock();
		m_endpoint->m_vdlg->m_image = QImage();
		m_endpoint->m_vdlg->m_mutex.unlock();
		m_endpoint->m_vdlg->setHidden(!show);
	}
}

void QtPhoneDlg::slot_MuteMicrophoneCmd(bool state)
{
	ui.cmdMuteMicrophone->setIcon(state ? QIcon(":/images/Resources/mike_p2.png") :
		QIcon(":/images/Resources/mike_p1.png"));

	H323Connection * connection = m_endpoint->FindConnectionWithLock(m_token);
	if (connection == NULL)
		return;
	
	H323Channel * channel = connection->FindChannel(RTP_Session::DefaultAudioSessionID, FALSE);
	if (channel != NULL) 
	{
		//bool newState = !channel->IsPaused();
		channel->SetPause(state);

	}
	
	connection->Unlock();
}

void QtPhoneDlg::slot_MuteSpeakerCmd(bool state)
{	
	ui.cmdMuteSpeaker->setIcon(state ? QIcon(":/images/Resources/sound_button_2.png") :
		QIcon(":/images/Resources/sound_button.png"));

	H323Connection * connection = m_endpoint->FindConnectionWithLock(m_token);
	if (connection == NULL)
		return;

	H323Channel * channel = connection->FindChannel(RTP_Session::DefaultAudioSessionID, TRUE);
	if (channel != NULL)
	{
		//bool newState = !channel->IsPaused();
		channel->SetPause(state);
	}

	connection->Unlock();
}


void QtPhoneDlg::slot_ChangeSoundCmd(int val)
{
	
	if(m_endpoint->pSoundChannelReceiv)
	{
		m_endpoint->pSoundChannelReceiv->SetVolume(val);
	}
	else
	{
		PSoundChannel sound;
		PString devName;
		PSoundChannel::Directions dir; 
		dir = PSoundChannel::Player;
		devName = m_endpoint->GetSoundChannelPlayDevice();
		if(sound.Open(devName, dir))
		{
			sound.SetVolume(val);
			sound.Close();
		}
	}
	//printf("slot_ChangeSoundCmd (get=%d, set=%d)\n", uVal, val);
	
	//GetVolumeX(Player, MIXERLINE_COMPONENTTYPE_DST_SPEAKERS);	
	//SetVolumeX(Player, MIXERLINE_COMPONENTTYPE_DST_SPEAKERS, (double)(val/100.0));	
}

void QtPhoneDlg::slot_ChangeMicCmd(int val)
{
	
	if(m_endpoint->pSoundChannelTransmit)
	{
		m_endpoint->pSoundChannelTransmit->SetVolume(val);
	}
	else
	{
		PSoundChannel sound;
		PString devName;
		PSoundChannel::Directions dir; 
		dir = PSoundChannel::Recorder;
		devName = m_endpoint->GetSoundChannelRecordDevice();
		if(sound.Open(devName, dir))
		{
			sound.SetVolume(val);
			sound.Close();
		}
	}
	//printf("slot_ChangeMicCmd (get=%d, set=%d)\n", uVal, val);
	
/*
	int x;
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_FIRST);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_UNDEFINED);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_DIGITAL);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_LINE);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_MONITOR);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_SPEAKERS);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_HEADPHONES);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_TELEPHONE);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_WAVEIN);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_VOICEIN);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_LAST);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_FIRST);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_DIGITAL);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_LINE);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_ANALOG);	
	x=GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_SRC_LAST);	

*/
	//GetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_WAVEIN);	
	//SetVolumeX(Recorder, MIXERLINE_COMPONENTTYPE_DST_WAVEIN, (double)val/100.0);
}

void QtPhoneDlg::slot_RecvStats(const QString &txt)
{
	m_pLblStat->setText(txt);
}

void QtPhoneDlg::about()
{
	QString caption = tr("О программе");
	QString text = "<HTML><p><b>";

	text += tr("Терминал видеоконференции");
	text += "</b></p><p>";
	text += tr("Версия 2013.2.0.1");
	text += "</p>Отправить сообщение <a href=\"mailto:zaozerskiy@mail.ru?subject=Вопрос по \
'Терминалу видеоконференции'\"&body=1\">разработчику</a></HTML>";

	QMessageBox::about(this, caption, text);
}
 
void QtPhoneDlg::openContacts()
{
	CAddrBook addrBook(this);
	QString name;
	if(addrBook.getAddr(name))
	{
		ui.txtAddress->setText(name);
		slot_OnCall();
	}
}
