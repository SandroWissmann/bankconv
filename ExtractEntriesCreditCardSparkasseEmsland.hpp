#ifndef BANKCONV_EXTRACTENTRIESCREDITCARDSPARKASSEEMSLAND_HPP
#define BANKCONV_EXTRACTENTRIESCREDITCARDSPARKASSEEMSLAND_HPP

#include <QtCore/QString>

#include <vector>

namespace bankconv {

class EntryCreditCard;

std::vector<EntryCreditCard> extractEntriesCreditCardSparkasseEmsland(const std::vector<QString> &rows);

}


#endif
