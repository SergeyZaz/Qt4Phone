#include "caddrbook.h"
#include <QFile>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QDomDocument>
#include <QTextStream>

#define FILE_NAME	"contacts.ini"

CAddrBook::CAddrBook(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	connect(ui.cmdAdd, SIGNAL(clicked()), this, SLOT(addAddr()));
	connect(ui.cmdDel, SIGNAL(clicked()), this, SLOT(delAddr()));
	connect(ui.cmdSave, SIGNAL(clicked()), this, SLOT(setInfo()));

	readIni(doc);
	saveIni(doc);

	model = new CTableAddrModel(&doc);
	ui.tblView->setModel(model->sortModel);
	ui.tblView->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
	ui.tblView->verticalHeader()->hide();
	ui.tblView->verticalHeader()->setDefaultSectionSize(20);  
	ui.tblView->setSortingEnabled(true);	
	
	connect(ui.tblView, SIGNAL(clicked(const QModelIndex &)), 
		this, SLOT(getInfo(const QModelIndex &)));
	connect(ui.tblView, SIGNAL(doubleClicked(const QModelIndex &)), 
		this, SLOT(applay(const QModelIndex &)));

	connect(ui.txtMask, SIGNAL(textChanged(const QString &)), 
		this, SLOT(changedMaskName(const QString &)));
	
	ui.txtName->setEnabled(false);
	ui.txtAddr->setEnabled(false);
	ui.cmdSave->setEnabled(false);

	model->Update();	
}

CAddrBook::~CAddrBook()
{
}
	
void CAddrBook::closeEvent(QCloseEvent *e)
{
}
	
int CAddrBook::readIni(QDomDocument &d)
{
	QDomProcessingInstruction processingInstruction = d.createProcessingInstruction("xml", 
		"version=\"1.0\" encoding=\"UTF-8\"");
	QDomElement rootElement;

	QFile file(FILE_NAME);
	if (!file.open(QFile::ReadOnly | QFile::Text)) 
	{
		d.appendChild(processingInstruction);
		rootElement = d.createElement("Contacts");
		rootElement.setAttribute("version", "1.0");
		d.appendChild(rootElement);
	}

	QString text;
	QIODevice *device = &file;
	int errorLine;
	int errorColumn;

	if (file.exists() && !d.setContent(device, true, &text, &errorLine,
		&errorColumn)) 
	{
		QMessageBox::information(NULL, tr("Ошибка чтения файла"),
			QString("Файл: %1, строка: %2; %3").arg(text).arg(errorLine).arg(text),
			QMessageBox::Ok);
		
		d.clear();
		d.appendChild(processingInstruction);
		rootElement = d.createElement("Contacts");
		rootElement.setAttribute("version", "1.0");
		d.appendChild(rootElement);
		return 0;
	}
	file.close(); 
	return 1;
}

void CAddrBook::applay(const QModelIndex &)
{
	accept();
}

void CAddrBook::changedMaskName(const QString &text)
{
    QRegExp regExp(text, Qt::CaseInsensitive, QRegExp::RegExp);
    model->sortModel->setFilterRegExp(regExp);
}

int CAddrBook::saveIni(QDomDocument &d)
{
	QFile file(FILE_NAME);
	if (!file.open(QFile::WriteOnly | QFile::Text)) 
	{
		QMessageBox::information(NULL, tr("Внимание!"), 
			tr("Невозможно создать файл"), QMessageBox::Ok);
		return 0;
	}
	QIODevice *device = &file;
	QTextStream out(device);
	d.save(out, 4);
	file.close(); 
	return 1;
}

int CAddrBook::CheckAndAddUser(const QString &name, const QString &addr)
{
	QDomDocument	t_doc;
	readIni(t_doc);

	bool fExist = false;
	QDomElement	child = t_doc.documentElement().firstChildElement("contact");
	while (!child.isNull()) 
	{
		if(child.attribute("addr") == addr)
		{
			fExist = true;
			//child.setAttribute("name", name);
			break;
		}
		child = child.nextSiblingElement("contact");
	}
	if(!fExist)
	{
		child = t_doc.createElement("contact"); 
		child.setAttribute("addr", addr);
		child.setAttribute("name", name);
		t_doc.documentElement().appendChild(child);
	}

	saveIni(t_doc);
	return 1;
}

