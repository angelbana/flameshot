// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include "capturebutton.h"
#include "tools/capturetool.h"

#include <QMap>
#include <QVector>

class QWidget;
class CaptureToolButton : public CaptureButton
{
    Q_OBJECT

public:
    enum class Group
    {
        Annotation,
        History,
        Output
    };

    explicit CaptureToolButton(const CaptureTool::Type,
                               QWidget* parent = nullptr);
    ~CaptureToolButton();

    static const QList<CaptureTool::Type>& getIterableButtonTypes();
    static int getPriorityByButton(CaptureTool::Type);

    QString name() const;
    QString description() const;
    QIcon icon() const;
    CaptureTool* tool() const;
    Group group() const;

    void setColor(const QColor& c);
    void setSelected(bool selected, const QColor& color = {});

protected:
    bool event(QEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void nextCheckState() override;
    static QList<CaptureTool::Type> iterableButtonTypes;

    CaptureTool* m_tool;

signals:
    void pressedButtonLeftClick(CaptureToolButton*);
    void pressedButtonRightClick(CaptureToolButton*);

private:
    CaptureToolButton(QWidget* parent = nullptr);
    CaptureTool::Type m_buttonType;

    QColor m_color;
    QColor m_selectedColor;

    void initButton();
    void updateAppearance();
};
