// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "visualseditor.h"
#include "config/buttonlistview.h"
#include "config/colorpickereditor.h"
#include "config/extendedslider.h"
#include "config/uicoloreditor.h"
#include "utils/confighandler.h"

#include <QCompleter>
#include <QDirIterator>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>

VisualsEditor::VisualsEditor(QWidget* parent)
  : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(14);
    initWidgets();
}

void VisualsEditor::updateComponents()
{
    m_buttonList->updateComponents();
    m_colorEditor->updateComponents();
    refreshOpacity();
}

void VisualsEditor::initOpacitySlider(QVBoxLayout* layout)
{
    m_opacitySlider = new ExtendedSlider();
    m_opacitySlider->setOrientation(Qt::Horizontal);
    m_opacitySlider->setRange(0, 100);
    auto* localLayout = new QHBoxLayout();
    localLayout->addWidget(new QLabel(QStringLiteral("0%")));
    localLayout->addWidget(m_opacitySlider);
    localLayout->addWidget(new QLabel(QStringLiteral("100%")));

    m_opacityLabel = new QLabel();
    m_opacityLabel->setBuddy(m_opacitySlider);
    connect(m_opacitySlider, &ExtendedSlider::valueChanged, this, [this]() {
        updateOpacityLabel();
        ConfigHandler().setContrastOpacity(
          m_opacitySlider->mappedValue(0, 255));
    });
    layout->addWidget(m_opacityLabel);
    layout->addLayout(localLayout);
    refreshOpacity();
}

void VisualsEditor::refreshOpacity()
{
    const QSignalBlocker blocker(m_opacitySlider);
    m_opacitySlider->setMapedValue(0, ConfigHandler().contrastOpacity(), 255);
    updateOpacityLabel();
}

void VisualsEditor::updateOpacityLabel()
{
    m_opacityLabel->setText(
      (tr("Opacity of area outside selection:") + QStringLiteral(" %1%"))
        .arg(m_opacitySlider->value()));
}

void VisualsEditor::initWidgets()
{
    auto* appearance = new QGroupBox(tr("Language and appearance"), this);
    auto* appearanceLayout = new QVBoxLayout(appearance);
    appearanceLayout->setSpacing(8);
    initTranslations(appearanceLayout);
    initOpacitySlider(appearanceLayout);
    m_layout->addWidget(appearance);

    m_tabWidget = new QTabWidget();
    m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_layout->addWidget(m_tabWidget);

    m_colorEditor = new UIcolorEditor();
    m_colorEditorTab = new QWidget();
    auto* colorEditorLayout = new QVBoxLayout(m_colorEditorTab);
    m_colorEditorTab->setLayout(colorEditorLayout);
    colorEditorLayout->addWidget(m_colorEditor);
    m_tabWidget->addTab(m_colorEditorTab, tr("UI Color Editor"));

    m_colorpickerEditor = new ColorPickerEditor();
    m_colorpickerEditorTab = new QWidget();
    auto* colorpickerEditorLayout = new QVBoxLayout(m_colorpickerEditorTab);
    colorpickerEditorLayout->addWidget(m_colorpickerEditor);
    m_tabWidget->addTab(m_colorpickerEditorTab, tr("Colorpicker Editor"));

    auto* boxButtons = new QGroupBox();
    boxButtons->setTitle(tr("Button Selection"));
    auto* listLayout = new QVBoxLayout(boxButtons);
    m_buttonList = new ButtonListView();
    m_layout->addWidget(boxButtons, 1);
    listLayout->addWidget(m_buttonList);

    auto* setAllButtons = new QPushButton(tr("Select All"));
    connect(setAllButtons,
            &QPushButton::clicked,
            m_buttonList,
            &ButtonListView::selectAll);
    listLayout->addWidget(setAllButtons, 0, Qt::AlignRight);
}

void VisualsEditor::initTranslations(QVBoxLayout* layout)
{
    auto* localLayout = new QHBoxLayout();
    auto* label = new QLabel(tr("UI language"));
    localLayout->addWidget(label);
    m_selectTranslation = new QComboBox(this);
    label->setBuddy(m_selectTranslation);
    m_selectTranslation->setMinimumContentsLength(12);
    m_selectTranslation->setSizeAdjustPolicy(
      QComboBox::AdjustToMinimumContentsLengthWithIcon);
    // The translation list is long enough to overflow the screen as a
    // plain dropdown, so make it editable with a filtering completer -
    // typing narrows the popup instead of showing every item at once.
    m_selectTranslation->setEditable(true);
    m_selectTranslation->setInsertPolicy(QComboBox::NoInsert);
    m_selectTranslation->completer()->setCompletionMode(
      QCompleter::PopupCompletion);
    m_selectTranslation->completer()->setFilterMode(Qt::MatchContains);
    m_selectTranslation->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    m_selectTranslation->setMaxVisibleItems(12);

    QStringList translations;
    QString tmpFilename;
    for (const QString& path : PathInfo::translationsPaths()) {
        QDirIterator it(path,
                        QStringList() << QStringLiteral("*.qm"),
                        QDir::NoDotAndDotDot | QDir::Files);
        while (it.hasNext()) {
            it.next();
            tmpFilename = it.fileName();

            if (tmpFilename.startsWith(
                  QStringLiteral("Internationalization_"))) {
                tmpFilename =
                  tmpFilename.remove(QStringLiteral("Internationalization_"))
                    .remove(QStringLiteral(".qm"));
                if (!translations.contains(tmpFilename)) {
                    translations << tmpFilename;
                }
            }
        }
    }
    translations.sort();
    translations.push_front(QStringLiteral("auto"));
    m_selectTranslation->addItems(translations);

    QString language = ConfigHandler().value("uiLanguage").toString();
    m_selectTranslation->setCurrentIndex(
      m_selectTranslation->findText(language));

    // textActivated only fires once the user has actually picked an item
    // (popup click, or Enter on a completed match) - unlike
    // currentTextChanged, which the now-editable/filterable combobox would
    // otherwise emit on every keystroke while typing to search.
    connect(m_selectTranslation,
            &QComboBox::textActivated,
            this,
            [this](const QString& text) {
                ConfigHandler().setUiLanguage(text);
                // TODO: Retranslate UI without restart
                QMessageBox::information(
                  this,
                  tr("Configuration"),
                  tr("Flameshot must be restarted to apply these changes!"));
            });

    localLayout->addWidget(m_selectTranslation);
    localLayout->addStretch();
    layout->addLayout(localLayout);
}
