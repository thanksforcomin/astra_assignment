/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <iostream>

#include <QApplication>
#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QScreen>
#include <QScroller>
#include <QTreeView>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QHeaderView>

#include "fsmodel.hpp"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  
  QCoreApplication::setApplicationVersion(QT_VERSION_STR);
  QCommandLineParser parser;
  parser.setApplicationDescription("Qt Dir View Example");
  parser.addHelpOption();
  parser.addVersionOption();
  QCommandLineOption dontUseCustomDirectoryIconsOption("c", "Set QFileSystemModel::DontUseCustomDirectoryIcons");
  parser.addOption(dontUseCustomDirectoryIconsOption);
  QCommandLineOption dontWatchOption("w", "Set QFileSystemModel::DontWatch");
  parser.addOption(dontWatchOption);
  parser.addPositionalArgument("directory", "The directory to start in.");
  parser.process(app);
  
  const QString root_path = parser.positionalArguments().isEmpty()
                           ? QDir::homePath()
                           : parser.positionalArguments().first();

  // creating the central widget
  auto central = new QWidget;
  central->setWindowTitle(QObject::tr("Dir View"));
  
  // actual model and proxy models
  auto model = new model::ExtendedFileSystemModel(central);
  model->setRootPath(root_path);
  model->setFilter(model->filter() | QDir::Hidden); 
  
  if (parser.isSet(dontUseCustomDirectoryIconsOption))
    model->setOption(QFileSystemModel::DontUseCustomDirectoryIcons);
  if (parser.isSet(dontWatchOption))
    model->setOption(QFileSystemModel::DontWatchForChanges);

  auto proxy_model = new QSortFilterProxyModel(central);
  proxy_model->setSourceModel(model);
  proxy_model->setFilterKeyColumn(0);
  proxy_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
  proxy_model->setRecursiveFilteringEnabled(true);
  
  auto tree = new QTreeView(central);
  tree->setModel(proxy_model);

  QPersistentModelIndex root_index;
  
  if (!root_path.isEmpty())
    root_index = model->index(QDir::cleanPath(root_path));
  else 
    root_index = model->index(model->rootPath());
  
  if (root_index.isValid()) {
    QPersistentModelIndex proxy_root_index = proxy_model->mapFromSource(root_index);
    tree->setRootIndex(proxy_root_index);
  }
  
  // some styling
  tree->setAnimated(false);
  tree->setIndentation(20);
  tree->setSortingEnabled(true);
  const auto availableSize = central->screen()->availableGeometry().size();

  QHeaderView *header = tree->header();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
  header->setStretchLastSection(false);

  QObject::connect(tree, &QTreeView::expanded, [model, proxy_model](const QModelIndex &proxyIndex) {
    QModelIndex sourceIndex = proxy_model->mapToSource(proxyIndex);
    model->calculateSizeRecursive(sourceIndex);
  });
  
  // touch support
  QScroller::grabGesture(tree, QScroller::TouchGesture);
  
  // the dir size update button
  auto update_button = new QPushButton("Update", central);
  update_button->setEnabled(false);
  update_button->setToolTip("Select a directory and press the button to update it's size");

  // when we select something the button lights up
  QObject::connect(tree->selectionModel(),
                   &QItemSelectionModel::selectionChanged,
                   [update_button, tree]() {
                     bool hasSelection = !tree->selectionModel()->selectedRows().isEmpty();

                     // interestingly the update button does nothing to
                     // regular files, they're handled by the filesystem
                     // watcher
                     update_button->setEnabled(hasSelection);
                   });
  
  // when we press the button we call the function that raises the dataChanged
  // signal
  QObject::connect(
      update_button, &QPushButton::clicked, [proxy_model, model, tree]() {
        QModelIndexList selected = tree->selectionModel()->selectedRows(0);

        for (auto &index : selected) {
          QModelIndex source_index = proxy_model->mapToSource(index);
          if (source_index.isValid())
            model->calculateSize(source_index);
        }
  });

  // the search bar
  auto filter_bar = new QLineEdit(central);
  filter_bar->setPlaceholderText("Filter files");

  // this is hacky but this is actually a pretty nice solution.
  // writing a complete persistent root proxy is very tedious
  // (https://github.com/VSRonin/QtModelUtilities/)
  // and the updates happen at most only a couple times per second,
  // and if the root is in place it actually does nothing,
  // so I'd say it's hacky and poor but actually a very sound
  // solution
  QObject::connect(filter_bar, &QLineEdit::textChanged,
                   [proxy_model, tree, root_index](const QString &text) {
                     proxy_model->setFilterWildcard("*" + text + "*");
                     
                     if(root_index.isValid()) {
                       QPersistentModelIndex proxy_root_index = proxy_model->mapFromSource(root_index);
                       
                       // because it might not be valid (not be on the filter)
                       if(proxy_root_index.isValid())
                         tree->setRootIndex(proxy_root_index);
                     }
                   });
  
  // top layout
  auto top_layout = new QHBoxLayout;
  top_layout->addWidget(filter_bar);
  top_layout->addWidget(update_button);
  
  // final layout
  auto layout = new QVBoxLayout(central);
  layout->addLayout(top_layout);
  layout->addWidget(tree);

  central->resize(availableSize / 2);
  central->show();
  
  return app.exec();
}
