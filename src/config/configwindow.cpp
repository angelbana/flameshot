// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "configwindow.h"
#include "config/configresolver.h"
#include "config/filenameeditor.h"
#include "config/generalconf.h"
#include "config/shortcutswidget.h"
#include "config/visualseditor.h"
#include "utils/colorutils.h"
#include "utils/confighandler.h"
#include "utils/globalvalues.h"
#include "utils/pathinfo.h"

#include <QApplication>
#include <QCursor>
#include <QFrame>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollArea>
#include <QShortcut>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

ConfigWindow::ConfigWindow(QWidget* parent)
  : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(QIcon(GlobalValues::iconPath()));
    setWindowTitle(tr("Configuration"));

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_sidebar = new QFrame(this);
    auto* sidebar = m_sidebar;
    sidebar->setObjectName("settingsSidebar");
    auto* sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(12, 20, 12, 16);
    m_navigation = new QListWidget(sidebar);
    auto* navigation = m_navigation;
    navigation->setObjectName("settingsNavigation");
    navigation->setAccessibleName(tr("Settings sections"));
    navigation->setFrameShape(QFrame::NoFrame);
    navigation->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigation->setIconSize(QSize(20, 20));
    navigation->setSpacing(3);
    navigation->setStyleSheet(
      "#settingsNavigation { background: transparent; }"
      "#settingsNavigation::item { border: 1px solid transparent;"
      " padding: 8px 10px; border-radius: 7px; }"
      "#settingsNavigation::item:hover { background: palette(midlight); }"
      "#settingsNavigation::item:selected { background: palette(highlight);"
      " color: palette(highlighted-text); }");
    sidebar->setStyleSheet(
      "#settingsSidebar { background: palette(alternate-base); }");
    sidebarLayout->addWidget(navigation);
    m_layout->addWidget(sidebar);

    auto* pages = new QStackedWidget(this);
    pages->setObjectName("settingsPages");
    m_layout->addWidget(pages, 1);

    int sideWidth = 0;
    const auto addPage = [&](QWidget* editor,
                             const QString& title,
                             const QString& summary,
                             const QString& iconName,
                             bool scroll = false) {
        auto* item = new QListWidgetItem(title, navigation);
        item->setToolTip(title);
        item->setData(Qt::UserRole, iconName);
        const int textWidth =
          navigation->fontMetrics().horizontalAdvance(title);
        sideWidth = qMax(sideWidth, textWidth);
        m_navigationRowWidth += textWidth + 58;

        auto* page = new QWidget(pages);
        auto* pageLayout = new QVBoxLayout(page);
        pageLayout->setContentsMargins(24, 20, 20, 16);
        pageLayout->setSpacing(8);

        auto* heading = new QLabel(title, page);
        QFont font = heading->font();
        font.setPointSizeF(font.pointSizeF() * 1.65);
        font.setWeight(QFont::DemiBold);
        heading->setFont(font);
        heading->setWordWrap(true);
        pageLayout->addWidget(heading);

        auto* description = new QLabel(summary, page);
        description->setWordWrap(true);
        pageLayout->addWidget(description);
        pageLayout->addSpacing(4);
        if (scroll) {
            auto* scroller = new QScrollArea(page);
            scroller->setFrameShape(QFrame::NoFrame);
            scroller->setWidgetResizable(true);
            scroller->setWidget(editor);
            pageLayout->addWidget(scroller, 1);
        } else {
            pageLayout->addWidget(editor, 1);
        }

        initErrorIndicator(page, editor);
        pages->addWidget(page);
    };

    auto* general = new GeneralConf(pages);
    addPage(general,
            tr("General"),
            tr("Save paths, capture behavior, and clipboard."),
            QStringLiteral("config.svg"));
    auto* visuals = new VisualsEditor(pages);
    addPage(visuals,
            tr("Interface"),
            tr("Language, colors, and capture tools."),
            QStringLiteral("graphics.svg"),
            true);
    auto* filenames = new FileNameEditor(pages);
    addPage(filenames,
            tr("Filename Editor"),
            tr("Build a filename pattern and preview the result."),
            QStringLiteral("name_edition.svg"),
            true);
    auto* shortcuts = new ShortcutsWidget(pages);
    addPage(shortcuts,
            tr("Shortcuts"),
            tr("View and change keyboard shortcuts."),
            QStringLiteral("shortcut.svg"),
            true);

    m_sidebarWidth = qBound(170, sideWidth + 90, 248);
    sidebar->setFixedWidth(m_sidebarWidth);
    updateNavigationIcons();
    connect(navigation,
            &QListWidget::currentRowChanged,
            pages,
            &QStackedWidget::setCurrentIndex);
    navigation->setCurrentRow(0);

    const auto cycle = [navigation](int step) {
        const int count = navigation->count();
        navigation->setCurrentRow((navigation->currentRow() + step + count) %
                                  count);
    };
    new QShortcut(QKeySequence::NextChild, this, [cycle]() { cycle(1); });
    new QShortcut(QKeySequence::PreviousChild, this, [cycle]() { cycle(-1); });

    connect(ConfigHandler::getInstance(),
            &ConfigHandler::fileChanged,
            this,
            &ConfigWindow::updateChildren);
    connect(this,
            &ConfigWindow::updateChildren,
            general,
            &GeneralConf::updateComponents);
    connect(this,
            &ConfigWindow::updateChildren,
            visuals,
            &VisualsEditor::updateComponents);
    connect(this,
            &ConfigWindow::updateChildren,
            filenames,
            &FileNameEditor::updateComponents);

    const QSize preferred(880, 660);
    if (const QScreen* screen = QGuiApplication::screenAt(QCursor::pos())) {
        resize(preferred.boundedTo(screen->availableGeometry().size() -
                                   QSize(32, 48)));
    } else {
        resize(preferred);
    }
}

