#include "mainwindow.h"
#include "sortfilterproxymodel.h"
#include "filterheader.h"

#include <QStandardItemModel>
#include <QTableView>

static const int rowCount = 250000; //生成一个有rowCount行的表
static const int columnCount = 4;  //生成一个有columnCount列的表

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	QStandardItemModel *model = new QStandardItemModel(this);

	for (int i = 0; i < rowCount; ++i) {
		QList<QStandardItem*> row;
		for (int j = 0; j < columnCount; ++j) {
			QStandardItem *item = new QStandardItem;
            if (j == 3) {//如果是第四列，该列的item是复选框
                item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled); //设置item类型为checkboxes
                if (i % 2)
					item->setCheckState(Qt::Unchecked);
				else
					item->setCheckState(Qt::Checked);
			}
            else {//其余第一到第三列则填充数字
                item->setText(QString::number(i + 1 + j));
			}
            row.append(item);//每行从左往右添加item
		}
        model->appendRow(row);//表格从上往下添加行
	}

	SortFilterProxyModel *proxy = new SortFilterProxyModel(this);
	proxy->setSourceModel(model);

	QTableView *view = new QTableView(this);
	view->setModel(proxy);

	FilterHeader *header = new FilterHeader(this);
	header->setCheckColumns(QList<int>() << 3);
	view->setHorizontalHeader(header);

	connect(header, SIGNAL(stringFilterChanged(int,QString)),
			proxy, SLOT(updateStringFilter(int,QString)));
	connect(header, SIGNAL(stateFilterChanged(int,Qt::CheckState)),
			proxy, SLOT(updateStateFilter(int,Qt::CheckState)));

	resize(500, 500);
	setCentralWidget(view);
}

MainWindow::~MainWindow()
{
}
