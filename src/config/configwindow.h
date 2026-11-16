// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include <QWidget>

class QBoxLayout;
class QFrame;
class QListWidget;

class ConfigWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigWindow(QWidget* parent = nullptr);

signals:
    void updateChildren();

protected:
    void changeEvent(QEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    QBoxLayout* m_layout;
    QFrame* m_sidebar;
    QListWidget* m_navigation;
    int m_sidebarWidth = 0;
    int m_navigationRowWidth = 0;
    bool m_navigationLayoutPending = false;

    void updateNavigationLayout();
    void updateNavigationIcons();
    void initErrorIndicator(QWidget* page, QWidget* editor);
};
