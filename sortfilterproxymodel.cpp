#include "sortfilterproxymodel.h"

SortFilterProxyModel::SortFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setSortCaseSensitivity(Qt::CaseInsensitive);//设置字母大小写不敏感
	setSortLocaleAware(true);
}

//根据输入filter的字符串更新表格
void SortFilterProxyModel::updateStringFilter(int column, const QString &pattern)
{
    //编辑_columnPatterns键值对里的内容
    //如果当前_columnPatterns不包含输入所在的列，则插入新的键值对，也就是支持输入多列的filter！
	if (!_columnPatterns.contains(column))
		_columnPatterns.insert(column, QRegExp(pattern, Qt::CaseInsensitive, QRegExp::WildcardUnix));
    //如果filter里没有输入东西，则从_columnPatterns移除所在列
	else if (pattern.isEmpty())
		_columnPatterns.remove(column);
	else
        //更新当前列的filter里输入的内容
		_columnPatterns[column].setPattern(pattern);

	// Prefer over invalidateFilter() due https://bugreports.qt.io/browse/QTBUG-14355
    //Invalidate()仅仅是通知系统：此时的窗体已经变为无效。强制系统调用WM_PAINT，而这个消息仅仅被放入消息队列，当程序运行到WM_PAINT消息时才会对窗口进行重绘。
	invalidate();
}

void SortFilterProxyModel::updateStateFilter(int column, const Qt::CheckState &cs)
{
	if (!_columnStates.contains(column))
		_columnStates.insert(column, cs);
	else if (cs == Qt::CheckState::PartiallyChecked)
		_columnStates.remove(column);
	else
		_columnStates[column] = cs;

	// Prefer over invalidateFilter() due https://bugreports.qt.io/browse/QTBUG-14355
	invalidate();
}

//重写filterAcceptsRow()
bool SortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    //过滤含有filter输入框字符串的行
	for (QMap<int, QRegExp>::const_iterator it = _columnPatterns.constBegin();
		it != _columnPatterns.constEnd(); ++it) {

		QModelIndex index = sourceModel()->index(sourceRow, it.key(), sourceParent);
		if (!index.data().toString().contains(it.value()))
			return false;
	}
    //
	for (QMap<int, Qt::CheckState>::const_iterator it = _columnStates.constBegin();
		it != _columnStates.constEnd(); ++it) {

		QModelIndex index = sourceModel()->index(sourceRow, it.key(), sourceParent);
		if (index.data(Qt::CheckStateRole) != it.value())
			return false;
	}

	return true;
}
