#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "core/Layout.h"
#include "core/ext/Ranges.h"
#include "core/ext/ModelStore.h"
#include "items/checkbox/Check.h"
#include "items/radiobutton/Radio.h"
#include "items/button/Button.h"
#include "items/text/Text.h"
#include "items/selection/Selection.h"
#include "items/image/StyleStandardPixmap.h"
#include "items/image/Image.h"
#include "items/image/Pixmap.h"
#include "items/link/Link.h"

#include "cache/space/CacheSpaceGrid.h"

#include <QMessageBox>
#include <functional>

using namespace Qi;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    using namespace std::placeholders;

    ui->setupUi(this);

    auto fixedGrid = ui->gridWidget->subGrid(topLeftID);

    auto clientGrid = ui->gridWidget->subGrid();
    clientGrid->setDimensions(100, 100);
    clientGrid->rows()->setLineSizeAll(25);
    clientGrid->columns()->setLineSizeAll(100);

    auto topGrid = ui->gridWidget->subGrid(topID);
    topGrid->setRowsCount(2);
    topGrid->rows()->setLineSizeAll(25);

    auto leftGrid = ui->gridWidget->subGrid(leftID);
    leftGrid->setColumnsCount(2);
    leftGrid->columns()->setLineSizeAll(40);

    auto selection = QSharedPointer<ModelSelection>::create(clientGrid);
    selection->setActiveItem(ItemID(1, 1));
    selection->setSelection(QSharedPointer<RangeColumn>::create(3));

    clientGrid->addSchema(makeRangeAll(), QSharedPointer<ViewSelectionClient>::create(selection), makeLayoutBackground());
    topGrid->addSchema(makeRangeAll(), QSharedPointer<ViewSelectionHeader>::create(selection, SelectionRowsHeader), makeLayoutBackground());
    leftGrid->addSchema(makeRangeAll(), QSharedPointer<ViewSelectionHeader>::create(selection, SelectionColumnsHeader), makeLayoutBackground());
    fixedGrid->addSchema(makeRangeAll(), QSharedPointer<ViewSelectionHeader>::create(selection, SelectionCornerHeader), makeLayoutBackground());
    ui->gridWidget->setControllerKeyboard(QSharedPointer<ControllerKeyboardSelection>::create(selection, &ui->gridWidget->cacheSubGrid(clientID), ui->gridWidget));

    // text in all cells except column 5
    {
        auto modelText = QSharedPointer<ModelTextCallback>::create();
        modelText->getValueFunction = [](const ItemID& item)->QString {
            return QString("Item [%1, %2]").arg(item.row).arg(item.column);
        };
        auto range = QSharedPointer<RangeCallback>::create([](const ItemID& item)->bool{
            return item.column != 5;
        });
        clientGrid->addSchema(range, QSharedPointer<ViewText>::create(modelText));
    }

    auto modelChecks = QSharedPointer<ModelStorageValue<Qt::CheckState>>::create(Qt::Checked);
    clientGrid->addSchema(makeRangeColumn(0), QSharedPointer<ViewCheck>::create(modelChecks), makeLayoutLeft());

    auto modelRadio = QSharedPointer<ModelRadioStorage>::create();
    clientGrid->addSchema(makeRangeColumn(1), QSharedPointer<ViewRadio>::create(modelRadio), makeLayoutLeft());

    auto modelText = QSharedPointer<ModelTextCallback>::create();
    modelText->getValueFunction = [](const ItemID& item)->QString {
        return QString("Top Item [%1, %2]").arg(item.row).arg(item.column);
    };
    topGrid->addSchema(makeRangeAll(), QSharedPointer<ViewText>::create(modelText));

    modelText = QSharedPointer<ModelTextCallback>::create();
    modelText->getValueFunction = [](const ItemID& item)->QString {
        return QString("Left Item [%1, %2]").arg(item.row).arg(item.column);
    };
    leftGrid->addSchema(makeRangeAll(), QSharedPointer<ViewText>::create(modelText));

    modelRadio = QSharedPointer<ModelRadioStorage>::create(ItemID(0, 5));
    leftGrid->addSchema(makeRangeColumn(0), QSharedPointer<ViewRadio>::create(modelRadio), makeLayoutLeft());

    // Button with text example
    {
        auto modelBttnText = QSharedPointer<ModelStorageValue<QString>>::create(" ... ");
        auto viewBttnText  = QSharedPointer<ViewText>::create(modelBttnText);
        auto viewBttn = QSharedPointer<ViewButton>::create(viewBttnText);
        viewBttn->tuneBttnState = [](const ItemID& item, QStyle::State& bttnState) {
            if (item.row % 2 == 0)
                bttnState |= QStyle::State_Sunken;
        };
        viewBttn->action = [](const ItemID& item,  const ControllerContext& context, const ViewButton*) {
            QMessageBox::information(context.widget, "ViewButton clicked", QString("item[%1, %2]").arg(item.row).arg(item.column));
        };
        viewBttn->setTooltipText("Click me");
        clientGrid->addSchema(makeRangeColumn(2), viewBttn, makeLayoutRight());
    }

    // Standard Pixmap example
    {
        auto viewStdIcon = QSharedPointer<ViewStyleStandardPixmap>::create(QStyle::SP_DialogOkButton);
        viewStdIcon->action = [](const ItemID& item, const ControllerContext& context, const ViewStyleStandardPixmap*) {
            QMessageBox::information(context.widget, "ViewStyleStandardPixmap clicked", QString("item[%1, %2]").arg(item.row).arg(item.column));
        };
        clientGrid->addSchema(makeRangeColumn(3), viewStdIcon, makeLayoutRight());
    }

    // QImage example
    {
        m_images[0] = QImage(":/new/img/abort-icon.png");
        m_images[1] = QImage(":/new/img/application-icon.png");
        m_images[2] = QImage(":/new/img/information-icon.png");
        auto modelImage = QSharedPointer<ModelImageCallback>::create();
        modelImage->getValueFunction = std::bind(std::mem_fn(&MainWindow::image), this, _1);
        auto viewImage = QSharedPointer<ViewImage>::create(modelImage);
        clientGrid->addSchema(makeRangeColumn(4), viewImage, makeLayoutLeft());
    }

    // QPixmap example
    {
        m_pixmaps[0] = QPixmap(":/new/img/bubble-icon.png");
        m_pixmaps[1] = QPixmap(":/new/img/calendar-icon.png");
        m_pixmaps[2] = QPixmap(":/new/img/home-icon.png");
        auto modelPixmap = QSharedPointer<ModelPixmapCallback>::create();
        modelPixmap->getValueFunction = std::bind(std::mem_fn(&MainWindow::pixmap), this, _1);
        auto viewPixmap = QSharedPointer<ViewPixmap>::create(modelPixmap);
        viewPixmap->tooltipTextCallback = std::bind(std::mem_fn(&MainWindow::pixmapTooltip), this, _1, _2);
        clientGrid->addSchema(makeRangeColumn(4), viewPixmap, makeLayoutLeft());
    }

    // Link example
    {
        auto modelLink = QSharedPointer<ModelTextCallback>::create();
        modelLink->getValueFunction = [](const ItemID& item)->QString {
            return QString("Link [%1, %2]").arg(item.row).arg(item.column);
        };
        auto viewLink = QSharedPointer<ViewLink>::create(modelLink);
        viewLink->action = [](const ItemID& item, const ControllerContext& context, const ViewLink* viewLink) {
            QMessageBox::information(context.widget, "Link clicked",
                                     QString("%1 was clicked").arg(viewLink->theModel()->value(item)));
        };

        clientGrid->addSchema(makeRangeColumn(5), viewLink, makeLayoutLeft());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    close();
}

QImage MainWindow::image(const Qi::ItemID& item) const
{
    return m_images[item.row%3];
}

QPixmap MainWindow::pixmap(const Qi::ItemID& item) const
{
    return m_pixmaps[item.row%3];
}

bool MainWindow::pixmapTooltip(const Qi::ItemID& item, QString& text) const
{
    int index = item.row%3;
    switch (index)
    {
    case 0:
        text = "Bubble";
        return true;
    case 1:
        text = "Calendar";
        return true;
    case 2:
        text = "Home";
        return true;
    default:
        return false;
    }
}

