#!/usr/bin/env python3

import os

from typing import List

import tkinter as tk
import tkinter.filedialog

import pdf_parser

from bankconv.credit_card.bookings import Bookings as CreditCardBookings


def get_directory() -> str:
    """
    Select directory from GUI User input and return it.
    """
    # get directory
    # root = tk.Tk()
    # root.withdraw()
    # directory = tk.filedialog.askdirectory()
    directory = "/mnt/Buisness/Haushalt/Kreditkartenabrechnungen Sandro"
    return directory


def make_credit_card_booking_csv_from_directory(directory: str):
    """
    Scans folder for PDF which contain booking data from credit card.
    Merges all the data into one CSV file. Similar to the auto export data
    you can download for recent dates from Sparkasse
    """
    credit_card_bookings: CreditCardBookings = CreditCardBookings()
    filenames_pdf: List[str] = os.listdir(directory)
    filenames_pdf.sort()
    for filename_pdf in filenames_pdf:
        if not filename_pdf.lower().endswith(".pdf"):
            continue

        text_lines: List[str] = pdf_parser.get_text_lines_from_pdf(
            directory, filename_pdf
        )

        if not credit_card_bookings.add_monthly_billing_data(text_lines):
            print("File: {}".format(filename_pdf))

        filename_txt_absolute: str = pdf_parser.get_filename_txt_absolute(
            directory, filename_pdf
        )

    credit_card_bookings.write_to_directory(directory)


def main():
    directory: str = get_directory()

    make_credit_card_booking_csv_from_directory(directory)


if __name__ == "__main__":
    main()
