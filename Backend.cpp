#include "Backend.hpp"

#include "EntryCheckingAccount.hpp"
#include "EntryCreditCard.hpp"
#include "ExtractEntriesCheckingAccountSparkasseEmsland.hpp"
#include "ExtractEntriesCreditCardSparkasseEmsland.hpp"

#include <QtCore/QDir>
#include <QtCore/QProcess>

#include <algorithm>
#include <optional>
#include <iterator>
#include <set>

#include <QDebug>

namespace bankconv {

namespace {

std::optional<QByteArray> extractRawData(const QString& filenameAbsolutePdf)
{
    // TODO relative path based on exe.
    QString program =
        "/home/sandro/Documents/bankconv/bankconv/pdf_to_utf8/dist/pdf_to_utf8";
    Q_ASSERT(QFileInfo::exists(program));

    qWarning() << "filenameAbsolutePdf: " << filenameAbsolutePdf;
    QStringList arguments = {filenameAbsolutePdf};

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

std::vector<QString> toRows(const QByteArray &rawDataFromPdf)
{
    QString result = QString::fromUtf8(rawDataFromPdf);
    QStringList rawRows = result.split("\\n");

    std::vector<QString> rows;
    rows.reserve(rawRows.size());
    for (auto &rawRow : rawRows) {
        // rawRow = rawRow.simplified();
        if (rawRow.isEmpty()) {
            continue;
        }
        rows.emplace_back(rawRow);
    }
    return rows;
}

QString toFileNameAbsoluteText(QString filenameAbolutePdf)
{
    filenameAbolutePdf.replace("PDF", "txt");
    return filenameAbolutePdf;
}

void exportRowsToTextFile(const std::vector<QString>& rows, const QString& filenameAbsolute)
{
    QFile fileDebug{filenameAbsolute};
    if (fileDebug.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream outDebug(&fileDebug);
        for (const auto &row : rows) {
            outDebug << row << "\n";
        }
    }
}

bool isPdfCheckingAccount(const std::vector<QString>& rows)
{
    const auto it = std::ranges::find_if(rows,
                      [](QString row){
                        return row.startsWith("Kontoauszug");
    });

    return it != rows.end();
}

bool isPdfFileCreditCardSparkasseEmsland(const std::vector<QString>& rows)
{
    const auto it = std::ranges::find_if(rows,
                                         [](QString row){
                                             return row.startsWith("Abrechnung / Saldenmitteilung");
                                         });

    return it != rows.end();
}

QString toFileNameAbsoluteCSV(QString filenameAbolutePdf)
{
    filenameAbolutePdf.replace("PDF", "csv");
    return filenameAbolutePdf;
}

void writeEntries(QTextStream& textStream, const std::vector<EntryCheckingAccount>& entries)
{
    constexpr char separator = ';';

    textStream << "Auftragskonto" << separator << "Buchung" << separator
        << "Valuta" << separator
        << "Auftraggeber/Empf�nger" << separator << "Buchungstext"
        << separator << "Verwendungszweck" << separator << "Saldo"
        << separator << "W�hrung" << separator << "Betrag" << separator
        << "W�hrung" << '\n';

    for (const auto &entry : entries) {
        textStream << entry << '\n';
    }
}

void writeEntries(QTextStream& textStream, const std::vector<EntryCreditCard>& entries)
{
    constexpr char separator = ';';

    textStream << "Umsatz get�tigt von"   << separator<<
        "Belegdatum" << separator<<
        "Buchungsdatum" << separator<<
        "Originalbetrag" << separator<<
        "Originalw�hrung" << separator<<
        "Umrechnungskurs" << separator<<
        "Buchungsbetrag" << separator<<
        "Buchungsw�hrung" << separator<<
        "Transaktionsbeschreibung" << separator<<
        "Transaktionsbeschreibung Zusatz" << separator<<
        "Buchungsreferenz" << separator<<
        "Geb�hrenschl�ssel" << separator<<
        "L�nderkennzeichen" << separator<<
        "BAR-Entgelt+Buchungsreferenz" << separator<<
        "AEE+Buchungsreferenz" << separator<<
        "Abrechnungskennzeichen" << '\n';

    for (const auto &entry : entries) {
        textStream << entry << '\n';
    }
}

template<typename EntryType>
void exportEntriesToCSVFile(const std::vector<EntryType>& entries, const QString& filenameAbsolute)
{
    QFile file(filenameAbsolute);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&file);

        writeEntries(out, entries);
    }
}

void tryConvertPdfToCSV(const QString& folder, const QString& filenamePdf)
{
    const auto filenameAbsolutePdf = folder + "/" + filenamePdf;

    const auto maybeRawDataFromPdf = extractRawData(filenameAbsolutePdf);
    if(!maybeRawDataFromPdf) {
        return;
    }
    const auto rows = toRows(*maybeRawDataFromPdf);

    // Print rows to text file for debug
    const auto filenameAbsoluteDebug = toFileNameAbsoluteText(filenameAbsolutePdf);
    exportRowsToTextFile(rows, filenameAbsoluteDebug);

    const auto filenameCSV = toFileNameAbsoluteCSV(filenameAbsolutePdf);
    if(isPdfCheckingAccount(rows)) {
        const auto entriesCheckingAccount = extractEntriesCheckingAccountSparkasseEmsland(rows);
        exportEntriesToCSVFile(entriesCheckingAccount, filenameCSV);
    }
    else if(isPdfFileCreditCardSparkasseEmsland(rows)) {
        const auto entriesCreditCard = extractEntriesCreditCardSparkasseEmsland(rows);
        exportEntriesToCSVFile(entriesCreditCard, filenameCSV);
    }
    else {
        qWarning() << "Unsupported pdf: " << filenamePdf;
    }
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
      m_folder{
      "/mnt/Buisness/Dokumente/Beide/Haushalt/Kontoauszüge/Sparkasse Emsland/Girokonto Sandro/2015/"
      },
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
