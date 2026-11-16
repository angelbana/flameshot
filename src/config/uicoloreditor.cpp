// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "uicoloreditor.h"
#include "utils/confighandler.h"
#include "utils/globalvalues.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace {
QIcon colorSwatch(const QColor& color)
{
    QPixmap swatch(18, 18);
    swatch.fill(color);
    return QIcon(swatch);
}
}

UIcolorEditor::UIcolorEditor(QWidget* parent)
  : QWidget(parent)
{
    m_hLayout = new QHBoxLayout(this);
    m_hLayout->setContentsMargins(16, 14, 16, 14);
    m_hLayout->setSpacing(20);
    m_vLayout = new QVBoxLayout();
    m_vLayout->setAlignment(Qt::AlignVCenter);
    m_vLayout->setSpacing(8);

    initButtons();
    initHexColorInput();
    initColorWheel();
    m_hLayout->addLayout(m_vLayout, 1);
    updateComponents();
}

void UIcolorEditor::updateComponents()
{
    ConfigHandler config;
    m_uiColor = config.uiColor();
    m_contrastColor = config.contrastUiColor();
    m_buttonMainColor->setIcon(colorSwatch(m_uiColor));
    m_buttonContrast->setIcon(colorSwatch(m_contrastColor));
    const QColor color =
      m_lastButtonPressed == m_buttonMainColor ? m_uiColor : m_contrastColor;
    const QSignalBlocker blocker(m_colorWheel);
    m_colorWheel->setColor(color);
    updateLocalColor(color);
}

void UIcolorEditor::updateUIcolor()
{
    ConfigHandler config;
    if (m_lastButtonPressed == m_buttonMainColor) {
        config.setUiColor(m_uiColor);
    } else {
        config.setContrastUiColor(m_contrastColor);
    }
}

void UIcolorEditor::updateLocalColor(const QColor color)
{
    if (m_lastButtonPressed == m_buttonMainColor) {
        m_uiColor = color;
    } else {
        m_contrastColor = color;
    }

    m_lastButtonPressed->setIcon(colorSwatch(color));
    const QSignalBlocker blocker(m_hexColorEdit);
    m_hexColorEdit->setText(color.name());
}

void UIcolorEditor::changeInputColor(const QString& hexColor)
{
    const QColor color(hexColor);
    if (!m_hexColorEdit->hasAcceptableInput() || !color.isValid()) {
        return;
    }
    const QSignalBlocker blocker(m_colorWheel);
    m_colorWheel->setColor(color);
    updateLocalColor(color);
    updateUIcolor();
}

void UIcolorEditor::initColorWheel()
{
    m_colorWheel = new color_widgets::ColorWheel(this);
    connect(m_colorWheel,
            &color_widgets::ColorWheel::colorSelected,
            this,
            &UIcolorEditor::updateUIcolor);
    connect(m_colorWheel,
            &color_widgets::ColorWheel::colorChanged,
            this,
            &UIcolorEditor::updateLocalColor);

    const int size = GlobalValues::buttonBaseSize() * 3;
    m_colorWheel->setMinimumSize(size, size);
    m_colorWheel->setMaximumSize(size * 2, size * 2);
    m_colorWheel->setToolTip(tr("Change the color moving the selectors and see"
                                " the changes in the preview buttons."));
    m_hLayout->addWidget(m_colorWheel, 0, Qt::AlignVCenter);
}

void UIcolorEditor::initButtons()
{
    m_buttonMainColor = new QPushButton(tr("Main Color"), this);
    m_buttonContrast = new QPushButton(tr("Contrast Color"), this);
    for (auto* button : { m_buttonMainColor, m_buttonContrast }) {
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setMinimumHeight(fontMetrics().height() + 12);
        m_vLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, button]() {
            changeLastButton(button);
        });
    }
    m_lastButtonPressed = m_buttonMainColor;
    m_buttonMainColor->setChecked(true);
}

void UIcolorEditor::initHexColorInput()
{
    m_hexColorEdit = new QLineEdit(this);
    m_hexColorLabel = new QLabel(tr("Hex for Main Color"), this);
    m_hexColorLabel->setBuddy(m_hexColorEdit);
    connect(m_hexColorEdit,
            &QLineEdit::textEdited,
            this,
            &UIcolorEditor::changeInputColor);
    const QRegularExpression rgbRegex("^#[0-9A-Fa-f]{6}$");
    m_hexColorEdit->setValidator(
      new QRegularExpressionValidator(rgbRegex, m_hexColorEdit));
    m_hexColorEdit->setMaxLength(7);
    m_vLayout->addWidget(m_hexColorLabel);
    m_vLayout->addWidget(m_hexColorEdit);
}

void UIcolorEditor::changeLastButton(QPushButton* button)
{
    if (m_lastButtonPressed == button) {
        return;
    }
    m_lastButtonPressed = button;
    m_hexColorLabel->setText(button == m_buttonMainColor
                               ? tr("Hex for Main Color")
                               : tr("Hex for Contrast Color"));
    updateComponents();
}
