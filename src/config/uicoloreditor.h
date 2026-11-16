// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include "QtColorWidgets/color_wheel.hpp"
#include <QWidget>

class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QLineEdit;
class QLabel;

class UIcolorEditor : public QWidget
{
    Q_OBJECT
public:
    explicit UIcolorEditor(QWidget* parent = nullptr);

public slots:
    void updateComponents();

private slots:
    void updateUIcolor();
    void updateLocalColor(const QColor);
    void changeLastButton(QPushButton*);
    void changeInputColor(const QString&);

private:
    QColor m_uiColor, m_contrastColor;
    QPushButton* m_buttonMainColor;
    QPushButton* m_buttonContrast;
    QPushButton* m_lastButtonPressed;
    color_widgets::ColorWheel* m_colorWheel;
    QLineEdit* m_hexColorEdit;
    QLabel* m_hexColorLabel;

    QHBoxLayout* m_hLayout;
    QVBoxLayout* m_vLayout;

    void initColorWheel();
    void initButtons();
    void initHexColorInput();
};
