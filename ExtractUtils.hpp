#ifndef BANKCONV_EXTRACTUTILS_HPP
#define BANKCONV_EXTRACTUTILS_HPP

#include <QtCore/QString>

#include <optional>
#include <vector>

namespace bankconv {

QString extractAmount(QString row);

QString calculateSaldo(QString saldoLast, QString amount);

std::optional<QString> findRowWhoStartsWith(
    const std::vector<QString>& rows,
    const std::vector<QString>& startStrings);

}

#endif
