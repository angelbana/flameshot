// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "buttonhandler.h"
#include "utils/globalvalues.h"

#include <QPainter>
#include <QPoint>
#include <QScreen>

class ButtonHandler::Backdrop : public QWidget
{
public:
    explicit Backdrop(QWidget* parent)
      : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void setGroups(const QVector<QRect>& groups)
    {
        m_groups = groups;
        QRect bounds;
        for (const QRect& group : m_groups) {
            bounds = bounds.united(group);
        }
        setGeometry(bounds);
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor("#171B27"));
        painter.setPen(QColor("#50576B"));
        painter.translate(-pos());
        for (const QRect& group : m_groups) {
            const QRectF shape = QRectF(group).adjusted(0.5, 0.5, -0.5, -0.5);
            painter.drawRoundedRect(shape, 9, 9);
        }
    }

private:
    QVector<QRect> m_groups;
};

ButtonHandler::ButtonHandler(const QVector<CaptureToolButton*>& v,
                             QObject* parent)
  : QObject(parent)
{
    init();
    setButtons(v);
}

ButtonHandler::ButtonHandler(QObject* parent)
  : QObject(parent)
{
    init();
}

void ButtonHandler::hide()
{
    if (m_backdrop) {
        m_backdrop->hide();
    }
    for (CaptureToolButton* b : m_vectorButtons) {
        b->hide();
    }
}

void ButtonHandler::show()
{
    if (m_vectorButtons.isEmpty() || m_vectorButtons.first()->isVisible()) {
        return;
    }
    m_backdrop->show();
    for (CaptureToolButton* b : m_vectorButtons) {
        b->show();
    }
}

bool ButtonHandler::isVisible() const
{
    bool ret = true;
    for (const CaptureToolButton* b : m_vectorButtons) {
        if (!b->isVisible()) {
            ret = false;
            break;
        }
    }
    return ret;
}

bool ButtonHandler::buttonsAreInside() const
{
    return m_buttonsAreInside;
}

size_t ButtonHandler::size() const
{
    return m_vectorButtons.size();
}

// updatePosition updates the position of the buttons around the
// selection area. Ignores the sides blocked by the end of the screen.
// When the selection is too small, it works on a virtual selection with
// the original in the center.
void ButtonHandler::updatePosition(const QRect& selection)
{
    resetRegionTrack();
    m_buttonGroups.clear();
    const int vecLength = m_vectorButtons.size();
    if (vecLength == 0) {
        return;
    }
    // Copy of the selection area for internal modifications
    m_selection = intersectWithAreas(selection);
    updateBlockedSides();
    ensureSelectionMinimumSize();
    // Indicates the actual button to be moved
    int elemIndicator = 0;

    while (elemIndicator < vecLength) {

        // Add them inside the area when there is no more space
        if (m_allSidesBlocked) {
            m_selection = selection;
            positionButtonsInside(elemIndicator);
            break; // the while
        }
        // Number of buttons per row column
        int buttonsPerRow =
          (m_selection.width() + m_separator) / (m_buttonExtendedSize);
        int buttonsPerCol =
          (m_selection.height() + m_separator) / (m_buttonExtendedSize);
        // Buttons to be placed in the corners
        int extraButtons =
          (vecLength - elemIndicator) - (buttonsPerRow + buttonsPerCol) * 2;
        int elemsAtCorners = extraButtons > 4 ? 4 : extraButtons;
        int maxExtra = 2;
        if (m_oneHorizontalBlocked) {
            maxExtra = 1;
        } else if (m_horizontalyBlocked) {
            maxExtra = 0;
        }
        int elemCornersTop = qBound(0, elemsAtCorners, maxExtra);
        elemsAtCorners -= elemCornersTop;
        int elemCornersBotton = qBound(0, elemsAtCorners, maxExtra);

        // Add buttons at the button of the selection
        if (!m_blockedBotton) {
            int addCounter = buttonsPerRow + elemCornersBotton;
            // Don't add more than we have
            addCounter = qBound(0, addCounter, vecLength - elemIndicator);
            QPoint center = QPoint(m_selection.center().x(),
                                   m_selection.bottom() + m_separator);
            if (addCounter > buttonsPerRow) {
                adjustHorizontalCenter(center, addCounter);
            }
            // ElemIndicator, elemsAtCorners
            QVector<QPoint> positions =
              horizontalPoints(center, addCounter, true);
            moveButtonsToPoints(positions, elemIndicator);
        }
        // Add buttons to the right side of the selection
        if (!m_blockedRight && elemIndicator < vecLength) {
            int addCounter = buttonsPerCol;
            addCounter = qBound(0, addCounter, vecLength - elemIndicator);

            QPoint center = QPoint(m_selection.right() + m_separator,
                                   m_selection.center().y());
            QVector<QPoint> positions =
              verticalPoints(center, addCounter, false);
            moveButtonsToPoints(positions, elemIndicator);
        }
        // Add buttons at the top of the selection
        if (!m_blockedTop && elemIndicator < vecLength) {
            int addCounter = buttonsPerRow + elemCornersTop;
            addCounter = qBound(0, addCounter, vecLength - elemIndicator);
            QPoint center = QPoint(m_selection.center().x(),
                                   m_selection.top() - m_buttonExtendedSize);
            if (addCounter == 1 + buttonsPerRow) {
                adjustHorizontalCenter(center, addCounter);
            }
            QVector<QPoint> positions =
              horizontalPoints(center, addCounter, false);
            moveButtonsToPoints(positions, elemIndicator);
        }
        // Add buttons to the left side of the selection
        if (!m_blockedLeft && elemIndicator < vecLength) {
            int addCounter = buttonsPerCol;
            addCounter = qBound(0, addCounter, vecLength - elemIndicator);

            QPoint center = QPoint(m_selection.left() - m_buttonExtendedSize,
                                   m_selection.center().y());
            QVector<QPoint> positions =
              verticalPoints(center, addCounter, true);
            moveButtonsToPoints(positions, elemIndicator);
        }
        // If there are elements for the next cycle, increase the size of the
        // base area
        if (elemIndicator < vecLength && !(m_allSidesBlocked)) {
            expandSelection();
        }
        updateBlockedSides();
    }
    m_backdrop->setGroups(m_buttonGroups);
}

