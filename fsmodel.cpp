#include <QLocale>

#include "fsmodel.hpp"

namespace model {
  ExtendedFileSystemModel::ExtendedFileSystemModel(QObject *parent)
  : QFileSystemModel(parent) {}

  ExtendedFileSystemModel::~ExtendedFileSystemModel() {}

  QVariant ExtendedFileSystemModel::data(const QModelIndex &index, int role) const {
    if(role == Qt::DisplayRole && index.column() == 1) {
      QString path = filePath(index);
      QFileInfo info(path);

      if (info.isDir()) {
        if (!size_cache.contains(path)) {
          qint64 size = size_cache[path];

          return size >= 0 ? QLocale().formattedDataSize(size) : QString("-");
        }

        return QString("fart");
      } else 
        return QFileSystemModel::data(index, role);
    }

    return QFileSystemModel::data(index, role);
  }

  // this function we attach to the button to recalculate the dir size
  void ExtendedFileSystemModel::calculateSize(const QModelIndex &index) {
    if (!index.isValid())
      return;

    QString path = filePath(index);
    QFileInfo info(path);

    if (info.isDir()) {
      qint64 size = dirSize(path);
      size_cache[path] = size;

      // emitting the size changed signal for the view
      QModelIndex size_index = index.sibling(index.row(), 1);
      emit dataChanged(size_index, size_index, {Qt::DisplayRole});
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
} // namespace model

