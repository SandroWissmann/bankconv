#ifndef BANKCONV_EXTRACTENTRIESCREDITCARDTFBANK_HPP
#define BANKCONV_EXTRACTENTRIESCREDITCARDTFBANK_HPP

#include <QtCore/QString>

#include <vector>

namespace bankconv {

class EntryCreditCard;

std::vector<EntryCreditCard> extractEntriesCreditCardTfBank(const std::vector<QString> &rows);

}


#endif
