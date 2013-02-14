#ifndef CADDRBOOK_H
#define CADDRBOOK_H

#include <QDialog>
#include <QCloseEvent>
#include <QSortFilterProxyModel>
#include <QDomDocument>
#include "ui_caddrbook.h"

class CTableAddrModel : public QAbstractTableModel
{
	Q_OBJECT
	
	QDomDocument	*pdoc;
	
	QStringList headers;
	QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const;

public:
	QSortFilterProxyModel		*sortModel;

	CTableAddrModel(QDomDocument *doc);
	~CTableAddrModel();

	void Update();
	QDomNode getItem(int r);

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const {return headers.size();}

	QVariant data(const QModelIndex &index, int role) const;
};


class CAddrBook : public QDialog
{
	Q_OBJECT

	CTableAddrModel *model;
	QDomDocument	doc;
	QDomElement		cur_elem;

	void closeEvent(QCloseEvent *e);

public:
	CAddrBook(QWidget *parent = 0);
	~CAddrBook();
	int getAddr(QString &addr);

	static int saveIni(QDomDocument &d);
	static int readIni(QDomDocument &d);
	static int CheckAndAddUser(const QString &name, const QString &addr);
	static int User2Addr(QString &text);

public slots:
	void addAddr();
	void delAddr();
	void getInfo(const QModelIndex &index);
	void setInfo();
	void changedMaskName(const QString &);
	void applay(const QModelIndex &);

private:
	Ui::CAddrBook ui;
};

#endif // CADDRBOOK_H