int CAddrBook::User2Addr(QString &text)
{
	QDomDocument	t_doc;
	readIni(t_doc);

	QDomElement	child = t_doc.documentElement().firstChildElement("contact");
	while (!child.isNull()) 
	{
		if(child.attribute("name") == text)
		{
			text = child.attribute("addr");
			return 1;
		}
		child = child.nextSiblingElement("contact");
	}
	return 0;
}

int CAddrBook::getAddr(QString &addr)
{
	int rc;

	rc = exec();
	if(rc)
	{
		addr = cur_elem.attribute("addr");
	}

	return rc;
}

void CAddrBook::addAddr()
{
	QDomElement elem = doc.createElement("contact"); 
	elem.setAttribute("name", tr("Новый контакт"));
	doc.documentElement().appendChild(elem);

	model->Update();	
	saveIni(doc);
}

void CAddrBook::delAddr()
{
	QModelIndexList listItems = ui.tblView->selectionModel()->selectedRows();

	if(listItems.size()==0)
		return;

	if(QMessageBox::question(this, tr("Удаление контактов"),
		tr("Вы действительно хотите удалить выделенные контакты?"),
		tr("Да"), tr("Нет"), tr("Отмена"))!=0)
		return;

	QStringList delIds;
	int i;
	QDomNode node;

	for(i=0;i<listItems.size();i++)
	{
		node = model->getItem(listItems.at(i).data(Qt::UserRole+2).toInt());
		node.parentNode().removeChild(node);
	}

	model->Update();	
	saveIni(doc);
	
	cur_elem.clear();
	ui.txtName->setText("");
	ui.txtAddr->setText("");
	ui.txtName->setEnabled(false);
	ui.txtAddr->setEnabled(false);
	ui.cmdSave->setEnabled(false);
}

void CAddrBook::getInfo(const QModelIndex &index)
{
	ui.txtName->setEnabled(true);
	ui.txtAddr->setEnabled(true);
	ui.cmdSave->setEnabled(true);
	cur_elem = model->getItem(index.data(Qt::UserRole+2).toInt()).toElement();
	ui.txtName->setText(cur_elem.attribute("name"));
	ui.txtAddr->setText(cur_elem.attribute("addr"));
}

void CAddrBook::setInfo()
{
	cur_elem.setAttribute("name", ui.txtName->text());
	cur_elem.setAttribute("addr", ui.txtAddr->text());
	model->Update();	
	saveIni(doc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

CTableAddrModel::CTableAddrModel(QDomDocument *doc)
{
	pdoc = doc;

	sortModel = new QSortFilterProxyModel();
	sortModel->setDynamicSortFilter(true); 
	sortModel->setFilterKeyColumn(0); 
	sortModel->setSourceModel(this); 

	headers << QString("Имя")
			<< QString("Адрес") 
			;
}

CTableAddrModel::~CTableAddrModel()
{
}

QVariant CTableAddrModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if(orientation == Qt::Horizontal && headers.size()>section)
		return headers.at(section);

	return section;
}

void CTableAddrModel::Update()
{
	reset();
}
	
int CTableAddrModel::rowCount(const QModelIndex &parent) const
{
	return pdoc->documentElement().elementsByTagName("contact").size();
}
	
QDomNode CTableAddrModel::getItem(int r)
{
	QDomNodeList nodes = pdoc->documentElement().elementsByTagName("contact");
	if (nodes.size()<=r)
		return QDomElement();
	return nodes.at(r);
}

QVariant CTableAddrModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int r = index.row(),
		c = index.column();

	QDomNodeList nodes = pdoc->documentElement().elementsByTagName("contact");

	if (nodes.size()<=r)
		return QVariant();

	QDomElement elem = nodes.at(r).toElement();

	switch(role)
	{
	case Qt::UserRole:
		return elem.attribute("addr");
	case Qt::UserRole+1:
		return elem.attribute("name");
	case Qt::UserRole+2:
		return r;
	case Qt::DisplayRole:
	case Qt::EditRole:
		switch(c)
		{
		case 0:
			return elem.attribute("name");
		case 1:
			return elem.attribute("addr");
		default:
			break;
		}
	default:
        return QVariant();
	}
		
	return QVariant();
}

