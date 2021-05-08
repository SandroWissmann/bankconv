#!/usr/bin/env python3

import tkinter as tk
import tkinter.filedialog

import os

from tika import parser

from bankconv.credit_card_billing import CreditCardBilling


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


def get_text_lines_from_pdf(directory: str, filename_pdf: str) -> List[str]:
    """
    Extract Text from PDF file.
    Requires input to be a pdf file.
    Returns dict with lines of text in PDF file.
    """
    filename_pdf_absolut: str = os.path.join(directory, filename_pdf)

    raw: str = str(parser.from_file(filename_pdf_absolut))
    text: str = str(raw.encode("utf-8", errors="ignore"))

    text = text.replace(r"\xc3\xa4", "ä")
    text = text.replace(r"\xc3\xb6", "ö")
    text = text.replace(r"\xc3\xbc", "ü")
    text = text.replace(r"xc3x9c", "Ü")
    text = text.replace(r"\xc3\x9f", "ß")

    text_lines: List[str] = text.split("\\n")
    text_lines = [s.replace("\\", "") for s in text_lines]
    return text_lines


def get_filename_txt_absolute(directory: str, filename_pdf: str) -> str:
    """
    Get txt filename absolute based on pdf filename.
    """
    filename_txt: str = os.path.splitext(filename_pdf)[0] + ".txt"
    filename_txt_absolute: str = os.path.join(directory, filename_txt)
    return filename_txt_absolute


def write_text_lines_to_file_in_dir(
    filename_txt_absolute: str, text_lines: List[str]
) -> None:
    """
    Write text lines to file.
    """
    text_file: file = open(filename_txt_absolute, "w")
    for text_line in text_lines:
        text_file.write(text_line)
        text_file.write("\n")
    text_file.close()


def main():
    directory: str = get_directory()

    credit_card_billing: CreditCardBilling = CreditCardBilling()

    filenames_pdf: List[str] = os.listdir(directory)
    filenames_pdf.sort()
    for filename_pdf in filenames_pdf:
        if not filename_pdf.lower().endswith(".pdf"):
            continue

        text_lines: List[str] = get_text_lines_from_pdf(
            directory, filename_pdf
        )

        credit_card_billing.add_monthly_billing_data(text_lines)

        filename_txt_absolute: str = get_filename_txt_absolute(
            directory, filename_pdf
        )

        write_text_lines_to_file_in_dir(filename_txt_absolute, text_lines)

    credit_card_billing.write_to_directory(directory)


if __name__ == "__main__":
    main()
