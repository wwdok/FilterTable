#include "filterheader.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QContextMenuEvent>

static const int filterReactionTime = 500; // in millisecons，代表软件在你输入完过滤值后多少毫秒开始反应，配置越高的电脑该值可以设得更小

FilterHeader::FilterHeader(QWidget *parent)
	: QHeaderView(Qt::Horizontal, parent), _menu(tr("Show or hide columns"), this)
{
	setSectionsClickable(true);
	setSectionsMovable(true);
	setDefaultAlignment(Qt::AlignTop | Qt::AlignHCenter);
	setContextMenuPolicy(Qt::DefaultContextMenu);

	connect(this, SIGNAL(sectionResized(int, int, int)),
            this, SLOT(handleSectionResized(int)));//这里得section相当于一整列，所以该函数用来调整某一整列的宽度
    connect(this, SIGNAL(sectionMoved(int, int, int)),
            this, SLOT(handleSectionMoved(int, int, int)));//这里得section相当于一整列，所以该函数用来调整某一整列的顺序

	connect(&_actionSignalMapper, SIGNAL(mapped(int)),
            this, SLOT(showOrHideColumn(int)));//右击表头，可以选勾选显示拿几列
	connect(&_filterSignalMapper, SIGNAL(mapped(int)),
			this, SLOT(filterChanged(int)));

	_filterTimer.setSingleShot(true);
	connect(&_filterTimer, SIGNAL(timeout()),
			this, SLOT(triggerFilter()));
}

FilterHeader::~FilterHeader()
{
}

void FilterHeader::fixWidgetPositions()
{
	for (int i = 0; i < count(); ++i)
        setFilterGeometry(i);
}

void FilterHeader::setCheckColumns(const QList<int> &checkColumns)
{
	_checkColumns = checkColumns;
}

void FilterHeader::contextMenuEvent(QContextMenuEvent* event)
{
    _menu.popup(event->globalPos());
}

void FilterHeader::showEvent(QShowEvent *e)
{
	QFont font;
	QFontMetrics fm(font);
	_textHeight = 0;

	// Create our filters and actions and determine max height via each section name
    for (int i = 0; i < count(); ++i) {
        if (!_filters[i]) {  //_filters是一个键值对字典，键是列序号，值是Qwidget，这里就是输入框和复选框
			createFilter(i);
			createAction(i);
		}
		QString title = model()->headerData(i, orientation()).toString();

		// Cannot use boundingRect(QString) due possible bug in Qt
		// see https://bugreports.qt.io/browse/QTBUG-15974
		QRect rect = fm.boundingRect(QRect(0,0,0,0), Qt::AlignLeft|Qt::AlignBottom, title);
		if (rect.height() > _textHeight)
			_textHeight = rect.height();
    }
	// Now add some space and set height of header
	_textHeight += 10;
	setFixedHeight(_textHeight + QLineEdit().sizeHint().height() + 17);

	// Finally show each filter
	for (int i = 0; i < count(); ++i) {
		setFilterGeometry(i);
		_filters[i]->show();
	}
    QHeaderView::showEvent(e);
}

void FilterHeader::createFilter(int column)
{
	if (_checkColumns.contains(column)) {
	    QCheckBox *box = new QCheckBox(this);
	    box->setTristate(true);
		box->setCheckState(Qt::CheckState::PartiallyChecked);

	    connect(box, SIGNAL(stateChanged(int)),
			    &_filterSignalMapper, SLOT(map()));
        _filterSignalMapper.setMapping(box, column);  //https://stackoverflow.com/a/24261056/12169382
	    _filters[column] = box;
    }
    else {
        QLineEdit *edit = new QLineEdit(this);  //创建过滤器的输入框
        edit->setPlaceholderText(tr("Filter")); //设置输入框的占位提示符为“Filter”
        edit->setToolTip(tr("Wildcards *, ? oder [] erlaubt. Zum Freistellen \\ vor die Wildcard setzen"));//鼠标长时间悬停后显示的提示符，用于说明过滤器的规则
        //这句话是德语，翻译成中文是“允许的通配符有*, ? 或 [], 去掉放在通配符前面的\\”

	    connect(edit, SIGNAL(textChanged(QString)),
			    &_filterSignalMapper, SLOT(map()));
        _filterSignalMapper.setMapping(edit, column);  //https://stackoverflow.com/a/24261056/12169382
	    _filters[column] = edit;
    }
}

