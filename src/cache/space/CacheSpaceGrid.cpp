#include "CacheSpaceGrid.h"
#include "cache/CacheItem.h"
#include "cache/CacheItemFactory.h"
#include "utils/auto_value.h"
#include <QMap>

namespace Qi
{

CacheSpaceGrid::CacheSpaceGrid(const QSharedPointer<SpaceGrid>& grid, ViewApplicationMask viewApplicationMask)
    : CacheSpace(grid, viewApplicationMask),
      m_grid(grid)
{
}

CacheSpaceGrid::~CacheSpaceGrid()
{
}

bool CacheSpaceGrid::isItemInFrame(const ItemID& visibleItem) const
{
    if (m_itemsCacheInvalid)
        validateItemsCache();

    return visibleItem.row >= m_itemStart.row && visibleItem.row <= m_itemEnd.row &&
            visibleItem.column >= m_itemStart.column && visibleItem.column <= m_itemEnd.column;
}

bool CacheSpaceGrid::isItemInFrameStrict(const ItemID& visibleItem) const
{
    if (m_itemsCacheInvalid)
        validateItemsCache();

    return visibleItem.row > m_itemStart.row && visibleItem.row < m_itemEnd.row &&
            visibleItem.column > m_itemStart.column && visibleItem.column < m_itemEnd.column;
}

bool CacheSpaceGrid::isItemAbsInFrame(const ItemID& absItem) const
{
    ItemID visibleItem = spaceGrid()->toVisible(absItem);
    if (!visibleItem.isValid())
        return false;

    return isItemInFrame(visibleItem);
}

void CacheSpaceGrid::visibleItemsRange(ItemID& itemStart, ItemID& itemEnd) const
{
    if (m_itemsCacheInvalid)
        validateItemsCache();

    itemStart = m_itemStart;
    itemEnd = m_itemEnd;
}

ItemID CacheSpaceGrid::visibleItemByPosition(const QPoint& point) const
{
    if (m_grid->isEmptyVisible())
        return ItemID();

    QPoint gridPoint = window2Space(point);

    ItemID visibleItem;
    visibleItem.row = m_grid->rows()->findVisibleIDByPos(gridPoint.y());
    visibleItem.column = m_grid->columns()->findVisibleIDByPos(gridPoint.x());

    return visibleItem;
}

void CacheSpaceGrid::clearItemsCacheImpl() const
{
    Q_ASSERT(!m_cacheIsInUse);

    m_itemStart = m_itemEnd = ItemID();
    m_items.clear();
    m_scrollDelta = QPoint(0, 0);
    m_sizeDelta = QSize(0, 0);
}

void CacheSpaceGrid::validateItemsCacheImpl() const
{
    Q_ASSERT(m_itemsCacheInvalid);

    if (m_grid->isEmptyVisible())
    {
        clearItemsCache();
        m_itemsCacheInvalid = false;
        return;
    }

    auto_value<bool> inUse(m_cacheIsInUse, true);

    const Lines& rows = *m_grid->rows();
    const Lines& columns = *m_grid->columns();

    int visibleRowStart = rows.findVisibleIDByPos(m_scrollOffset.y());
    int visibleRowEnd = rows.findVisibleIDByPos(m_scrollOffset.y() + m_window.height());
    int visibleColumnStart = columns.findVisibleIDByPos(m_scrollOffset.x());
    int visibleColumnEnd = columns.findVisibleIDByPos(m_scrollOffset.x() + m_window.width());

    Q_ASSERT(visibleRowStart != InvalidIndex);
    Q_ASSERT(visibleRowEnd != InvalidIndex);
    Q_ASSERT(visibleColumnStart != InvalidIndex);
    Q_ASSERT(visibleColumnEnd != InvalidIndex);

    ItemID newItemStart(visibleRowStart, visibleColumnStart);
    ItemID newItemEnd(visibleRowEnd, visibleColumnEnd);

    if ((m_itemStart == newItemStart) && (m_itemEnd == newItemEnd))
    {
        // just offset rectangles
        for (const auto& item: m_items)
            item->correctRectangles(m_scrollDelta);

        // clear offset
        m_scrollDelta = QPoint(0, 0);
        m_sizeDelta = QSize(0, 0);
        // mark items as valid
        m_itemsCacheInvalid = false;
        return;
    }

    // init new items with empty caches
    int newItemRows = newItemEnd.row - newItemStart.row + 1;
    int newItemColumns = newItemEnd.column - newItemStart.column + 1;
    QVector<QSharedPointer<CacheItem>> newItems(newItemRows * newItemColumns, QSharedPointer<CacheItem>());

    if (!m_items.isEmpty())
    {
        // find intersection between old and new items
        ItemID intersectionStart(qMax(m_itemStart.row, newItemStart.row), qMax(m_itemStart.column, newItemStart.column));
        ItemID intersectionEnd(qMin(m_itemEnd.row, newItemEnd.row), qMin(m_itemEnd.column, newItemEnd.column));

        // copy intersected cache items
        int oldItemColumns = m_itemEnd.column - m_itemStart.column + 1;
        for (ItemID item = intersectionStart; item.column <= intersectionEnd.column; ++item.column)
            for (item.row = intersectionStart.row; item.row <= intersectionEnd.row; ++item.row)
            {
                ItemID itemNew = item - newItemStart;
                ItemID itemOld = item - m_itemStart;
                QSharedPointer<CacheItem>& oldCacheItem = m_items[itemOld.row * oldItemColumns + itemOld.column];
                QSharedPointer<CacheItem>& newCacheItem = newItems[itemNew.row * newItemColumns + itemNew.column];
                newCacheItem.swap(oldCacheItem);
                newCacheItem->correctRectangles(m_scrollDelta);
            }
    }

    // initialize non-intersected cells
    QPoint origin = originPos();
    for (ItemID itemVisible = newItemStart; itemVisible.column <= newItemEnd.column; ++itemVisible.column)
    {
        for (itemVisible.row = newItemStart.row; itemVisible.row <= newItemEnd.row; ++itemVisible.row)
        {
            ItemID item = itemVisible - newItemStart;

            QSharedPointer<CacheItem>& cacheItem = newItems[item.row * newItemColumns + item.column];
            if (cacheItem)
                continue;

            cacheItem = QSharedPointer<CacheItem>::create(m_cacheItemsFactory->create(itemVisible));
            // correct rectangle
            cacheItem->rect.translate(origin);
        }
    }

    m_itemStart.swap(newItemStart);
    m_itemEnd.swap(newItemEnd);
    m_items.swap(newItems);

    // clear offset
    m_scrollDelta = QPoint(0, 0);
    m_sizeDelta = QSize(0, 0);
    // mark items as valid
    m_itemsCacheInvalid = false;
}

void CacheSpaceGrid::invalidateItemsCacheStructureImpl() const
{
    for (const auto& cacheItem : m_items)
    {
        cacheItem->invalidateCacheView();
    }
}

void CacheSpaceGrid::updateItemsCacheSchemaImpl() const
{
    for (const auto& cacheItem : m_items)
    {
        cacheItem->invalidateCacheView();
        m_cacheItemsFactory->updateSchema(*cacheItem);
    }
}

void CacheSpaceGrid::drawImpl(QPainter* painter, const GuiContext& ctx) const
{
    Q_ASSERT(!m_itemsCacheInvalid);

    for (const auto& cacheItem : m_items)
    {
        //if (drawContext.pDC->RectVisible(&cacheCell->cell.rect))
            cacheItem->draw(painter, ctx, &m_window);
    }
}

const CacheItem* CacheSpaceGrid::cacheItemImpl(const ItemID& visibleItem) const
{
    if (!isItemInFrame(visibleItem))
        return nullptr;

    int itemColumns = m_itemEnd.column - m_itemStart.column + 1;
    auto item = visibleItem - m_itemStart;
    return m_items[item.row * itemColumns + item.column].data();
}

const CacheItem* CacheSpaceGrid::cacheItemByPositionImpl(const QPoint& point) const
{
    if (m_itemsCacheInvalid)
        validateItemsCache();

    if (isEmpty())
        return nullptr;

    QPoint gridPoint = window2Space(point);

    ItemID visibleItem;
    visibleItem.row = m_grid->rows()->findVisibleIDByPos(gridPoint.y(), m_itemStart.row, m_itemEnd.row);
    visibleItem.column = m_grid->columns()->findVisibleIDByPos(gridPoint.x(), m_itemStart.column, m_itemEnd.column);

    if (!isItemInFrame(visibleItem))
        return nullptr;

    int itemColumns = m_itemEnd.column - m_itemStart.column + 1;
    visibleItem = visibleItem - m_itemStart;
    int index = visibleItem.row * itemColumns + visibleItem.column;
    Q_ASSERT(index < m_items.size());
    return m_items[index].data();
}

} // end namespace Qi
