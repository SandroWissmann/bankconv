#include "EntryUtils.hpp"

#include <QtCore/QList>
#include <QtCore/QRegularExpression>

namespace bankconv {

namespace {
bool isAmountWithSignAtTheEnd(const QString &amount)
{
    static QRegularExpression re{R"(^\d+,\d{2}(\+?|-)$)"};
    if(re.match(amount).hasMatch()) {
        return true;
    }
    return false;
}

double calculateSaldo(double saldoLast, double amount)
{
    return saldoLast + amount;
}
}

QString extractAmount(QString row)
{
    row = row.simplified();
    const auto enrties = row.split(' ');
    auto amount = enrties.back();
    amount = amount.remove('.');

    if(isAmountWithSignAtTheEnd(amount)) {
        const auto token = amount.back();
        if (token == '-') {
            amount.push_front(token);
            amount.removeLast();
        }
        if(token == '+') {
            amount.removeLast();
        }
        return amount;
    }
    // sign at the front
    if(!amount.startsWith('-')) {
        amount.push_front('+');
    }
    return amount;
}

QString calculateSaldo(QString saldoLast, QString amount)
{
    saldoLast.remove('.');
    saldoLast.replace(',', '.');
    amount.remove('.');
    amount.replace(',', '.');

    bool saldoLastDoubleOk{};
    const auto saldoLastDouble =saldoLast.toDouble(&saldoLastDoubleOk);
    qWarning() << "saldoLastDouble: " << saldoLastDouble;
    Q_ASSERT(saldoLastDoubleOk);
    bool amountDoubleOk{};
    const auto amountDouble = amount.toDouble(&amountDoubleOk);
    qWarning() << "amountDouble: " << amountDouble;
    Q_ASSERT(amountDoubleOk);
    const auto saldoAsDouble =
        calculateSaldo(saldoLastDouble, amountDouble);
    auto saldo = QString::number(saldoAsDouble, 'f', 2);
    saldo.replace('.', ',');
    return saldo;
}

std::optional<QString> findRowWhoStartsWith(
    const std::vector<QString>& rows,
    const std::vector<QString>& startStrings)
{
    const auto it = std::ranges::find_if(rows, [startStrings](const QString& row){
        for(const auto& startString : startStrings) {
            if(row.startsWith(startString)) {
                return true;
            }
        }
        return false;
    });

    if(it == rows.end()) {
        return {};
    }
    return *it;
}


}
