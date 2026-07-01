#pragma once

#include <QFileSystemModel>
#include <QHash>

namespace model {
  constexpr int SIZE_COLUMN = 1;
  constexpr qint64 PENDING_CALCULATION = -1;
  
  class ExtendedFileSystemModel : public QFileSystemModel {
    Q_OBJECT
  public:
    explicit ExtendedFileSystemModel(QObject *parent = nullptr);

    ~ExtendedFileSystemModel() = default;
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void calculateSize(const QModelIndex &index, bool use_cache = false);
    void calculateSizeRecursive(const QModelIndex &index);

  private slots:
    void onRootPathChanged(const QString& path);

  private:
    qint64 dirSize(const QString &path) const;
    void dirSizeAsync(const QString &path);
    
  private:
    // to avoid heavy recalculations 
    mutable QHash<QString, qint64> size_cache;
  };
} // namespace model

