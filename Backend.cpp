#include "Backend.hpp"

#include <QtCore/QDir>
#include <QtCore/QProcess>

#include <algorithm>

#include <QDebug>

namespace bankconv {

namespace {

QVariantList getPdfFiles(const QUrl folder) {

    QDir dir{folder.path()};
    const auto& entryList = dir.entryList(QDir::Files);

    qWarning() << dir;

    QVariantList pdfFiles;
    pdfFiles.reserve(entryList.size());

    for (const QString &filename : entryList) {

        if(!filename.endsWith(".pdf") && !filename.endsWith(".PDF")) {
            continue;
        }

        qWarning() << filename;

        pdfFiles.push_back(filename);
    }
    return pdfFiles;
}

}

Backend::Backend(QObject *parent)
    : QObject{parent},
      m_pdfFiles(getPdfFiles(m_folder))
{
}

QUrl Backend::folder() const
{
    return m_folder;
}

bool Backend::buttonStartEnabled() const
{
    return true;
    //return !m_pdfFiles.isEmpty();
}


QVariantList  Backend::pdfFiles() const
{
    return m_pdfFiles;
}

void Backend::setFolder(const QUrl &folder)
{
    if (m_folder == folder) {
        return;
    }
    m_folder = folder;

    qWarning() << folder;
    const auto pdfFiles = getPdfFiles(m_folder);
    _setPdfFiles(pdfFiles);
}

void Backend::tryConvertToCSV()
{
    QString program = "/home/sandro/Documents/bankconv/bankconv/pdf_to_utf8/dist/pdf_to_utf8";
    //QStringList arguments = {m_folder.path() + "/" + m_pdfFiles[0].toString()};

    qWarning() << "program exists:" << QFileInfo(program).exists();

    QStringList arguments = {"/mnt/Buisness/Dokumente/Beide/Haushalt/Kontoauszüge/Sparkasse Emsland/Girokonto Sandro/2015/2015.04 Sparkasse Emsland Kontoauszug 102303070.PDF"};

    QProcess process;
    process.start(program, arguments);
    if(!process.waitForStarted()) {
        qWarning() << "Could not start converter";
    }

    if (!process.waitForFinished()) {
        qWarning() << "Could not finnish conversion";
    }

    QByteArray result = process.readAll();

//    QFile file(arguments[0] + ".txt");

//    qWarning() << QString::fromUtf8(result);
}

void Backend::_setPdfFiles(const QVariantList& pdfFiles)
{
    if(m_pdfFiles == pdfFiles) {
        return;
    }
    m_pdfFiles = pdfFiles;
    emit pdfFilesChanged();

    }

} // namespace sps
