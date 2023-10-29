#include "Backend.hpp"

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>

#include <algorithm>
#include <optional>

#include <QDebug>

namespace bankconv {

namespace {

struct EntryCheckingAccount {
    QString accountNumber; // Auftragskonto
    QString dateFirst;     // Buchung -> first date
    QString dateSecond;    // Valuata -> second date
    QString payee;         // Auftragsgeber/Empfänger
    QString bookingType;   // Buchungstext
    QString bookingReason; // Verwendungszweck
    QString saldo;         // Saldo -> How much is on the account after booking
    QString currency;      // WÄhrung
    QString amount;        // Betrag
};

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

std::optional<QString> extractAccountNumber(const std::vector<QString>& rows)
{
    const auto cit = std::find_if(rows.cbegin(), rows.cend(), [](const QString &row) {
        return row.startsWith("Onlinekonto");
    });
    if(cit == rows.cend()) {
        return{};
    }

    const auto& row = *cit;
    const auto items = row.split(',');
    if(items.size() != 2) {
        return {};
    }
    const auto accountNumber = items.back();
    return accountNumber.simplified();
}

std::optional<QString> extractCurrency(const std::vector<QString>& rows)
{
    const auto cit = std::find_if(rows.cbegin(), rows.cend(), [](const QString &row) {
        return row.startsWith("Datum Wert Erläuterung ");
    });
    if(cit == rows.cend()) {
        return{};
    }
    const auto& row = *cit;
    const auto items = row.split(' ');

    return items.back();
}

QString extractValue(const QString& row)
{
    auto value = row.section(' ', -1);
    const auto token = value.back();
    if (token == '-') {
        value.push_front(token);
    }
    value.removeLast();
    // TODO validate
    value.remove('.');
    return value;
}

std::optional<QString> extractStartValue(std::vector<QString>::const_iterator &cit,
                          const std::vector<QString> &rows)
{
    cit = std::find_if(cit, rows.cend(), [](const QString &row) {
        return row.startsWith(" Kontostand");
    });

    if(cit == rows.cend()) {
        return{};
    }

    const auto row = *cit;
    auto startValue = row.section(' ', -1);
    const auto token = startValue.back();
    if (token == '-') {
        startValue.push_front(token);
    }
    startValue.removeLast();
    // TODO validate
    return startValue;
}

bool isEndOfPageEntries(const QString &row)
{
    return row == " ";
}

bool isEndOfDocument(const QString &row)
{
    return row.startsWith(" Kontostand am");
}

QString extractDateFirst(const QString &row)
{
    // TODO error handling
    return row.section(' ', 0, 0);
}

QString extractDateSecond(const QString &row)
{
    // TODO error handling
    return row.section(' ', 1, 1);
}

QString extractBookingType(const QString &row)
{
    // TODO error handling
    return row.section(' ', 2, 2);
}

bool isAmount(const QString &row)
{
    static QRegularExpression re{R"(\d+,\d{2}(\+|-))"};
    auto match = re.match(row);
    return match.hasMatch();
}

QString
extractBookingReason(std::vector<QString>::const_iterator &cit)
{
    QString bookingReason;
    while (!isAmount(*cit)) {
        if (!bookingReason.isEmpty()) {
            bookingReason += " ";
        }
        bookingReason += *cit;
        ++cit;
    }
    bookingReason = bookingReason.simplified();

    return bookingReason;
}

std::pair<QString, QString>
extractPayeeAndBookingReason(std::vector<QString>::const_iterator &cit)
{
    static QRegularExpression re{R"(\s{2,})"};
    const auto &entries = cit->split(re);
    qWarning() << entries;
    Q_ASSERT(entries.size() == 2);
    const auto payee = entries[0];
    auto bookingReason = entries[1];
    ++cit;
    while (!isAmount(*cit)) {
        if (!bookingReason.isEmpty()) {
            bookingReason += " ";
        }
        bookingReason += *cit;
        ++cit;
    }
    bookingReason = bookingReason.simplified();

    return {payee, bookingReason};
}

std::pair<QString, QString>
extractPayeeAndBookingReason(std::vector<QString>::const_iterator &cit, const QString& bookingType)
{
    if(bookingType == "Geldautomat") {
        // Not payee known for Geldautomat
        return { QString{}, extractBookingReason(cit)};
    }
    return extractPayeeAndBookingReason(cit);

}

QString extractAmount(const QString &row)
{
    auto amount = row.simplified();
    const auto token = amount.back();
    if (token == '-') {
        amount.push_front(token);
    }
    amount.removeLast();
    return amount;
}

double calculateSaldo(double saldoLast, double amount)
{
    return saldoLast + amount;
}

QString calculateSaldo(QString saldoLast, QString amount)
{
    saldoLast.remove('.');
    saldoLast.replace(',', '.');
    amount.remove('.');
    amount.replace(',', '.');

    const auto saldoLastDouble =saldoLast.toDouble();
    qWarning() << "saldoLastDouble: " << saldoLastDouble;
    const auto amountDouble = amount.toDouble();
    qWarning() << "amountDouble: " << amountDouble;
    const auto saldoAsDouble =
        calculateSaldo(saldoLastDouble, amountDouble);
    auto saldo = QString::number(saldoAsDouble, 'f', 2);
    saldo.replace('.', ',');
    return saldo;
}

std::optional<EntryCheckingAccount> extractEntry(
    std::vector<QString>::const_iterator &cit,
    const QString& saldoLast,
    const QString& accountNumber,
    const QString& currency)
{
    const auto dateFirst = extractDateFirst(*cit);
    qWarning() << "dateFirst: " << dateFirst;
    const auto dateSecond = extractDateSecond(*cit);
    qWarning() << "dateSecond: " << dateSecond;
    const auto bookingType = extractBookingType(*cit);
    qWarning() << "bookingType: " << bookingType;
    ++cit;

    const auto [payee, bookingReason] =
        extractPayeeAndBookingReason(cit, bookingType);
    qWarning() << "payee: " << bookingReason;
    qWarning() << "bookingReason: " << bookingReason;

    const auto amount = extractAmount(*cit);
    qWarning() << amount;

    const auto saldo = calculateSaldo(saldoLast, amount);
    qWarning() << "saldo: " << saldo;

    return EntryCheckingAccount{
       accountNumber,
       dateFirst,
       dateSecond,
       payee,
       bookingType,
       bookingReason,
       saldo,
       currency,
       amount};
}

std::vector<EntryCheckingAccount>
toEntriesCheckingAccount(const QByteArray &rawDataFromPdf)
{
    const auto rows = toRows(rawDataFromPdf);

    const auto maybeAccountNumber = extractAccountNumber(rows);
    if(!maybeAccountNumber) {
        return{};
    }
    const auto& accountNumber = *maybeAccountNumber;

    const auto maybeCurrency = extractCurrency(rows);
    if(!maybeAccountNumber) {
        return{};
    }
    const auto& currency = *maybeCurrency;

    std::vector<EntryCheckingAccount> entries{};
    auto cit = rows.begin();
    const auto maybeStartValue = extractStartValue(cit, rows);
    if(!maybeStartValue) {
        return {};
    }
    const auto& startValue = *maybeStartValue;
    qWarning() << "startValue: " << startValue;

    ++cit;

    auto saldoLast = startValue;
    // Run until reach end of document
    for (;;) {
        // Run until current page is finnished
        for(;;) {
            if(isEndOfPageEntries(*cit)) {
                break;
            }
            if(isEndOfDocument(*cit)) {
                // Sanity check. Calculated saldo should be same as the one in
                // the file
                Q_ASSERT(extractValue(*cit) == entries.back().saldo);
                return entries;
            }
            if(const auto maybeEntry =
                extractEntry(cit, saldoLast, accountNumber, currency);
                maybeEntry.has_value()) {
                entries.emplace_back(*maybeEntry);
                saldoLast = entries.back().saldo;
            }
            ++cit;
            if(cit == rows.end()) {
                break;
            }
        }

        cit = std::find_if(cit, rows.cend(), [](const QString &row) {
            return row.startsWith("Datum Wert Erläuterung");
        });
        ++cit;
    }

    constexpr char separator = ';';

    for(const auto& entry : entries) {
        qWarning() << entry.accountNumber << separator << entry.dateFirst
            << separator << entry.dateSecond << separator << entry.payee
            << separator << entry.bookingType << separator
            << entry.bookingReason << separator << entry.saldo << separator
            << entry.currency << separator << entry.amount << separator
            << entry.currency;
    }
    return entries;
}

QVariantList getPdfFiles(const QUrl folder)
{

    QDir dir{folder.path()};
    const auto &entryList = dir.entryList(QDir::Files);

    qWarning() << dir;

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
    : QObject{parent}, m_pdfFiles(getPdfFiles(m_folder))
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
    // TODO relative path based on exe.
    QString program =
        "/home/sandro/Documents/bankconv/bankconv/pdf_to_utf8/dist/pdf_to_utf8";
    Q_ASSERT(QFileInfo::exists(program));

    QStringList arguments = {
        "/mnt/Buisness/Dokumente/Beide/Haushalt/Kontoauszüge/Sparkasse "
        "Emsland/Girokonto Sandro/2015/2015.04 Sparkasse Emsland Kontoauszug "
        "102303070.PDF"};

    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted()) {
        qWarning() << "Could not start converter";
        return;
    }

