#ifndef BANKCONV_BACKEND_HPP
#define BANKCONV_BACKEND_HPP

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QVariant>

#include <QtQml/qqmlregistration.h>

namespace bankconv {

class Backend : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QUrl folder READ folder NOTIFY folderChanged)
    Q_PROPERTY(bool buttonStartEnabled READ buttonStartEnabled NOTIFY pdfFilesChanged)
    Q_PROPERTY(QVariantList pdfFiles READ pdfFiles NOTIFY pdfFilesChanged)
public:
    Backend(QObject *parent = nullptr);

    Backend(const Backend &) = delete;
    Backend(Backend &&) = delete;
    Backend &operator=(const Backend &) = delete;
    Backend &operator=(Backend &&) = delete;

    ~Backend() = default;

    QUrl folder() const;

    bool buttonStartEnabled() const;

    QVariantList pdfFiles() const;

    Q_INVOKABLE
    void setFolder(const QUrl &folder);

    Q_INVOKABLE
    void tryConvertToCSV();

signals:
    void folderChanged();
    void pdfFilesChanged();

private:
    void _setPdfFiles(const QVariantList& pdfFiles);

    QUrl m_folder;
    QVariantList m_pdfFiles;
};
} // namespace sps

#endif