int ButtonHandler::calculateShift(int elements, bool reverse) const
{
    const int span = elements * m_buttonExtendedSize - m_separator;
    const int leftOffset = (span - 1) / 2;
    return reverse ? leftOffset : span - leftOffset - m_buttonBaseSize;
}
// horizontalPoints is an auxiliary method for the button position computation.
// starts from a known center and keeps adding elements horizontally
// and returns the computed positions.
QVector<QPoint> ButtonHandler::horizontalPoints(const QPoint& center,
                                                const int elements,
                                                const bool leftToRight) const
{
    QVector<QPoint> res;
    // Distance from the center to start adding buttons
    int shift = calculateShift(elements, leftToRight);

    int x = leftToRight ? center.x() - shift : center.x() + shift;
    QPoint i(x, center.y());
    while (elements > res.length()) {
        res.append(i);
        leftToRight ? i.setX(i.x() + m_buttonExtendedSize)
                    : i.setX(i.x() - m_buttonExtendedSize);
    }
    return res;
}

// verticalPoints is an auxiliary method for the button position computation.
// starts from a known center and keeps adding elements vertically
// and returns the computed positions.
QVector<QPoint> ButtonHandler::verticalPoints(const QPoint& center,
                                              const int elements,
                                              const bool upToDown) const
{
    QVector<QPoint> res;
    // Distance from the center to start adding buttons
    int shift = calculateShift(elements, upToDown);

    int y = upToDown ? center.y() - shift : center.y() + shift;
    QPoint i(center.x(), y);
    while (elements > res.length()) {
        res.append(i);
        upToDown ? i.setY(i.y() + m_buttonExtendedSize)
                 : i.setY(i.y() - m_buttonExtendedSize);
    }
    return res;
}

QRect ButtonHandler::intersectWithAreas(const QRect& rect)
{
    QRect res;
    for (const QRect& r : m_screenRegions) {
        QRect temp = rect.intersected(r);
        if (temp.height() * temp.width() > res.height() * res.width()) {
            res = temp;
        }
    }
    return res;
}

void ButtonHandler::init()
{
    m_buttonBaseSize = GlobalValues::buttonBaseSize();
    m_separator = m_buttonBaseSize / 4;
    m_buttonExtendedSize = m_buttonBaseSize + m_separator;
}

void ButtonHandler::resetRegionTrack()
{
    m_buttonsAreInside = false;
}

void ButtonHandler::updateBlockedSides()
{
    QRegion screenRegion{};
    for (const QRect& rect : m_screenRegions) {
        screenRegion += rect;
    }

    const int EXTENSION = m_separator * 2 + m_buttonBaseSize;
    // Right
    QPoint pointA(m_selection.right() + EXTENSION, m_selection.bottom());
    QPoint pointB(pointA.x(), m_selection.top());
    m_blockedRight =
      !(screenRegion.contains(pointA) && screenRegion.contains(pointB));
    // Left
    pointA.setX(m_selection.left() - EXTENSION);
    pointB.setX(pointA.x());
    m_blockedLeft =
      !(screenRegion.contains(pointA) && screenRegion.contains(pointB));
    // Bottom
    pointA = QPoint(m_selection.left(), m_selection.bottom() + EXTENSION);
    pointB = QPoint(m_selection.right(), pointA.y());
    m_blockedBotton =
      !(screenRegion.contains(pointA) && screenRegion.contains(pointB));
    // Top
    pointA.setY(m_selection.top() - EXTENSION);
    pointB.setY(pointA.y());
    m_blockedTop =
      !(screenRegion.contains(pointA) && screenRegion.contains(pointB));
    // Auxiliary
    m_oneHorizontalBlocked =
      (!m_blockedRight && m_blockedLeft) || (m_blockedRight && !m_blockedLeft);
    m_horizontalyBlocked = (m_blockedRight && m_blockedLeft);
    m_allSidesBlocked =
      (m_blockedBotton && m_horizontalyBlocked && m_blockedTop);
}

