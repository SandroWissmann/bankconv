#ifndef BANKCONV_EXTRACTENTRIESCHECKINGACCOUNTSPARKASSE_HPP
#define BANKCONV_EXTRACTENTRIESCHECKINGACCOUNTSPARKASSE_HPP

#include <QtCore/QString>

#include <vector>

namespace bankconv {

class EntryCheckingAccount;

std::vector<EntryCheckingAccount> extractEntriesCheckingAccountSparkasseEmsland(const std::vector<QString>& rows);

}


#endif