void FilterHeader::createAction(int column)
{
	QString title = model()->headerData(column, orientation()).toString();
	QAction *action = new QAction(title.simplified(), &_menu);
	action->setCheckable(true);
	action->setChecked(!isSectionHidden(column));

	connect(action, SIGNAL(triggered()),
			&_actionSignalMapper, SLOT(map()));
	_actionSignalMapper.setMapping(action, column);
	_columnActions.insert(column, action);
	_menu.addAction(action);
}

void FilterHeader::handleSectionResized(int logical)
{
    for (int i = visualIndex(logical); i < count(); ++i)
        setFilterGeometry(logicalIndex(i));
}

void FilterHeader::handleSectionMoved(int logical, int oldVisualIndex, int newVisualIndex)
{
    for (int i = qMin(oldVisualIndex, newVisualIndex); i < count(); ++i)
		setFilterGeometry(logicalIndex(i));
}

//设置过滤器输入框的矩形框尺寸
void FilterHeader::setFilterGeometry(int column)
{
	if (_filters[column]) {
        _filters[column]->setGeometry(sectionViewportPosition(column) + 5, _textHeight,  //+5代表缩进5像素，-10代表输入框两边都缩进5像素，总宽度比单元格宽度少了10
                                      sectionSize(column) - 10, _filters[column]->height());  //void setGeometry(const QRect &);
	}
}

void FilterHeader::showOrHideColumn(int column)
{
    //根据头文件，_columnActions是一个键值对，键是列的序号，值是是否隐藏某列的动作
    //如果请求的列不属于1 2 3 4 中的一个，则不予受理，直接返回
    if (!_columnActions.contains(column))
        return;
    //获取选择的列对应的动作
    QAction* action = _columnActions[column];
    //如果
    if (action->isChecked()) {
        showSection(column);
    }
	else {
        // If the user hides every column then the table will disappear. This
        // guards against that. NB: hiddenCount reflects checked QAction's so
        // size-hiddenCount will be zero the moment they uncheck the last
        // section
        if (_columnActions.size() - hiddenCount() > 0) {
            hideSection(column);
        }
        else { //如果_columnActions.size() = hiddenCount()，即总列数等于要隐藏的列数时，不予受理，即至少要显示一列
            // Otherwise, ignore the request and re-check this QAction
            action->setChecked(true);
        }
    }
}

//计算已隐藏的列的数量
int FilterHeader::hiddenCount()
{
    int count = 0;
	for (auto action : _columnActions) {
        if (!action->isChecked())
            ++count;
    }

    return count;
}

void FilterHeader::filterChanged(int column)
{
	if (_filterTimer.isActive() && _currentColumn != column) {
		// Let current column complete it's filtering first of all
		_filterTimer.stop();
		triggerFilter();
	}

	_currentColumn = column;
    _filterTimer.start(filterReactionTime);
}

//过滤触发类型是字符串的改变还是复选框的改变
void FilterHeader::triggerFilter()
{
	QLineEdit *edit = qobject_cast<QLineEdit*>(_filters[_currentColumn]);
	if (edit)
        emit stringFilterChanged(_currentColumn, edit->text()); //将目前输入Filter的列序号和内容作为信号发送出去，由connect知，接收函数是updateStringFilter()

	QCheckBox *box = qobject_cast<QCheckBox*>(_filters[_currentColumn]);
	if (box)
		emit stateFilterChanged(_currentColumn, box->checkState());
}
