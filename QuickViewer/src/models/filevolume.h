#ifndef FILELIST_H
#define FILELIST_H

#include <QtCore>
#include <QtGui>
#include <QtConcurrent>

#include "exif.h"
#include "fileloader.h"
#include "timeorderdcache.h"

struct ImageContent
{
public:
    QPixmap Image;
    QSize BaseSize;
    QSize ImportSize;
    QString Path;
    easyexif::EXIFInfo Info;

    ImageContent(){}
    ImageContent(QPixmap image, QString path, QSize size, easyexif::EXIFInfo info)
        : Image(image), Path(path), BaseSize(size), ImportSize(image.size()), Info(info) {}
    ImageContent(const ImageContent& rhs)
        : Image(rhs.Image), Path(rhs.Path), BaseSize(rhs.BaseSize), ImportSize(rhs.ImportSize), Info(rhs.Info) {}
    inline ImageContent& operator=(const ImageContent &rhs)
    {
        Image = rhs.Image;
        Path = rhs.Path;
        BaseSize = rhs.BaseSize;
        ImportSize = rhs.ImportSize;
        Info = rhs.Info;
        return *this;
    }
    bool wideImage() const {return BaseSize.width() > BaseSize.height(); }
};



class IFileVolume : public QObject
{
    Q_OBJECT
//    Q_DISABLE_COPY(IFileVolume)
public:
    enum CacheMode
    {
        Normal,
        FastFowrard,
        CoverOnly,
        NoAsync,
    };

    typedef QFuture<ImageContent> future_image;

    explicit IFileVolume(QObject *parent, IFileLoader* loader);
    ~IFileVolume() {
        if(m_loader) {
            delete m_loader;
            m_loader = nullptr;
        }
    }
    /**
     * @brief A factory function that returns an instance of IFileVolume from the path of the specified file or directory
     * @return An object that inherits the IFileVolume interface. It is null if generation failed
     */
    static IFileVolume* CreateVolume(QObject* parent, QString path);
    static IFileVolume* CreateVolumeWithOnlyCover(QObject* parent, QString path, CacheMode mode = CacheMode::CoverOnly);

    static ImageContent futureLoadImageFromFileVolume(IFileVolume* volume, QString path);
    static QString FullPathToVolumePath(QString path);
    static QString FullPathToSubFilePath(QString path);

    bool isArchive() const { return m_loader->isArchive(); }

    QString currentPath() {
        if(m_loader->isArchive())
            return QString("%1::%2")
                    .arg(QDir::fromNativeSeparators(m_loader->volumePath()))
                    .arg(m_filelist[m_cnt]);
        else
            return QDir::fromNativeSeparators(QDir(m_loader->volumePath()).absoluteFilePath(m_filelist[m_cnt]));
    }
    QString getPathByFileName(QString name) {
        if(m_loader->isArchive())
            return QString("%1::%2")
                    .arg(QDir::fromNativeSeparators(m_loader->volumePath()))
                    .arg(name);
        else
            return QDir(m_loader->realVolumePath()).absoluteFilePath(name);
    }
    QString getIndexedFileName(int idx) {
        if(idx < 0 || idx >= m_filelist.size())
            return "";
        return m_filelist[idx];
    }
    QString getPathByIndex(int idx) {
        if(idx < 0 || idx >= m_filelist.size())
            return "";
        return QDir(m_loader->volumePath()).absoluteFilePath(m_filelist[idx]);
    }
    void setCacheMode(CacheMode cachemode) { m_cacheMode = cachemode; }
    CacheMode cacheMode() const { return m_cacheMode; }


    const ImageContent currentImage() { return m_cacheMode == NoAsync ? m_currentCacheSync : m_currentCache.result(); }
    QString volumePath() { return m_loader->volumePath(); }

    bool nextPage();
    bool prevPage();
    bool findPageByIndex(int idx);

    /**
     * @brief Move to the file corresponding to the idx value specified in the file list(Max is size()-1)
     */
    bool findImageByIndex(int idx) {
        if(idx < 0 || idx >= m_filelist.size())
            return false;
        m_cnt = idx;
        return true;
    }

    /**
     * @brief Move to the file corresponding to the file name specified in the current file list
     */
    bool findImageByName(QString name) {
        int idx = m_filelist.indexOf(name);
        if(idx < 0)
            return false;
        m_cnt = idx;
        return true;
    }

    /**
     * @brief loadImageByName Reads and returns the image corresponding to the file name specified in the file list without advancing the internal counter
     */
    QByteArray loadByteArrayByName(const QString& name) { return m_loader->getFile(name, m_mutex); }
    /**
     * @brief Returns the number of pages the volume has
     */
    int size() { return m_filelist.size(); }
    /**
     * @brief on_ready Called when the application is ready. First, or the image to be displayed next and its file path are emitted
     */
    void on_ready();
    int pageCount() { return m_cnt; }

//    QPixmap getIndexedImage(int idx);
//    QString getIndexedImageName(int idx) { return m_filelist[idx]; }
//    QString currentImageName() const { return m_filelist[m_cnt]; }
    const ImageContent getIndexedImageContent(int idx);


protected:
    /**
     * @brief m_cnt File counter in the volume
     */
    int m_cnt;
    QList<QString> m_filelist;
    future_image m_currentCache;
    ImageContent m_currentCacheSync;

    TimeOrderdCache<int, future_image> m_imageCache;
//    QMap<int, future_image> m_imageCache;
//    QList<int> m_pageCache;

    QMutex m_mutex;

    IFileLoader* m_loader;
    CacheMode m_cacheMode;
};



#endif // FILELIST_H