void ConfigWindow::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        close();
    } else {
        QWidget::keyPressEvent(e);
    }
}

void ConfigWindow::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::PaletteChange && m_sidebarWidth != 0) {
        QTimer::singleShot(0, this, &ConfigWindow::updateNavigationIcons);
    }
}

void ConfigWindow::updateNavigationIcons()
{
    const auto iconPath = [](const QColor& textColor) {
        return ColorUtils::colorIsDark(textColor) ? PathInfo::blackIconPath()
                                                  : PathInfo::whiteIconPath();
    };
    const auto palette = m_navigation->palette();
    const QString normalIcons = iconPath(palette.text().color());
    const QString selectedIcons = iconPath(palette.highlightedText().color());
    for (int i = 0; i < m_navigation->count(); ++i) {
        auto* item = m_navigation->item(i);
        const QString name = item->data(Qt::UserRole).toString();
        QIcon icon(normalIcons + name);
        icon.addFile(selectedIcons + name, QSize(), QIcon::Selected);
        item->setIcon(icon);
    }
}

void ConfigWindow::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    if (m_sidebarWidth == 0 || m_navigationLayoutPending) {
        return;
    }
    m_navigationLayoutPending = true;
    QTimer::singleShot(0, this, [this]() {
        updateNavigationLayout();
        m_navigationLayoutPending = false;
    });
}

void ConfigWindow::updateNavigationLayout()
{
    const bool compact = width() < m_sidebarWidth + 510;
    const auto direction =
      compact ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight;
    if (direction != m_layout->direction()) {
        m_layout->setDirection(direction);
        m_sidebar->setMinimumWidth(compact ? 0 : m_sidebarWidth);
        m_sidebar->setMaximumWidth(compact ? QWIDGETSIZE_MAX : m_sidebarWidth);
        m_sidebar->layout()->setContentsMargins(
          compact ? QMargins(10, 5, 10, 5) : QMargins(12, 20, 12, 16));
        m_navigation->setFlow(compact ? QListView::LeftToRight
                                      : QListView::TopToBottom);
        m_navigation->setHorizontalScrollBarPolicy(
          compact ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
        m_navigation->setVerticalScrollBarPolicy(
          compact ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
        if (!compact) {
            m_navigation->setMinimumHeight(0);
            m_navigation->setMaximumHeight(QWIDGETSIZE_MAX);
        }
    }
    if (compact) {
        const int scrollHeight =
          width() < m_navigationRowWidth + 24
            ? style()->pixelMetric(QStyle::PM_ScrollBarExtent)
            : 0;
        m_navigation->setFixedHeight(m_navigation->sizeHintForRow(0) +
                                     2 * m_navigation->spacing() +
                                     scrollHeight);
    }
}

void ConfigWindow::initErrorIndicator(QWidget* page, QWidget* editor)
{
    auto* label = new QLabel(tr("<b>Configuration file has errors. "
                                "Resolve them before continuing.</b>"),
                             page);
    label->setWordWrap(true);
    auto* resolve = new QPushButton(tr("Resolve"), page);

    auto* layout = static_cast<QVBoxLayout*>(page->layout());
    layout->insertWidget(layout->count() - 1, label);
    layout->insertWidget(layout->count() - 1, resolve, 0, Qt::AlignLeft);

    const auto showError = [editor, label, resolve](bool error) {
        label->setVisible(error);
        resolve->setVisible(error);
        editor->setEnabled(!error);
    };
    showError(ConfigHandler().hasError());
    connect(ConfigHandler::getInstance(),
            &ConfigHandler::error,
            editor,
            [showError]() { showError(true); });
    connect(ConfigHandler::getInstance(),
            &ConfigHandler::errorResolved,
            editor,
            [showError]() { showError(false); });
    connect(
      resolve, &QPushButton::clicked, this, []() { ConfigResolver().exec(); });
}