void ButtonHandler::expandSelection()
{
    int& s = m_buttonExtendedSize;
    m_selection = m_selection + QMargins(s, s, s, s);
    m_selection = intersectWithAreas(m_selection);
}

void ButtonHandler::positionButtonsInside(int index)
{
    // Position the buttons in the botton-center of the main but inside of the
    // selection.
    QRect mainArea = m_selection;
    mainArea = intersectWithAreas(mainArea);
    const int buttonsPerRow = (mainArea.width()) / (m_buttonExtendedSize);
    if (buttonsPerRow == 0) {
        return;
    }
    QPoint center =
      QPoint(mainArea.center().x(), mainArea.bottom() - m_buttonExtendedSize);

    while (m_vectorButtons.size() > index) {
        int addCounter = buttonsPerRow;
        addCounter = qBound(0, addCounter, m_vectorButtons.size() - index);
        QVector<QPoint> positions = horizontalPoints(center, addCounter, true);
        moveButtonsToPoints(positions, index);
        center.setY(center.y() - m_buttonExtendedSize);
    }

    m_buttonsAreInside = true;
}

void ButtonHandler::ensureSelectionMinimumSize()
{
    // Detect if a side is smaller than a button in order to prevent collision
    // and redimension the base area to the base size of a single button per
    // side
    if (m_selection.width() < m_buttonBaseSize) {
        const int missing = m_buttonBaseSize - m_selection.width();
        if (!m_blockedLeft) {
            m_selection.moveLeft(m_selection.left() -
                                 (m_blockedRight ? missing : missing / 2));
        }
        m_selection.setWidth(m_buttonBaseSize);
    }
    if (m_selection.height() < m_buttonBaseSize) {
        const int missing = m_buttonBaseSize - m_selection.height();
        if (!m_blockedTop) {
            m_selection.moveTop(m_selection.top() -
                                (m_blockedBotton ? missing : missing / 2));
        }
        m_selection.setHeight(m_buttonBaseSize);
    }
}

void ButtonHandler::moveButtonsToPoints(const QVector<QPoint>& points,
                                        int& index)
{
    const int start = index;
    const int padding = qMax(1, m_separator / 3);
    for (const QPoint& p : points) {
        auto* button = m_vectorButtons[index];
        button->move(p);
        const QRect background =
          button->geometry().adjusted(-padding, -padding, padding, padding);
        if (index == start ||
            button->group() != m_vectorButtons[index - 1]->group()) {
            m_buttonGroups.append(background);
        } else {
            m_buttonGroups.last() = m_buttonGroups.last().united(background);
        }
        ++index;
    }
}

void ButtonHandler::adjustHorizontalCenter(QPoint& center, int elements) const
{
    const int leftOffset = calculateShift(elements, true);
    const int rightOffset =
      calculateShift(elements, false) + m_buttonBaseSize - 1;
    if (m_blockedLeft) {
        center.setX(qMax(center.x(), m_selection.left() + leftOffset));
    } else if (m_blockedRight) {
        center.setX(qMin(center.x(), m_selection.right() - rightOffset));
    }
}

// setButtons redefines the buttons of the button handler
void ButtonHandler::setButtons(const QVector<CaptureToolButton*>& v)
{
    hide();

    for (CaptureToolButton* b : m_vectorButtons) {
        delete (b);
    }
    m_vectorButtons = v;
    m_buttonGroups.clear();
    if (!m_vectorButtons.isEmpty()) {
        if (!m_backdrop) {
            m_backdrop = new Backdrop(v.first()->parentWidget());
        }
        m_backdrop->stackUnder(v.first());
    }
}

bool ButtonHandler::contains(const QPoint& p) const
{
    for (const QRect& group : m_buttonGroups) {
        if (group.contains(p)) {
            return true;
        }
    }
    return false;
}

void ButtonHandler::updateScreenRegions(const QVector<QRect>& rects)
{
    m_screenRegions = rects;
}

void ButtonHandler::updateScreenRegions(const QRect& rect)
{
    m_screenRegions = { rect };
}
