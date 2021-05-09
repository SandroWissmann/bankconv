import re


class Date:
    def __init__(self, datestr: str):
        """
        Expects str in format DD.MM.YYYY
        """
        if not re.match(r"\d{2}.\d{2}.\d{4}", datestr):
            print("Invalid date string: {}".format(datestr))
        self.day = datestr[0:2]
        self.month = datestr[3:5]
        self.year = datestr[6:]

    def decrement_month(self):
        """
        Decrements month by one month.
        """
        month: int = int(self.month)
        month -= 1
        if month == 0:
            month == 12
            year: int = int(self.year)
            year -= 1
            self.year = str(year)
        self.month = str(month)
        if len(self.month) == 1:
            self.month = "0" + self.month

    def __repr__(self) -> str:
        return self._get_class_presentation()

    def __str__(self) -> str:
        return self._get_class_presentation()

    def _get_class_presentation(self) -> str:
        """
        Helper for __str__ and __repr__.
        Prints in format DD.MM.YY
        """
        return "{}.{}.{}".format(self.day, self.month, self.year[-2:])
