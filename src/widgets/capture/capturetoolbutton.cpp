// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "capturetoolbutton.h"
#include "tools/capturetool.h"
#include "tools/toolfactory.h"
#include "utils/confighandler.h"
#include "utils/globalvalues.h"

#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>

namespace {
QColor tint(const QColor& base, const QColor& accent, qreal amount)
{
    return QColor::fromRgbF(
      base.redF() + (accent.redF() - base.redF()) * amount,
      base.greenF() + (accent.greenF() - base.greenF()) * amount,
      base.blueF() + (accent.blueF() - base.blueF()) * amount);
}
}

CaptureToolButton::CaptureToolButton(const CaptureTool::Type t, QWidget* parent)
  : CaptureButton(parent)
  , m_buttonType(t)
  , m_tool(nullptr)
{
    initButton();
    updateAppearance();
}

CaptureToolButton::~CaptureToolButton()
{
    if (m_tool) {
        delete m_tool;
        m_tool = nullptr;
    }
}

void CaptureToolButton::initButton()
{
    if (m_tool) {
        delete m_tool;
        m_tool = nullptr;
    }
    m_tool = ToolFactory().CreateTool(m_buttonType, this);

    const int size = GlobalValues::buttonBaseSize();
    setFixedSize(size, size);
    setIconSize(QSize(size, size) * 0.56);
    setGraphicsEffect(nullptr);
    setFocusPolicy(Qt::TabFocus);
    setAutoDefault(false);
    setCheckable(m_tool->isSelectable());
    connect(this, &QPushButton::clicked, this, [this]() {
        emit pressedButtonLeftClick(this);
    });

    // Set a tooltip showing a shortcut in parentheses (if there is a shortcut)
    const QString description = m_tool->description();
    setAccessibleName(m_tool->name());
    setAccessibleDescription(description);
    QString shortcut =
      ConfigHandler().shortcut(QVariant::fromValue(m_buttonType).toString());
    shortcut.replace("Return", "Enter");
    if (m_buttonType == CaptureTool::TYPE_COPY &&
        ConfigHandler().copyOnDoubleClick()) {
        const QString doubleClick = tr("Left double-click");
        shortcut = shortcut.isEmpty()
                     ? doubleClick
                     : tr("%1 or %2").arg(shortcut, doubleClick);
    }
    setToolTip(shortcut.isEmpty()
                 ? description
                 : QStringLiteral("%1 (%2)").arg(description, shortcut));
}

void CaptureToolButton::updateAppearance()
{
    const QColor surface("#242838");
    const QColor accent = m_color.isValid() ? m_color : QColor("#875BC1");
    const bool primary = m_buttonType == CaptureTool::TYPE_COPY ||
                         m_buttonType == CaptureTool::TYPE_SAVE ||
                         m_buttonType == CaptureTool::TYPE_ACCEPT;
    const QColor selectedAccent =
      m_selectedColor.isValid() ? m_selectedColor : accent;
    const QColor background = isChecked() ? tint(surface, selectedAccent, 0.52)
                              : primary   ? tint(surface, accent, 0.40)
                                          : surface;
    const QColor border = isChecked() ? tint(selectedAccent, Qt::white, 0.43)
                          : primary   ? tint(accent, Qt::white, 0.19)
                                      : surface;
    setStyleSheet(
      QStringLiteral("CaptureToolButton { border: 1px solid %1; "
                     "border-radius: %2px; padding: 0; background: %3; }"
                     "CaptureToolButton:hover { background: %4; }"
                     "CaptureToolButton:pressed { background: %5; }"
                     "CaptureToolButton:focus { border-color: #F0E5FF; }")
        .arg(border.name(),
             QString::number(width() / 5),
             background.name(),
             tint(background, Qt::white, 0.13).name(),
             tint(background, Qt::black, 0.22).name()));
    setIcon(m_tool->icon(background, true));
}

const QList<CaptureTool::Type>& CaptureToolButton::getIterableButtonTypes()
{
    return iterableButtonTypes;
}

// get icon returns the icon for the type of button
QIcon CaptureToolButton::icon() const
{
    return QPushButton::icon();
}

bool CaptureToolButton::event(QEvent* e)
{
    if (e->type() == QEvent::ShortcutOverride && hasFocus()) {
        const auto* key = static_cast<QKeyEvent*>(e);
        if (key->key() == Qt::Key_Space && key->modifiers() == Qt::NoModifier) {
            e->accept();
            return true;
        }
    }
    return CaptureButton::event(e);
}

void CaptureToolButton::mousePressEvent(QMouseEvent* e)
{
    activateWindow();
    if (e->button() == Qt::LeftButton) {
        emit pressedButtonLeftClick(this);
        emit pressed();
    } else if (e->button() == Qt::RightButton) {
        emit pressedButtonRightClick(this);
        emit pressed();
    }
}

