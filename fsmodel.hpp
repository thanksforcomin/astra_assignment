#pragma once

#include <QFileSystemModel>
#include <QMap>

namespace model {
  class ExtendedFileSystemModel : public QFileSystemModel {
    Q_OBJECT
  public:
    explicit ExtendedFileSystemModel(QObject *parent = nullptr);

    ~ExtendedFileSystemModel();
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void calculateSize(const QModelIndex &index);

  private:
    qint64 dirSize(const QString &path) const;
    
  private:
    // to avoid heavy recalculations 
    mutable QMap<QString, qint64> size_cache;
  };
} // namespace model

