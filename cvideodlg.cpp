#include <QPainter>
#include "cvideodlg.h"
#include "qt4phonedlg.h"

CVideoDlg::CVideoDlg(QtPhoneDlg *dlg)
	: QDialog(0)
{
	ui.setupUi(this);	
	installEventFilter(this);
	setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);

	p_dlg = dlg;
}

CVideoDlg::~CVideoDlg()
{

}
	
void CVideoDlg::closeEvent(QCloseEvent *ev)
{
	p_dlg->slot_ShowVideoPanels(false);
}

bool CVideoDlg::eventFilter(QObject *obj, QEvent *event)
{
	static int on_off = 0;

	if (event->type() == QEvent::MouseButtonDblClick)
	{
 		if(!on_off)
			showFullScreen();
		else
			showNormal();
		on_off = 1 - on_off; 
		return true;
	}
	if (event->type() == QEvent::Paint) 
	{
		if(m_image.isNull())
		    return false;
		    
		QPainter painter(this);


		QRect tRect = geometry();
		QSize tSize = size();

		m_mutex.lock();

//		printf("image(%d, %d), size(%d,%d), geo(%d, %d)\n", 
//		    m_image.width(), m_image.height(),
//		    tSize.width(), tSize.height(),
//		    tRect.width(), tRect.height()
//		    );


		if(m_image.width()>tSize.width() || m_image.height()>tSize.height())
		{
    		    resize(m_image.size());
		    painter.drawImage(0,0, m_image);
		}
		else
		{
			QImage	m_imageNew = m_image.scaled(tRect.width(), tRect.height(), Qt::KeepAspectRatio);
			int x,y;
			x = (m_imageNew.width()>tSize.width()) ? 0 : (tSize.width() - m_imageNew.width()) / 2;	
			y = (m_imageNew.height()>tSize.height()) ? 0 : (tSize.height() - m_imageNew.height()) / 2;	
		    painter.drawImage(x, y, m_imageNew);
		}

		m_mutex.unlock();

		return true;
	}
	return QObject::eventFilter(obj, event);
}