void CaptureToolButton::nextCheckState()
{
    // CaptureWidget owns the selection for mouse and keyboard activation.
}

CaptureTool* CaptureToolButton::tool() const
{
    return m_tool;
}

void CaptureToolButton::setColor(const QColor& c)
{
    m_color = c;
    updateAppearance();
}

void CaptureToolButton::setSelected(bool selected, const QColor& color)
{
    m_selectedColor = color;
    setChecked(selected);
    updateAppearance();
}

CaptureToolButton::Group CaptureToolButton::group() const
{
    switch (m_buttonType) {
        case CaptureTool::TYPE_UNDO:
        case CaptureTool::TYPE_REDO:
            return Group::History;
        case CaptureTool::TYPE_COPY:
        case CaptureTool::TYPE_SAVE:
#ifdef ENABLE_IMGUR
        case CaptureTool::TYPE_IMAGEUPLOADER:
#endif
        case CaptureTool::TYPE_ACCEPT:
        case CaptureTool::TYPE_OPEN_APP:
        case CaptureTool::TYPE_EXIT:
        case CaptureTool::TYPE_PIN:
            return Group::Output;
        default:
            return Group::Annotation;
    }
}

static std::map<CaptureTool::Type, int> buttonTypeOrder
{
    { CaptureTool::TYPE_PENCIL, 0 }, { CaptureTool::TYPE_DRAWER, 1 },
      { CaptureTool::TYPE_ARROW, 2 }, { CaptureTool::TYPE_SELECTION, 3 },
      { CaptureTool::TYPE_RECTANGLE, 4 }, { CaptureTool::TYPE_CIRCLE, 5 },
      { CaptureTool::TYPE_MARKER, 6 }, { CaptureTool::TYPE_TEXT, 7 },
      { CaptureTool::TYPE_PIXELATE, 8 }, { CaptureTool::TYPE_INVERT, 9 },
      { CaptureTool::TYPE_CIRCLECOUNT, 10 },
      { CaptureTool::TYPE_MOVESELECTION, 12 }, { CaptureTool::TYPE_UNDO, 13 },
      { CaptureTool::TYPE_REDO, 14 }, { CaptureTool::TYPE_COPY, 15 },
      { CaptureTool::TYPE_SAVE, 16 },
#ifdef ENABLE_IMGUR
      { CaptureTool::TYPE_IMAGEUPLOADER, 17 },
#endif
      { CaptureTool::TYPE_ACCEPT, 18 },
#if !defined(Q_OS_MACOS)
      { CaptureTool::TYPE_OPEN_APP, 19 }, { CaptureTool::TYPE_EXIT, 20 },
      { CaptureTool::TYPE_PIN, 21 },
#else
      { CaptureTool::TYPE_EXIT, 19 }, { CaptureTool::TYPE_PIN, 20 },
#endif

      { CaptureTool::TYPE_SIZEINCREASE, 22 },
      { CaptureTool::TYPE_SIZEDECREASE, 23 },
};

int CaptureToolButton::getPriorityByButton(CaptureTool::Type b)
{
    auto it = buttonTypeOrder.find(b);
    return it == buttonTypeOrder.cend() ? (int)buttonTypeOrder.size()
                                        : it->second;
}

QList<CaptureTool::Type> CaptureToolButton::iterableButtonTypes = {
    CaptureTool::TYPE_PENCIL,        CaptureTool::TYPE_DRAWER,
    CaptureTool::TYPE_ARROW,         CaptureTool::TYPE_SELECTION,
    CaptureTool::TYPE_RECTANGLE,     CaptureTool::TYPE_CIRCLE,
    CaptureTool::TYPE_MARKER,        CaptureTool::TYPE_TEXT,
    CaptureTool::TYPE_CIRCLECOUNT,   CaptureTool::TYPE_PIXELATE,
    CaptureTool::TYPE_INVERT,        CaptureTool::TYPE_MOVESELECTION,
    CaptureTool::TYPE_SIZEINCREASE,  CaptureTool::TYPE_SIZEDECREASE,
    CaptureTool::TYPE_UNDO,          CaptureTool::TYPE_REDO,
    CaptureTool::TYPE_COPY,          CaptureTool::TYPE_SAVE,
    CaptureTool::TYPE_ACCEPT,
#ifdef ENABLE_IMGUR
    CaptureTool::TYPE_IMAGEUPLOADER,
#endif
#if !defined(Q_OS_MACOS)
    CaptureTool::TYPE_OPEN_APP,
#endif
    CaptureTool::TYPE_PIN,           CaptureTool::TYPE_EXIT,
};