    if (!process.waitForFinished()) {
        qWarning() << "Could not finnish conversion";
        return;
    }

    const auto rawDataFromPdf = process.readAll();
    const auto rows = toRows(rawDataFromPdf);

    QFile fileDebug{"debug.txt"};
    if (fileDebug.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream outDebug(&fileDebug);
        for (const auto &row : rows) {
            outDebug << row << "\n";
        }
    }
    fileDebug.close();

    QFile file("output.csv");
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&file);

        const auto entries = toEntriesCheckingAccount(rawDataFromPdf);

        constexpr char separator = ';';

        out << "Auftragskonto" << separator << "Buchung" << separator
            << "Valuta"
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

void Backend::_setPdfFiles(const QVariantList &pdfFiles)
{
    if (m_pdfFiles == pdfFiles) {
        return;
    }
    m_pdfFiles = pdfFiles;
    emit pdfFilesChanged();
}

} // namespace bankconv

/*

// find Kontostand and Save Value at the end
Kontostand am 04.03.2015, Auszug Nr.    3            6.953,30+

// Save Dates Buchung Valuta
06.03.2015 06.03.2015 Lastschrift
// Save Auftraggeber/Empf�nger / delete excess soaces
ARAL DOETLING A1            050307387187295171221200700
// until value copy all without space into Verwendungszweck
OLV71017761 05.03 07.38 ME7
// Copy Betrag without space
           52,91-
// Add to start value

10.03.2015 10.03.2015 Lastschrift
AMAZON EU S.A.R.L.                  5 RUE PLAETIS

302-3538322-9354736 Amazon.de 24234 81875217818

2423481875217818                    XAYE5JvujTtZLKXla23hhxeWgBF,Rz

Gläubiger-ID: DE24ZZZ00000561652

            9,93-
// marks the end
Kontostand am 31.03.2015 um 20:04 Uhr           10.834,02+

*/
