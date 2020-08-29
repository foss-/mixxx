// wlibrarytableview.cpp
// Created 10/19/2009 by RJ Ryan (rryan@mit.edu)

#include <QHeaderView>
#include <QPalette>
#include <QScrollBar>
#include <QFontMetrics>

#include "library/trackmodel.h"
#include "widget/wwidget.h"
#include "widget/wskincolor.h"
#include "widget/wlibrarytableview.h"
#include "util/math.h"

WLibraryTableView::WLibraryTableView(QWidget* parent,
                                     UserSettingsPointer pConfig,
                                     ConfigKey vScrollBarPosKey)
        : QTableView(parent),
          m_pConfig(pConfig),
          m_vScrollBarPosKey(vScrollBarPosKey) {

    loadVScrollBarPosState();

    // Setup properties for table

    // Editing starts when clicking on an already selected item.
    setEditTriggers(QAbstractItemView::SelectedClicked|QAbstractItemView::EditKeyPressed);

    //Enable selection by rows and extended selection (ctrl/shift click)
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    setWordWrap(false);
    setShowGrid(false);
    setCornerButtonEnabled(false);
    setSortingEnabled(true);
    // Used by delegates (e.g. StarDelegate) to tell when the mouse enters a
    // cell.
    setMouseTracking(true);
    //Work around a Qt bug that lets you make your columns so wide you
    //can't reach the divider to make them small again.
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    verticalHeader()->hide();
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setAlternatingRowColors(true);

    connect(verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SIGNAL(scrollValueChanged(int)));

    setTabKeyNavigation(false);
}

WLibraryTableView::~WLibraryTableView() {
}

void WLibraryTableView::loadVScrollBarPosState() {
    // TODO(rryan) I'm not sure I understand the value in saving the v-scrollbar
    // position across restarts of Mixxx. Now that we have different views for
    // each mode, the views should just maintain their scrollbar position when
    // you switch views. We should discuss this.
    m_noSearchVScrollBarPos = m_pConfig->getValueString(m_vScrollBarPosKey).toInt();
}

void WLibraryTableView::restoreNoSearchVScrollBarPos() {
    // Restore the scrollbar's position (scroll to that spot)
    // when the search has been cleared
    //qDebug() << "restoreNoSearchVScrollBarPos()" << m_noSearchVScrollBarPos;
    updateGeometries();
    verticalScrollBar()->setValue(m_noSearchVScrollBarPos);
}

void WLibraryTableView::saveNoSearchVScrollBarPos() {
    // Save the scrollbar's position so we can return here after
    // a search is cleared.
    //qDebug() << "saveNoSearchVScrollBarPos()" << m_noSearchVScrollBarPos;
    m_noSearchVScrollBarPos = verticalScrollBar()->value();
}


void WLibraryTableView::saveVScrollBarPosState() {
    //Save the vertical scrollbar position.
    int scrollbarPosition = verticalScrollBar()->value();
    m_pConfig->set(m_vScrollBarPosKey, ConfigValue(scrollbarPosition));
}

void WLibraryTableView::moveSelection(int delta) {
    QAbstractItemModel* pModel = model();

    if (pModel == nullptr) {
        return;
    }

    while(delta != 0) {
        // TODO(rryan) what happens if there is nothing selected?
        QModelIndex current = currentIndex();
        if(delta > 0) {
            // i is positive, so we want to move the highlight down
            int row = current.row();
            if (row + 1 < pModel->rowCount()) {
                selectRow(row + 1);
            }

            delta--;
        } else {
            // i is negative, so we want to move the highlight up
            int row = current.row();
            if (row - 1 >= 0) {
                selectRow(row - 1);
            }

            delta++;
        }
    }
}

void WLibraryTableView::saveVScrollBarPos(TrackModel* key){
    m_vScrollBarPosValues[key] = verticalScrollBar()->value();
}

void WLibraryTableView::restoreVScrollBarPos(TrackModel* key){
    updateGeometries();

    if (m_vScrollBarPosValues.contains(key)){
        verticalScrollBar()->setValue(m_vScrollBarPosValues[key]);
    }else{
        m_vScrollBarPosValues[key] = 0;
        verticalScrollBar()->setValue(0);
    }
}

void WLibraryTableView::setTrackTableFont(const QFont& font) {
    setFont(font);
    setTrackTableRowHeight(verticalHeader()->defaultSectionSize());
}

void WLibraryTableView::setTrackTableRowHeight(int rowHeight) {
    QFontMetrics metrics(font());
    int fontHeightPx = metrics.height();
    verticalHeader()->setDefaultSectionSize(math_max(
                                                rowHeight, fontHeightPx));
}

void WLibraryTableView::setSelectedClick(bool enable) {
    if (enable) {
        setEditTriggers(QAbstractItemView::SelectedClicked|QAbstractItemView::EditKeyPressed);
    } else {
        setEditTriggers(QAbstractItemView::EditKeyPressed);
    }
}

bool WLibraryTableView::event(QEvent* e) {
    // On FocusIn, make sure there is always a track selected which can then
    // instantly be loaded to a deck.
    // Benefits: with no previous selection the first item is automatically
    // selected without having to move down and up again first with up/down
    // button presses sent by keyboard or emulated by [Library],MoveVertical.
    // Second benefit: if the table has only one track, previously one could not
    // select it with [Library],MoveVertical. fixes lp:1808632
    if (e->type() == QEvent::FocusIn && model()->rowCount() > 0) {
        int currRow = currentIndex().row();
        if (currRow == -1) {
            selectRow(0);
        // This creates a (single) selection from the previously focused item in
        // case a single table item was deselected previously.
        } else if (currRow != -1 && selectionModel()->selectedRows().isEmpty()) {
            selectRow(currRow);
        }
    }

    return QTableView::event(e);
}
