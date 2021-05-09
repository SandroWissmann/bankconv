from typing import List
from typing import Union
from typing import Optional

import os
import re
import csv

from bankconv.credit_card_entry import CreditCardEntry
from bankconv.date import Date


class CreditCardBilling:
    def __init__(self):
        self.credit_card_entries: List[CreditCardEntry] = []

    def add_monthly_billing_data(self, text_lines: List[str]) -> bool:
        index, end_date = self._get_end_date(text_lines)
        if end_date is None:
            print("Error: End date not found in file")
            return False

        index_credit_card_number_list = []
        index_credit_card_number_list = self._get_credit_card_number(
            text_lines, index + 1
        )
        if index_credit_card_number_list is None:
            print("Error: Credit card number not found in file")
            return False
        index = index_credit_card_number_list[0]
        credit_card_number = index_credit_card_number_list[1]

        index_currency_list = []
        index_currency_list = self._get_currency(text_lines, index + 1)
        if index_currency_list is None:
            print("Error: Currency not found in file")
            return False
        index = index_currency_list[0]
        currency = index_currency_list[1]

        index_start_date_list = []
        index_start_date_list = self._get_start_date(
            text_lines, index + 1, end_date
        )
        if index_start_date_list is None:
            print("Error: Start date not found in file")
            return False
        index = index_start_date_list[0]
        start_date = index_start_date_list[1]

        start_year = start_date.year

        index, credit_card_entries = self._get_credit_card_entries(
            text_lines, index + 1, credit_card_number, currency, start_year
        )

        credit_card_compensation = self._get_credit_card_compensation(
            text_lines, index + 1, credit_card_number, currency, end_date
        )
        if credit_card_compensation is None:
            print("Error: No compensation found")
            return False

        self.credit_card_entries += credit_card_entries

        self.credit_card_entries.append(credit_card_compensation)
        return True

    def write_to_directory(self, directory: str):
        """
        Writes credit card entries to directory as CSV.
        """

        filename_absolute: str = os.path.join(
            directory, "credit_card_entries.csv"
        )

        with open(filename_absolute, mode="w") as credit_card_entries_file:
            writer = csv.writer(
                credit_card_entries_file,
                delimiter=",",
                quotechar='"',
                quoting=csv.QUOTE_MINIMAL,
            )

            # writes the with same descriptions as export from
            # sparkasse so skrooge works directly with it
            writer.writerow(
                [
                    "Umsatz get�tigt von",  # credit card number
                    "Belegdatum",  # recite date
                    "Buchungsdatum",  # booking date
                    "Originalw�hrung",  # currency
                    "Buchungsbetrag",  # amount
                    "Transaktionsbeschreibung",  # description
                    "Transaktionsbeschreibung Zusatz",  # description addition
                ]
            )

            for credit_card_entry in self.credit_card_entries:
                writer.writerow(
                    [
                        credit_card_entry.credit_card_number,
                        credit_card_entry.recite_date,
                        credit_card_entry.booking_date,
                        credit_card_entry.currency,
                        credit_card_entry.amount,
                        credit_card_entry.description,
                        credit_card_entry.description_addition,
                    ]
                )

    def _get_end_date(
        self, text_lines: List[str]
    ) -> Optional[List[Union[int, Date]]]:
        """
        Search for end date in text lines
        Returns line number and end date or none if not result
        """
        for line_number, text_line in enumerate(text_lines):
            if text_line.startswith("Abrechnung / Saldenmitteilung bis zum"):
                end_date = self._find_date(text_line)
                if type(end_date) is not Date:
                    return None
                return [line_number, end_date]
        return None

    def _find_date(self, line: str) -> Optional[Date]:
        """
        Search line for date in the format DD.MM.YYYY
        """
        match = re.search(r"\d{2}.\d{2}.\d{4}", line)
        if match:
            return Date(match.group(0))
        return None

    def _get_credit_card_number(
        self, text_lines: List[str], start_line: int
    ) -> Optional[List[Union[int, str]]]:
        """
        Search for credit_card_number in text lines
        Returns line number and credit_card_number or None if not result
        """
        for line_number, text_line in enumerate(
            text_lines[start_line:], start_line
        ):
            if not text_line.startswith(
                "MasterCard"
            ) and not text_line.startswith("Mastercard"):
                continue

            credit_card_number = self._find_credit_card_number(text_line)
            if type(credit_card_number) is not str:
                return None
            credit_card_number: str = self._reformat_credit_card_number(
                credit_card_number
            )
            return [line_number, credit_card_number]

    def _find_credit_card_number(self, line: str) -> Optional[str]:
        """
        Search line for credit card number in the format
        DDDD DDXX XXXX DDDD.
        """
        match = re.search(r"\d{4} \d{2}XX XXXX \d{4}", line)
        if match:
            return match.group(0)
        return None

    def _reformat_credit_card_number(self, credit_card_number) -> str:
        """
        Reformats credit card number from Format
        DDDD DDXX XXXX DDDD to DDDD **** **** DDDD.
        Reason for this is that the automatic CSV export from Sparkasse
        is in DDDD **** **** DDDD and not DDDD DDXX XXXX DDDD.
        """
        parts: List[str] = credit_card_number.split()
        assert len(parts) == 4, "Credit card parts should be 4"
        return parts[0] + " **** **** " + parts[3]

    def _get_currency(
        self, text_lines: List[str], start_line: int
    ) -> Optional[List[Union[int, str]]]:
        """
        Search for currency in text lines
        Returns line number and currency or None if not result
        """
        for line_number, text_line in enumerate(
            text_lines[start_line:], start_line
        ):
            if text_line.find("EUR") != -1:
                return [line_number, "EUR"]
        return None

    def _get_start_date(
        self, text_lines: List[str], start_line: int, end_date: Date
    ) -> Optional[List[Union[int, Date]]]:
        """
        Search for start date in text lines
        Returns line number and start date or none if not result
        """
        for line_number, text_line in enumerate(
            text_lines[start_line:], start_line
        ):
            if text_line.find("Saldovortrag vom") != -1:
                start_date = self._find_date(text_line)
                if type(start_date) is not Date:
                    return None
                return [line_number, start_date]
            # this is a hack. In one file from Sparkasse the date was missing
            # so we assume it is end date - 1 month
            if text_line.find("Saldovortrag") != -1:
                start_date = end_date
                start_date.decrement_month()
                return [line_number, start_date]
        return None

    def _get_credit_card_entries(
        self,
        text_lines: List[str],
        start_line: int,
        credit_card_number: str,
        currency: str,
        start_year: str,
    ) -> List[Union[int, List[CreditCardEntry]]]:
        """
        Search for credit card entries in text lines
        Returns credit card entries and line_number
        """
        credit_card_entries: List[CreditCardEntry] = []

        found_entry: bool = False
        for line_number, text_line in enumerate(
            text_lines[start_line:], start_line
        ):
            if self._is_end_of_booking_line(text_line):
                break
            if text_line.isspace():
                found_entry = False
                continue
            if not self._is_credit_card_entry(text_line):
                if found_entry:
                    description_addition = text_line.strip()
                    if credit_card_entries[-1].description_addition != "":
                        credit_card_entries[-1].description_addition += "\t"
                    credit_card_entries[
                        -1
                    ].description_addition += description_addition
                else:
                    print("Unknown: {}".format(text_line))
                continue
            found_entry = True
            credit_card_entry = CreditCardEntry(
                credit_card_number, currency, text_line, start_year, None
            )
            credit_card_entries.append(credit_card_entry)

        return [line_number, credit_card_entries]

    def _get_credit_card_compensation(
        self,
        text_lines: List[str],
        start_line: int,
        credit_card_number: str,
        currency: str,
        end_date: str,
    ) -> Optional[CreditCardEntry]:
        """
        Searches text lines for special credit card entry after the normal
        entries which represents the compensation of the credit card.
        """
        for line_number, text_line in enumerate(
            text_lines[start_line:], start_line
        ):
            if text_line.find("Einzug von Kto.") == -1:
                continue
            return CreditCardEntry(
                credit_card_number, currency, text_line, None, end_date
            )
        return None

    def _is_end_of_booking_line(self, line: str) -> bool:
        """
        Returns true if end of booking line is found
        """
        return line.lstrip().startswith("--------------")

    def _is_credit_card_entry(self, line: str) -> bool:
        """
        Checks if passed line is a credit card enty in the format:
        DD.MM. DD.MM.   Description    Amount (+ or -)
        """
        match = re.match(
            r"\d{2}.\d{2}. \d{2}.\d{2}. {2}.+\d{1,3}(.\d{3})*,\d{2}(\+|-){1}",
            line,
        )
        return bool(match)
