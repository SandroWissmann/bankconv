#include "EntryCreditCard.hpp"

#include "EntryUtils.hpp"

#include <QtCore/QTextStream>
#include <QtCore/QList>
#include <QtCore/QDebug>
#include <QtCore/QRegularExpression>

namespace bankconv {

std::optional<QString> extractDateCompensation(QString row)
{
    // kill optional - Seite 1 stuff after date
    if(row.contains('-')) {
        row = row.mid(0, row.indexOf('-'));
    }

    row = row.simplified();

    auto entries = row.split(' ');
    if(entries.size() < 2) {
        qWarning() << "Date could not be extracted";
        return{};
    }
    return entries.back();
}

std::optional<QString> extractYear(QString row)
{
    const auto maybeDateCompensation = extractDateCompensation(row);
    if(!maybeDateCompensation) {
        qWarning() << "Compensation date could not be extracted";
        return {};
    }

    const auto& dateCompensation = *maybeDateCompensation;
    return dateCompensation.section('.',-1);
}

std::optional<QString> extractRowDateCompensation(const std::vector<QString>& rows)
{
    return findRowWhoStartsWith(rows, {"Abrechnung / Saldenmitteilung "});
}

std::optional<QString> extractYear(const std::vector<QString>& rows)
{
    const auto maybeRowDateCompensation = extractRowDateCompensation(rows);
    if(!maybeRowDateCompensation) {
        qWarning() << "Could not extract row date compensation";
        return {};
    }
    return extractYear(*maybeRowDateCompensation);
}

std::optional<QString> extractCreditCardNumber(const QString& row)
{
    if(row.split(' ').size() < 5) {
        return {};
    }
    auto creditCardNumber = row.section(' ', 1, 4);
    auto parts = creditCardNumber.split(' ');
    return parts.front() + " " + "****" + " " + "****" + " " + parts.back();
}

std::optional<QString> extractRowCreditCardNumberAndOwner(const std::vector<QString>& rows)
{
    return findRowWhoStartsWith(rows, {"Mastercard ", "MasterCard"});
}

std::optional<QString> extractCreditCardNumber(const std::vector<QString>& rows)
{
    const auto maybeRowCreditCardNumberAndOwner = extractRowCreditCardNumberAndOwner(rows);
    if(!maybeRowCreditCardNumberAndOwner) {
        qWarning() << "Could not extract row credit card number / owner";
        return {};
    }
    return extractCreditCardNumber(*maybeRowCreditCardNumberAndOwner);
}

std::optional<QString> extractCurrency(const std::vector<QString>& rows)
{
    const auto it = std::ranges::find_if(rows, [](const QString &row) {
        return row.startsWith("Summe Waren/Dienstleistungen:") ;
    });
    if(it == rows.cend()) {
        return{};
    }
    const auto& row = it->simplified();
    const auto items = row.split(' ');

    return items.back();
}

std::vector<QString>::const_iterator goToStartOfRelevantData(
    const std::vector<QString> &rows)
{
    return std::find_if(rows.cbegin(), rows.cend(), [](QString row) {
        return row.contains("Saldovortrag");
    });
}

bool isEndOfPageEntries(const QString& row)
{
    return row.contains("--------------");
}

bool containsSum(const QString& row)
{
    return row.contains("MasterCard Summe");
}

//bool isEndOfPage(QString row)
//{
//    row = row.simplified();
//    if(row == "--------------")
//    {
//        return true;
//    }
//    return false;
//}

QString extractDayAndMonthFirst(const QString &row)
{
    // TODO error handling
    return row.section(' ', 0, 0);
}

QString extractDayAndMonthSecond(const QString &row)
{
    // TODO error handling
    return row.section(' ', 1, 1);
}

QString extractMonth(const QString& dayAndMonth)
{
    // TODO error handling
    auto entries = dayAndMonth.split('.');
    entries.pop_back();
    return entries.back();
}

QString makeDate(const QString& dayAndMonth, QString year, bool hasDecemberAndJanuaryDates)
{
    if(hasDecemberAndJanuaryDates) {
        const auto month = extractMonth(dayAndMonth);
        if(month == "12") {
            auto yearAsInt = year.toInt();
            --yearAsInt;
            return dayAndMonth + QString::number(yearAsInt);
        }
    }
    return dayAndMonth + year;
}

QString extractDateFirst(
    const QString &row,
    const QString& year,
    bool hasDecemberAndJanuaryDates)
{
    Q_ASSERT(!row.simplified().isEmpty());
    const auto dayAndMonth = extractDayAndMonthFirst(row);

    return makeDate(dayAndMonth, year, hasDecemberAndJanuaryDates);
}

QString extractDateSecond(
    const QString &row,
    const QString& year,
    bool hasDecemberAndJanuaryDates)
{
    Q_ASSERT(!row.simplified().isEmpty());
    const auto dayAndMonth = extractDayAndMonthSecond(row);

    return makeDate(dayAndMonth, year, hasDecemberAndJanuaryDates);
}


void tryGoToNextPageEntries(std::vector<QString>::const_iterator &cit,
                            const std::vector<QString>& rows)
{
    cit = std::find_if(cit, rows.cend(), [](const QString& row) {
        //qWarning() << "row: " <<row;
        return row.contains("MasterCard Übertrag");
    });
    ++cit;  // empty space
    ++cit;  // next row
    Q_ASSERT(cit != rows.cend());
}

QString extractFirstMonth(const QString& row)
{
    const auto dayAndMonth = extractDayAndMonthFirst(row);
    return  extractMonth(dayAndMonth);
}

QString extractSecondMonth(const QString& row)
{
    const auto dayAndMonth = extractDayAndMonthSecond(row);
    return  extractMonth(dayAndMonth);;
}

void checkIfIsDecemberOrJanuary(const QString& month, bool& hasDecember, bool& hasJanuary)
{
    if(!hasDecember) {
        if(month == "12") {
            hasDecember = true;
        }
    }
    if(!hasJanuary) {
        if(month == "01") {
            hasJanuary = true;
        }
    }
}

bool extractHasDecemberAndJanuaryDates(const std::vector<QString> rows)
{
    // we need to go again over all entries and catch the dates here...
    auto cit = goToStartOfRelevantData(rows);
    ++cit;
    ++cit;

    auto hasDecember = false;
    auto hasJanuary = false;

    for(;;) {
        qWarning() << "row: " << *cit;
        const auto firstMonth = extractFirstMonth(*cit);
        checkIfIsDecemberOrJanuary(firstMonth, hasDecember, hasJanuary);
        if(hasDecember&& hasJanuary) {
            return true;
        }
        const auto secondMonth = extractSecondMonth(*cit);
        checkIfIsDecemberOrJanuary(secondMonth, hasDecember, hasJanuary);
        if(hasDecember&& hasJanuary) {
            return true;
        }

        for(;;) {
            const auto row = *cit;
            if(row.simplified().isEmpty()) {
                break;
            }
            // special case end of file
            if(row.contains("--------------")) {
                --cit;
                break;
            }
            ++cit;
        }
        ++cit;
        if(isEndOfPageEntries(*cit)) {
            ++cit;

             if(containsSum(*cit)) {
                return false;
            }
            qWarning() << "*cit before tryGoToNextPageEntries: " << *cit;
            // We have to go to the next page because same string is in
            // this line == end of first page
            // line of next page
            ++cit;
            tryGoToNextPageEntries(cit, rows);
            Q_ASSERT(cit != rows.cend());
        }
    }
    Q_ASSERT(false);
    return false;


//    for(;;) {
//        for(;;) {
//            if(isEndOfPageEntries(*cit)) {
//                break;
//            }
//            if(isEndOfPage(*cit)) {
//                ++cit;
//                return false;
//            }
//            const auto firstMonth = extractFirstMonth(*cit);
//            checkIfIsDecemberOrJanuary(firstMonth, hasDecember, hasJanuary);
//            if(hasDecember&& hasJanuary) {
//                return true;
//            }
//            const auto secondMonth = extractSecondMonth(*cit);
//            checkIfIsDecemberOrJanuary(secondMonth, hasDecember, hasJanuary);
//            if(hasDecember&& hasJanuary) {
//                return true;
//            }

//            for(;;) {
//                const auto row = *cit;
//                if(row.simplified().isEmpty()) {
//                    break;
//                }
//                // special case end of file
//                if(row.contains("--------------")) {
//                    --cit;
//                    break;
//                }
//                ++cit;
//            }

//            // TODO extract dates and check for year jump
//            ++cit;
//            if(cit == rows.end()) {
//                break;
//            }
//        }
//        tryGoToNextPageEntries(cit, rows);
//    }
//    Q_ASSERT(false);
//    return false;
}

std::vector<QString> extractEntryRows(
    std::vector<QString>::const_iterator &cit)
{
    std::vector<QString> rows{};

    for(;;) {
        const auto row = *cit;
        if(row.simplified().isEmpty()) {
            break;
        }
        // special case end of file
        if(row.contains("--------------")) {
            break;
        }
        rows.emplace_back(row);
        ++cit;
    }
    Q_ASSERT(rows.size() >=1);
    return rows;
}



std::pair<QString, QString> extractPayeeAndBookingReason(
    const QString& row)
{
    // TODO
    static QRegularExpression re{R"(\s{2,})"};
    auto entries = row.split(re);
    qWarning() << "entries: " << entries;
    // [dates, payee, [optional bookingReasons] , amount, ""]
    Q_ASSERT(entries.size() >= 4);

    const auto payee = entries[1];
    // special case
    if(payee == "Jahrespreis" || payee =="Rückzahlung zu Gunsten des Girokontos") {
        // TODO hard coded institute. Coulb be extracted from file
        return {"Sparkasse Emsland", payee};
    }


    entries.pop_back(); // remove empty ""
    entries.pop_back(); // remove amount
    entries.pop_front(); // remove dates
    entries.pop_front(); // remove payee
    QString bookingReason;
    for(const auto& entry : entries) {
        if(!bookingReason.isEmpty()) {
            bookingReason += " ";
        }
        bookingReason += entry;
    }
    return {payee, bookingReason};

}

std::pair<QString, QString> extractPayeeAndBookingReason(
    const std::vector<QString>& rows)
{
    auto [payee, bookingReason] = extractPayeeAndBookingReason(rows[0]);

    for(auto cit = rows.cbegin() + 1; cit != rows.cend(); ++cit) {
        bookingReason += " ";
        bookingReason += cit->simplified();
    }
    return {payee, bookingReason};
}

std::optional<EntryCreditCard> extractEntry(
    std::vector<QString>::const_iterator &cit,
    const QString& saldoLast,
    const QString& creditCardNumber,
    const QString& currency,
    const QString& year,
    bool hasDecemberAndJanuaryDates)
{
    const auto dateFirst = extractDateFirst(*cit, year, hasDecemberAndJanuaryDates);
    qWarning() << "dateFirst: " << dateFirst;
    const auto dateSecond = extractDateSecond(*cit, year, hasDecemberAndJanuaryDates);
    qWarning() << "dateSecond: " << dateSecond;

    const auto amount = extractAmount(*cit);
    qWarning() << "amount: " << amount;

    const auto saldo = calculateSaldo(saldoLast, amount);
    qWarning() << "saldo: " << saldo;

    const auto& rows = extractEntryRows(cit);

    const auto& [payee, bookingReason] = extractPayeeAndBookingReason(rows);
    qWarning() << "payee: " << payee;
    qWarning() << "booking reason" << bookingReason;

    return EntryCreditCard{
        creditCardNumber,
        dateFirst,
        dateSecond,
        payee,
        bookingReason,
        saldo,
        currency,
        amount
    };
}

QString extractDateCompensation(const std::vector<QString> &rows)
{
    const auto maybeRowDateCompensation = extractRowDateCompensation(rows);
    if(!maybeRowDateCompensation) {
        qWarning() << "Could not extract row date compensation";
        return {};
    }
    const auto maybeDateCompensation =
        extractDateCompensation(*maybeRowDateCompensation);

    Q_ASSERT(maybeDateCompensation);
    return *maybeDateCompensation;
}

std::optional<QString> extractPayeeCompensation(const QString& row)
{
    auto parts = row.split('-');
    return parts.back().simplified();
}

QString extractPayeeCompensation(const std::vector<QString> &rows)
{
    const auto maybeRowPayeeCompensation =
        extractRowCreditCardNumberAndOwner(rows);
    if(!maybeRowPayeeCompensation) {
        qWarning() << "Could not extract row payee compensation";
        return {};
    }
    const auto maybePayeeCompensation =
        extractPayeeCompensation(*maybeRowPayeeCompensation);
    Q_ASSERT(maybePayeeCompensation);
    return *maybePayeeCompensation;
}

QString extractBookingReasonCompensation(const std::vector<QString> &rows)
{
    const auto maybeRow = findRowWhoStartsWith(rows, {"               Einzug von "});
    Q_ASSERT(maybeRow);
    const auto row = maybeRow->simplified();
    return row.section(' ', 0, -2);
}

QString calculateAmountCompensation(QString saldoLast)
{
    saldoLast.remove('-');
    return saldoLast;
}

std::optional<EntryCreditCard> extractEntryCompensation(
    const std::vector<QString> &rows,
    const QString& saldoLast,
    const QString& creditCardNumber,
    const QString& currency)
{
    qWarning() << "extractEntryCompensation";

    const auto dateCompensation = extractDateCompensation(rows);
    qWarning() << "dateCompensation" << dateCompensation;

    const auto payee = extractPayeeCompensation(rows);
    qWarning() << "payee" << payee;

    const auto bookingReason = extractBookingReasonCompensation(rows);
    qWarning() << "bookingReason" << bookingReason;

    const auto amount = calculateAmountCompensation(saldoLast);
    qWarning() << "amount" << amount;

    const auto saldo = calculateSaldo(saldoLast, amount);
    qWarning() << "saldo" << saldo;

    return EntryCreditCard{
        creditCardNumber,
        dateCompensation,
        dateCompensation,
        payee,
        bookingReason,
        saldo,
        currency,
        amount
    };
}


std::vector<EntryCreditCard> toEntriesCreditCard(const std::vector<QString> &rows)
{
    const auto maybeYear = extractYear(rows);
    if(!maybeYear) {
        qWarning() << "Error: No year";
        return {};
    }
    const auto& year = *maybeYear;
    qWarning() << "year: " << year;

    const auto maybeCreditCardNumber = extractCreditCardNumber(rows);
    if(!maybeCreditCardNumber) {
        qWarning() << "Error: No Credit Card Number";
        return{};
    }
    const auto& creditCardNumber = *maybeCreditCardNumber;
    qWarning() << "creditCardNumber: " << creditCardNumber;

    const auto maybeCurrency = extractCurrency(rows);
    if(!maybeCurrency) {
        qWarning() << "Error: No Currency";
        return{};
    }
    const auto& currency = *maybeCurrency;
    qWarning() << "currency: " << currency;

    auto cit = goToStartOfRelevantData(rows);
    if(cit == rows.cend()) {
        qWarning() << "Could not find start value";
        return{};
    }

    const auto startValue = extractAmount(*cit);
    qWarning() << "startValue: " << startValue;

    const auto hasDecemberAndJanuaryDates = extractHasDecemberAndJanuaryDates(rows);
    qWarning() << "hasDecemberAndJanuaryDates: " << hasDecemberAndJanuaryDates;

    ++cit;  // skip empty space
    ++cit;

    auto saldoLast = startValue;

    std::vector<EntryCreditCard> entries{};
    std::optional<QString> amountCompensationPreviousPage;

    for(;;) {
        qWarning() << "*cit before:" <<*cit;
        if(const auto& maybeEntry =
            extractEntry(
                cit,
                saldoLast,
                creditCardNumber,
                currency,
                year,
                hasDecemberAndJanuaryDates);
            maybeEntry.has_value()) {
            entries.emplace_back(*maybeEntry);
            saldoLast = entries.back().saldo;
        }
        else {
            Q_ASSERT(false);
        }
        qWarning() << "*cit after2: " <<*cit;
        if(isEndOfPageEntries(*cit)) {
            ++cit;

            // End of last page reached
            if(containsSum(*cit)) {
                saldoLast = entries.back().saldo;
                // Sanity check. Calculated saldo should be same as the one in
                // the file
                qWarning() << "extractAmount(*cit): " << extractAmount(*cit);
                qWarning() << "saldoLast: " << saldoLast;
                Q_ASSERT(extractAmount(*cit) == saldoLast);

                if(saldoLast != "0,00") {
                    const auto maybeEntryCompensation =
                        extractEntryCompensation(rows, saldoLast, creditCardNumber, currency);
                    if(!maybeEntryCompensation) {
                        qWarning() << "Entry compensation could not be created";
                        return  {};
                    }
                    entries.emplace_back(*maybeEntryCompensation);
                }
                return entries;
            }
            // Not the last page reached so try to go to the next page
            // We have to go to the next page because same string is in
            // this line == end of first page
            // line of next page
            ++cit;
            tryGoToNextPageEntries(cit, rows);
            Q_ASSERT(cit != rows.cend());
            continue;
        }
        qWarning() << "*cit after3: " <<*cit;
        ++cit;
        qWarning() << "*cit after4: " <<*cit;
    }

//    // Run until end of document
//    for(;;) {
//        for(;;) {
//            // Run until current page is finnished
//            if(isEndOfPageEntries(*cit)) {
//                break;
//            }
//            if(isEndOfPage(*cit)) {
//                ++cit;
//                saldoLast = entries.back().saldo;
//                // Sanity check. Calculated saldo should be same as the one in
//                // the file
//                qWarning() << "extractAmount(*cit): " << extractAmount(*cit);
//                qWarning() << "saldoLast: " << saldoLast;
//                Q_ASSERT(extractAmount(*cit) == saldoLast);

//                if(saldoLast != "0,00") {
//                    const auto maybeEntryCompensation =
//                            extractEntryCompensation(rows, saldoLast, creditCardNumber, currency);
//                    if(!maybeEntryCompensation) {
//                        qWarning() << "Entry compensation could not be created";
//                        return  {};
//                    }
//                    entries.emplace_back(*maybeEntryCompensation);
//                }

//                return entries;
//            }
//            if(const auto& maybeEntry =
//                extractEntry(
//                        cit,
//                        saldoLast,
//                        creditCardNumber,
//                        currency,
//                        year,
//                        hasDecemberAndJanuaryDates);
//                maybeEntry.has_value()) {
//                entries.emplace_back(*maybeEntry);
//                saldoLast = entries.back().saldo;
//            }
//            ++cit;
//            if(cit == rows.end()) {
//                break;
//            }
//        }
//        tryGoToNextPageEntries(cit, rows);
//    }
    Q_ASSERT(false);
    return {};
}


QTextStream& operator<<(QTextStream& os, const EntryCreditCard& obj)
{
    constexpr char separator = ';';

    os << obj.creditCardNumber   << separator<<
        obj.dateFirst << separator<<
        obj.dateSecond << separator<<
        obj.amount << separator<<  // TODO maybe we need to list conversion here instead
        obj.currency << separator<< // TODO maybe we need to list original currency here instead
        "1.00" << separator<<       // TODO maybe we need to list conversion factor
        obj.amount << separator<<
        obj.currency << separator<<
        obj.payee<< separator<< // Transaktionsbeschreibung = payee
        obj.bookingReason;  //"Transaktionsbeschreibung Zusatz"

    for(int i=0; i<6; ++i) {
        os << separator;
    }

        // skip remaining because an empty ; would confuse excel

//       << separator<<
//        "" << separator<<  // ??? unclear were we get the number from
//        "" << separator<<
//        "" << separator<<
//        "" << separator<<
//        "" << separator<<
//        "";  // ??? unclear were we get the number from

    return os;
}

}
