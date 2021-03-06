"""
Module to represent one credit card booking entry in the bookings
"""

from typing import List

import re

from bankconv.date import Date


class BookingEntry:
    def __init__(
        self,
        credit_card_number: str,
        currency: str,
        line: str,
        start_date: Date = None,  # None or str
        end_date: Date = None,  # None or str
    ):
        self.credit_card_number = credit_card_number
        self.currency = currency
        if end_date is None:
            (
                booking_day_month,
                recite_day_month,
            ) = self._get_booking_and_recite_day_month(line)

            self.booking_date = self._make_date(booking_day_month, start_date)
            self.recite_date = self._make_date(recite_day_month, start_date)
        else:
            self.booking_date = end_date
            self.recite_date = self.booking_date
        self.description = self._get_description(line)
        self.description_addition = ""
        self.amount = self._get_amount(line)

    def _get_booking_and_recite_day_month(self, line: str) -> List[str]:
        """
        Searches line for booking and recite day month.
        Return booking and recite date in format DD.MM. as List.
        Assumes that line contains valid booking and recite date.
        """
        match = re.match(r"\d{2}.\d{2}. \d{2}.\d{2}.", line)
        assert match
        dates = match.group(0).split()
        assert len(dates) == 2, "booking or recite date missing"
        return [dates[0], dates[1]]

    def _get_month(self, day_month: str):
        """
        Extract Month from day month in format DD.MM.
        """
        return day_month[-3:-1]

    def _make_date(self, day_month: str, start_date: Date) -> Date:
        year = start_date.year
        if start_date.month == "12" and self._get_month(day_month) == "01":
            year = str(int(year) + 1)
        return Date(day_month + year)

    def _get_description(self, line: str) -> str:
        """
        Searches line for description and returns it.
        """
        date_match = re.match(r"\d{2}.\d{2}. \d{2}.\d{2}.", line)
        if date_match:
            line = line[date_match.end() :]
        line = line.lstrip()

        match = re.search(r"\d{1,3}(.\d{3})*,\d{2}(\+|-){1}", line)
        line = line[: match.start()]
        return line.rstrip()

    def _get_amount(self, line: str) -> str:
        """
        Searches line for amount in the format
        D+,DD(+/-)
        and returns it as
        (-?)D+.DD
        """
        match = re.search(r"\d{1,3}(.\d{3})*,\d{2}(\+|-){1}", line)
        assert match, "line: {}".format(line)
        amount = match.group(0)
        last_char = amount[-1]
        if amount[-1] == "-":
            return "-" + amount[:-1]
        return amount[:-1]

    def __repr__(self) -> str:
        return self._get_class_presentation()

    def __str__(self) -> str:
        return self._get_class_presentation()

    def _get_class_presentation(self) -> str:
        """
        Helper to show member of class for __str__ and __repr__
        """
        print_str: str = ""
        print_str += "{} ".format(self.credit_card_number)
        print_str += "{} ".format(self.booking_date)
        print_str += "{} ".format(self.recite_date)
        print_str += "{}\t".format(self.currency)
        print_str += "{}\t".format(self.amount)
        print_str += "{} ".format(self.description)
        if self.description_addition != "":
            print_str += "{} ".format(self.description_addition)
        return print_str
