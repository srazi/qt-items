#include "Layouts.h"

namespace Qi
{

QSharedPointer<Layout> makeLayoutBackground()
{
    static QSharedPointer<Layout> layout = QSharedPointer<LayoutClient>::create(LayoutBehaviorTransparent);
    return layout;
}

QSharedPointer<Layout> makeLayoutClient()
{
    static QSharedPointer<Layout> layout = QSharedPointer<LayoutClient>::create(LayoutBehaviorNone);
    return layout;
}

QSharedPointer<Layout> makeLayoutCenter()
{
    static QSharedPointer<Layout> layout = QSharedPointer<LayoutCenter>::create(LayoutBehaviorNone);
    return layout;
}

QSharedPointer<Layout> makeLayoutLeft(LayoutBehaviorMask behavior)
{
    if (behavior == LayoutBehaviorNone)
    {
        static QSharedPointer<Layout> layout = QSharedPointer<LayoutLeft>::create(LayoutBehaviorNone);
        return layout;
    }

    return QSharedPointer<LayoutLeft>::create(behavior);
}

QSharedPointer<Layout> makeLayoutRight(LayoutBehaviorMask behavior)
{
    if (behavior == LayoutBehaviorNone)
    {
        static QSharedPointer<Layout> layout = QSharedPointer<LayoutRight>::create(LayoutBehaviorNone);
        return layout;
    }

    return QSharedPointer<LayoutRight>::create(behavior);
}

QSharedPointer<Layout> makeLayoutTop(LayoutBehaviorMask behavior)
{
    if (behavior == LayoutBehaviorNone)
    {
        static QSharedPointer<Layout> layout = QSharedPointer<LayoutTop>::create(LayoutBehaviorNone);
        return layout;
    }

    return QSharedPointer<LayoutTop>::create(behavior);
}

QSharedPointer<Layout> makeLayoutBottom(LayoutBehaviorMask behavior)
{
    if (behavior == LayoutBehaviorNone)
    {
        static QSharedPointer<Layout> layout = QSharedPointer<LayoutBottom>::create(LayoutBehaviorNone);
        return layout;
    }

    return QSharedPointer<LayoutBottom>::create(behavior);
}

bool LayoutCenter::doLayoutImpl(const ViewInfo& viewInfo, LayoutInfo& info) const
{
    info.viewRect = info.itemRect;

    QSize viewSize = viewInfo.size();

    int deltaHeight = (info.itemRect.height() - viewSize.height()) / 2;
    if (deltaHeight > 0)
    {
        info.viewRect.setTop(info.viewRect.top() + deltaHeight);
        info.viewRect.setBottom(info.viewRect.bottom() - deltaHeight);
    }

    int deltaWidth = (info.itemRect.width() - viewSize.width()) / 2;
    if (deltaWidth > 0)
    {
        info.viewRect.setLeft(info.viewRect.left() + deltaWidth);
        info.viewRect.setRight(info.viewRect.right() - deltaWidth);
    }

    return true;
}

void LayoutCenter::expandSizeImpl(const ViewInfo& viewInfo, QSize& size) const
{
    QSize viewSize = viewInfo.size();
    size.rwidth() = qMax(size.width(), viewSize.width());
    size.rheight() = qMax(size.height(), viewSize.height());
}

void LayoutHor::expandSizeImpl(const ViewInfo& viewInfo, QSize& size) const
{
    QSize viewSize = viewInfo.size();
    size.rwidth() += (viewSize.width()+1);
    size.rheight() = qMax(size.height(), viewSize.height());
}

void LayoutVer::expandSizeImpl(const ViewInfo& viewInfo, QSize& size) const
{
    QSize viewSize = viewInfo.size();
    size.rwidth() = qMax(size.width(), viewSize.width());
    size.rheight() += (viewSize.height()+1);
}

bool LayoutClient::doLayoutImpl(const ViewInfo& /*viewInfo*/, LayoutInfo& info) const
{
    info.viewRect = info.itemRect;
    if (!isTransparent())
        info.itemRect.setLeft(info.viewRect.right()+1);

    return true;
}

void LayoutClient::expandSizeImpl(const ViewInfo& viewInfo, QSize& size) const
{
    QSize viewSize = viewInfo.size();
    size.rwidth() += (viewSize.width()+1);
    size.rheight() += (viewSize.height()+1);
}

bool LayoutLeft::doLayoutImpl(const ViewInfo& viewInfo, LayoutInfo& info) const
{
    QSize viewSize = viewInfo.size();

    info.viewRect = info.itemRect;
    info.viewRect.setRight(qMin(info.viewRect.right(), info.viewRect.left() + viewSize.width()));
    if (!isTransparent())
        info.itemRect.setLeft(info.viewRect.right()+1);

    return true;
}

bool LayoutRight::doLayoutImpl(const ViewInfo& viewInfo, LayoutInfo& info) const
{
    QSize viewSize = viewInfo.size();

    info.viewRect = info.itemRect;
    info.viewRect.setLeft(qMax(info.viewRect.left(), info.viewRect.right() - viewSize.width()));
    if (!isTransparent())
        info.itemRect.setRight(info.viewRect.left()-1);

    return true;
}

bool LayoutTop::doLayoutImpl(const ViewInfo& viewInfo, LayoutInfo& info) const
{
    QSize viewSize = viewInfo.size();

    info.viewRect = info.itemRect;
    info.viewRect.setBottom(qMin(info.viewRect.bottom(), info.viewRect.top() + viewSize.height()));
    if (!isTransparent())
        info.itemRect.setTop(info.viewRect.bottom()+1);

    return true;
}

bool LayoutBottom::doLayoutImpl(const ViewInfo& viewInfo, LayoutInfo& info) const
{
    QSize viewSize = viewInfo.size();

    info.viewRect = info.itemRect;
    info.viewRect.setTop(qMax(info.viewRect.top(), info.viewRect.bottom() - viewSize.height()));
    if (!isTransparent())
        info.itemRect.setBottom(info.viewRect.top()-1);

    return true;
}

} // end namespace Qi
