/*
   Copyright (c) 2008-1015 Alex Zhondin <qtinuum.team@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "ViewCacheSpace.h"
#include "space/CacheSpace.h"

namespace Qi
{

ViewCacheSpace::ViewCacheSpace(const SharedPtr<ModelCacheSpace> &model, bool useController)
    : m_model(model)
{
    if (useController)
        setController(makeShared<ControllerMouseCacheSpace>(model));

    connect(m_model.data(), &Model::modelChanged, this, &ViewCacheSpace::onModelChanged);
}

ViewCacheSpace::~ViewCacheSpace()
{
    disconnect(m_model.data(), &Model::modelChanged, this, &ViewCacheSpace::onModelChanged);
}

CacheView2* ViewCacheSpace::addCacheViewImpl(const Layout& layout, const GuiContext& ctx, ID id, QVector<CacheView2>& cacheViews, QRect& itemRect, QRect* visibleItemRect) const
{
    CacheView2* result = View::addCacheViewImpl(layout, ctx, id, cacheViews, itemRect, visibleItemRect);

    if (!result)
        return result;

    const auto& cacheSpace = m_model->value(id);
    if (!cacheSpace)
        return result;

    // update cache window to the Rect which will be visible
    cacheSpace->setWindow(result->rect());

    return result;
}

QSize ViewCacheSpace::sizeImpl(const GuiContext& /*ctx*/, ID id, ViewSizeMode /*sizeMode*/) const
{
    const auto& cacheSpace = m_model->value(id);

    if (cacheSpace)
        return cacheSpace->space().size();
    else
        return QSize(0, 0);
}

void ViewCacheSpace::drawImpl(QPainter* painter, const GuiContext& ctx, const CacheContext& cache, bool* /*showTooltip*/) const
{
    const auto& cacheSpace = m_model->value(cache.id);

    if (!cacheSpace)
        return;

    cacheSpace->draw(painter, ctx);
}

bool ViewCacheSpace::tooltipByPointImpl(QPoint point, ID item, TooltipInfo& tooltipInfo) const
{
    const auto& cacheSpace = m_model->value(item);

    if (!cacheSpace)
        return false;

    return cacheSpace->tooltipByPoint(point, tooltipInfo);
}

void ViewCacheSpace::onModelChanged(const Model*)
{
    emitViewChanged(ChangeReasonViewContent);
}

ControllerMouseCacheSpace::ControllerMouseCacheSpace(SharedPtr<ModelCacheSpace> model)
    : m_model(std::move(model))
{
}

void ControllerMouseCacheSpace::tryActivateImpl(QVector<ControllerMouse*>& activatedControllers, const ActivationInfo& activationInfo)
{
    const auto& cacheSpace = m_model->value(activationInfo.cache.id);

    if (!cacheSpace)
        return;

    cacheSpace->tryActivateControllers(activationInfo.context, activatedControllers);
}

} // end namespace Qi
