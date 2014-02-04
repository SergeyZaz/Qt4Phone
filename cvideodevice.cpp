#include <QFile>
#include <QImage>
#include "cvideodlg.h"
#include "cvideodevice.h"
#include "myphoneendpoint.h"

CVideoOutputDevice::CVideoOutputDevice(
	CMyPhoneEndPoint* pendpoint,
	bool fFlipVertically, 
	bool fFlipHorizontally,
	bool isLocal,
	bool doLocalVideoPnP) :
m_videoDlg(pendpoint->m_vdlg),
	m_isLocal(isLocal),
	m_doLocalVideoPnP(doLocalVideoPnP),
	m_fVertFlip(fFlipVertically),
	m_fHorzFlip(fFlipHorizontally),

	m_bIsInitialized(false)
{
	connect(this, SIGNAL(signal_ShowVideoPanels(bool)), pendpoint, SIGNAL(signal_ShowVideoPanels(bool)));
	connect(this, SIGNAL(signal_UpdateVideoDlg()), m_videoDlg, SLOT(update()));

	m_x = m_y = 0;  
	m_w = m_h = 200;

	emit signal_ShowVideoPanels(true);

	//m_rect = m_videoDlg->geometry();
	// Set for PIP
	//m_rect.setRight(m_doLocalVideoPnP&&m_isLocal ? m_rect.left()/2 : m_rect.left());
	//m_rect.setBottom(m_doLocalVideoPnP&&m_isLocal ? m_rect.bottom()/2 : m_rect.bottom());
}

CVideoOutputDevice::~CVideoOutputDevice()
{
	emit signal_ShowVideoPanels(false);
	Close();
}

PBoolean CVideoOutputDevice::Open(
		const PString & deviceName,   /// Device name to open
		PBoolean startImmediate    /// Immediately start device
		)
{
	return TRUE;
}

PBoolean CVideoOutputDevice::IsOpen()
{
	return TRUE;
}

PBoolean CVideoOutputDevice::Close()
{
	return TRUE;
}

#if PTLIB_MAJOR<=2 && PTLIB_MINOR<10
	PStringList CVideoOutputDevice::GetDeviceNames() const
	{
		PStringList  devlist;
		devlist.AppendString(GetDeviceName());
		return devlist;
	}
#else
	PStringArray CVideoOutputDevice::GetDeviceNames() const
	{
		PStringArray  devlist;
		devlist.AppendString(GetDeviceName());
		return devlist;
	}
#endif


PBoolean CVideoOutputDevice::SetFrameSize(
		unsigned width,   /// New width of frame
		unsigned height   /// New height of frame
		)
{
	//puts("CVideoOutputDevice::SetFrameSize()");
	return PVideoOutputDeviceRGB::SetFrameSize(width, height);
}

PBoolean CVideoOutputDevice::SetColourFormat(
		const PString & colourFormat // New colour format for device.
		)
{
	PWaitAndSignal m(mutex);

	printf("colourFormat=%s\n", (const char*)colourFormat);
	if (((colourFormat *= "BGR24") || (colourFormat *= "BGR32")) &&
		PVideoOutputDeviceRGB::SetColourFormat(colourFormat))
	{
		//m_bmi.bmiHeader.biBitCount = (WORD)(bytesPerPixel*8);
		//m_bmi.bmiHeader.biSizeImage = frameStore.GetSize();
		return TRUE;
	}

	return FALSE;
}
	
PBoolean CVideoOutputDevice::FrameComplete()
{

	unsigned int width = 0, height = 0;
	//width = GetFrameWidth();
	//height = GetFrameHeight();
	if(!m_doLocalVideoPnP || !GetFrameSize(width, height))
		return FALSE;

	//printf("CVideoOutputDevice::FrameComplete(%d, %d)\n", width, height);
   
	//size = frameStore.GetSize();

/*
	if(m_videoDlg->size().width() < width || m_videoDlg->size().height() < height)
	{
		puts("-----resize----");
		m_videoDlg->resize(width, height);
	}
*/

	m_videoDlg->m_mutex.lock();

	m_videoDlg->m_image = QImage((const uchar *)frameStore.GetPointer(), width, height, QImage::Format_RGB32);
	m_videoDlg->m_image = m_videoDlg->m_image.mirrored(m_fHorzFlip, m_fVertFlip);
	
	m_videoDlg->m_mutex.unlock();

	emit signal_UpdateVideoDlg();
	return TRUE;
}

