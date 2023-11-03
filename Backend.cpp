#include "Backend.hpp"

#include "EntryCheckingAccount.hpp"

#include <QtCore/QDir>
#include <QtCore/QProcess>

#include <algorithm>
#include <optional>
#include <set>

#include <QDebug>

namespace bankconv {

namespace {

std::vector<QString> toRows(const QByteArray &rawDataFromPdf)
{
    QString result = QString::fromUtf8(rawDataFromPdf);
    QStringList rawRows = result.split("\\n");
    qWarning() << rawRows.size();

    std::vector<QString> rows;
    rows.reserve(rawRows.size());
    for (auto &rawRow : rawRows) {
        // rawRow = rawRow.simplified();
        if (rawRow.isEmpty()) {
            continue;
        }
        rows.emplace_back(rawRow);
    }
    qWarning() << rows.size();
    return rows;
}

std::optional<QByteArray> extractRawData(const QString& filenamePdfAbsolute)
{
    // TODO relative path based on exe.
    QString program =
        "/home/sandro/Documents/bankconv/bankconv/pdf_to_utf8/dist/pdf_to_utf8";
    Q_ASSERT(QFileInfo::exists(program));

    qWarning() << "filenamePdfAbsolute: " << filenamePdfAbsolute;
    QStringList arguments = {filenamePdfAbsolute};

    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted()) {
        qWarning() << "Could not start converter";
        return {};
    }

    if (!process.waitForFinished()) {
        qWarning() << "Could not finnish conversion";
        return {};
    }

    const auto rawDataFromPdf = process.readAll();
    return rawDataFromPdf;
}

void tryConvertPdfToCSV(const QString& folder, const QString& filenamePdf)
{
    const auto filenamePdfAbsolute = folder + "/" + filenamePdf;

    const auto maybeRawDataFromPdf = extractRawData(filenamePdfAbsolute);
    if(!maybeRawDataFromPdf) {
        return;
    }
    const auto rows = toRows(*maybeRawDataFromPdf);

    // TODO debug
    auto filenameDebug = filenamePdfAbsolute;
    filenameDebug.replace("PDF", "txt");
    QFile fileDebug{filenameDebug};
    if (fileDebug.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream outDebug(&fileDebug);
        for (const auto &row : rows) {
            outDebug << row << "\n";
        }
    }
    fileDebug.close();

    auto filenameCSV = filenamePdfAbsolute;
    filenameCSV.replace("PDF", "csv");
    QFile file(filenameCSV);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&file);

        const auto entries = toEntriesCheckingAccount(*maybeRawDataFromPdf);

        constexpr char separator = ';';

        out << "Auftragskonto" << separator << "Buchung" << separator
            << "Valuta" << separator
            << "Auftraggeber/Empf�nger" << separator << "Buchungstext"
            << separator << "Verwendungszweck" << separator << "Saldo"
            << separator << "W�hrung" << separator << "Betrag" << separator
            << "W�hrung" << '\n';

        for (const auto &entry : entries) {
            out << entry.accountNumber << separator << entry.dateFirst
                << separator << entry.dateSecond << separator << entry.payee
                << separator << entry.bookingType << separator
                << entry.bookingReason << separator << entry.saldo << separator
                << entry.currency << separator << entry.amount << separator
                << entry.currency << '\n';
        }
    }
    file.close();
}

QVariantList getPdfFiles(const QUrl folder)
{

    QDir dir{folder.path()};
    const auto &entryList = dir.entryList(QDir::Files);

    QVariantList pdfFiles;
    pdfFiles.reserve(entryList.size());

    for (const QString &filename : entryList) {

        if (!filename.endsWith(".pdf") && !filename.endsWith(".PDF")) {
            continue;
        }

        qWarning() << filename;

        pdfFiles.push_back(filename);
    }
    return pdfFiles;
}

} // namespace

Backend::Backend(QObject *parent)
    : QObject{parent},
      m_folder{"/mnt/Buisness/Dokumente/Beide/Haushalt/Kontoauszüge/Sparkasse Emsland/Geldmarkkonto Sandro"},
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
    // return !m_pdfFiles.isEmpty();
}

QVariantList Backend::pdfFiles() const
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
//    const auto folder = "/mnt/Buisness/Dokumente/Beide/Haushalt/Kontoauszüge/Sparkasse "
//                        "Emsland/Girokonto Sandro/2015";
//    const auto filename = "2015.04 Sparkasse Emsland Kontoauszug 102303070.PDF";

    for(const auto& pdfFile :  m_pdfFiles) {
        tryConvertPdfToCSV(m_folder.path(), pdfFile.toString());
    }
}

void Backend::_setPdfFiles(const QVariantList &pdfFiles)
{
    if (m_pdfFiles == pdfFiles) {
        return;
    }
    m_pdfFiles = pdfFiles;
    emit pdfFilesChanged();
}

} // namespace bankconv
