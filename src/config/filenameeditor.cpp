// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "filenameeditor.h"
#include "config/strftimechooserwidget.h"
#include "utils/confighandler.h"
#include "utils/filenamehandler.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

FileNameEditor::FileNameEditor(QWidget* parent)
  : QWidget(parent)
{
    initWidgets();
    initLayout();
}

void FileNameEditor::initLayout()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(14);

    auto* pattern = new QGroupBox(tr("Filename pattern"), this);
    auto* fields = new QVBoxLayout(pattern);
    fields->setSpacing(8);
    m_nameEditor->setAccessibleName(pattern->title());
    fields->addWidget(m_nameEditor);
    auto* preview = new QLabel(tr("Preview:"), pattern);
    preview->setBuddy(m_outputLabel);
    fields->addWidget(preview);
    fields->addWidget(m_outputLabel);

    auto* horizLayout = new QHBoxLayout();
    horizLayout->addStretch();
    horizLayout->addWidget(m_resetButton);
    horizLayout->addWidget(m_clearButton);
    horizLayout->addWidget(m_saveButton);
    fields->addLayout(horizLayout);
    m_layout->addWidget(pattern);

    auto* dateAndTime = new QGroupBox(tr("Insert date and time"), this);
    auto* tokens = new QVBoxLayout(dateAndTime);
    tokens->addWidget(m_helperButtons);
    m_layout->addWidget(dateAndTime);
    m_layout->addStretch();
}

void FileNameEditor::initWidgets()
{
    m_nameHandler = new FileNameHandler(this);

    // editor
    m_nameEditor = new QLineEdit(this);
    m_nameEditor->setMaxLength(FileNameHandler::MAX_CHARACTERS);

    // preview
    m_outputLabel = new QLineEdit(this);
    m_outputLabel->setReadOnly(true);

    connect(m_nameEditor,
            &QLineEdit::textChanged,
            this,
            &FileNameEditor::showParsedPattern);
    updateComponents();

    // helper buttons
    m_helperButtons = new StrftimeChooserWidget(this);
    connect(m_helperButtons,
            &StrftimeChooserWidget::variableEmitted,
            this,
            &FileNameEditor::addToNameEditor);

    // save
    m_saveButton = new QPushButton(tr("Save"), this);
    connect(
      m_saveButton, &QPushButton::clicked, this, &FileNameEditor::savePattern);
    m_saveButton->setToolTip(tr("Saves the pattern"));
    // restore previous saved values
    m_resetButton = new QPushButton(tr("Restore"), this);
    connect(
      m_resetButton, &QPushButton::clicked, this, &FileNameEditor::resetName);
    m_resetButton->setToolTip(tr("Restores the saved pattern"));
    // clear
    m_clearButton = new QPushButton(tr("Clear"), this);
    connect(m_clearButton, &QPushButton::clicked, this, [this]() {
        m_nameEditor->setText(ConfigHandler().filenamePatternDefault());
        m_nameEditor->selectAll();
        m_nameEditor->setFocus();
    });
    m_clearButton->setToolTip(tr("Deletes the name"));
}

void FileNameEditor::savePattern()
{
    QString pattern = m_nameEditor->text();
    ConfigHandler().setFilenamePattern(pattern);
}

void FileNameEditor::showParsedPattern(const QString& p)
{
    QString output = m_nameHandler->parseFilename(p);
    m_outputLabel->setText(output);
}

void FileNameEditor::resetName()
{
    m_nameEditor->setText(ConfigHandler().filenamePattern());
}

void FileNameEditor::addToNameEditor(const QString& s)
{
    m_nameEditor->setText(m_nameEditor->text() + s);
    m_nameEditor->setFocus();
}

void FileNameEditor::updateComponents()
{
    m_nameEditor->setText(ConfigHandler().filenamePattern());
    m_outputLabel->setText(m_nameHandler->parsedPattern());
}
