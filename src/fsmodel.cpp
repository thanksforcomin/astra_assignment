#include <cstdio>
#include <iostream>

#include <functional>

#include <QLocale>
#include <QDebug>
#include <QFutureWatcher>
#include <QtConcurrent>

#include "fsmodel.hpp"

namespace model {
  ExtendedFileSystemModel::ExtendedFileSystemModel(QObject *parent)
  : QFileSystemModel(parent) {
    connect(this, &QFileSystemModel::rootPathChanged, 
            this, &ExtendedFileSystemModel::onRootPathChanged);
  }

  QVariant ExtendedFileSystemModel::data(const QModelIndex &index, int role) const {
    if(role == Qt::DisplayRole && index.column() == SIZE_COLUMN) {
      QString path = QDir::cleanPath(filePath(index));
      QFileInfo info(path);

      if (info.isDir()) {
        
        if (size_cache.contains(path)) {
          qint64 size = size_cache[path];

          return size >= 0 ? QLocale().formattedDataSize(size) : QString("...");
        }

        // fallback
        return QString("-");
      } else 
        return QFileSystemModel::data(index, role);
    }

    return QFileSystemModel::data(index, role);
  }

  // this function we attach to the button to recalculate the dir size
  // the use cache flag is kind of a last minute fix but it works well
  // in cases where we need to calculate it's size the first time and 
  // just use the cache later, like the first time we view a directory
  void ExtendedFileSystemModel::calculateSize(const QModelIndex &index, bool use_cache) {
    if (!index.isValid() || !isDir(index)) 
      return;

    QString path = QDir::cleanPath(filePath(index));
    
    if(use_cache && size_cache.contains(path))
      return;
    
    qDebug() << "Calculating size for" << path;

    size_cache[path] = PENDING_CALCULATION;
    QModelIndex size_index = index.sibling(index.row(), SIZE_COLUMN);
    emit dataChanged(size_index, size_index, {Qt::DisplayRole});

    dirSizeAsync(path);
  }

  void ExtendedFileSystemModel::calculateSizeRecursive(const QModelIndex &index) {
    if (!index.isValid() || !isDir(index))
      return;

    // doing actual calculation
    calculateSize(index, true);

    auto path = filePath(index);
    QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot);

    // the function is actually not recursive at all! but
    // I am too lazy to change it's name. We only do the
    // directories that are already viewable
    while (it.hasNext()) {
      auto curr_index = it.next();
      calculateSize(this->index(curr_index), true);
    }
  }

  void ExtendedFileSystemModel::onRootPathChanged(const QString &path) {
    QModelIndex rootIdx = index(path);
    
    if (rootIdx.isValid()) {
      // automatically trigger on-demand calculation for the new root tree
      calculateSizeRecursive(rootIdx);
    }
  }

  qint64 ExtendedFileSystemModel::dirSize(const QString &path) const {
    qint64 size {0};

    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
      it.next();
      size += it.fileInfo().size();
    }

    return size;
  }

  // the point of this function is to calculate directory sizes in background
  // threads and update them on arrival
  void ExtendedFileSystemModel::dirSizeAsync(const QString& path) {
    auto watcher = new QFutureWatcher<qint64>(this);
    
    // when we finish calculating we update the size cache and emit the signal
    QObject::connect(watcher, &QFutureWatcher<qint64>::finished, this,
                     [this, watcher, path]() {
                       qint64 size = watcher->result();
                       size_cache[path] = size;

                       // I actually decided to calculate the update index here
                       // and not capture it because of potential of index 
                       // invalidation and it's overall better to just recalculate
                       // it
                       QModelIndex index = this->index(path);
                       
                       if(index.isValid()) {
                         QModelIndex size_index = index.sibling(index.row(), SIZE_COLUMN);
                         
                         emit dataChanged(size_index, size_index, {Qt::DisplayRole});
                         watcher->deleteLater();
                       }
                       
                     });

    // here we actually count our stuff
    // I decided to make it some kind of async to save time for large
    // directories but I don't really know how good it works
    QFuture<qint64> future = QtConcurrent::run(
                                               std::bind(&ExtendedFileSystemModel::dirSize, this, path));
    
    
    watcher->setFuture(future);
  }
} // namespace model

