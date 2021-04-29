#!/usr/bin/env python3

from typing import List

import tkinter as tk
import tkinter.filedialog

import os

from tika import parser


def getDirectory() -> str:
    """
    Select directory from GUI User input and return it.
    """
    # get directory
    # root = tk.Tk()
    # root.withdraw()
    # directory = tk.filedialog.askdirectory()
    directory = "/mnt/Buisness/Haushalt/Kreditkartenabrechnungen Sandro"
    return directory


def getTextLinesFromPDF(directory: str, filename_pdf: str) -> List[str]:
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
    text = text.replace(r"\xc3\x9f", "ß")

    return text.split("\\n")


def getFilenameTxtAbsolut(directory: str, filename_pdf: str) -> str:
    """
    Get txt filename absolut based on pdf filename.
    """
    filename_txt: str = os.path.splitext(filename_pdf)[0] + ".txt"
    filename_txt_absolut: str = os.path.join(directory, filename_txt)
    return filename_txt_absolut


def writeTextLinesToFileInDir(
    filename_txt_absolut: str, text_lines: List[str]
) -> None:
    """
    Write text lines to file.
    """
    text_file: file = open(filename_txt_absolut, "w")
    for text_line in text_lines:
        text_file.write(text_line)
        text_file.write("\n")
    text_file.close()


def main():
    # iterate over all pdf files in folder
    directory: str = getDirectory()

    for filename_pdf in os.listdir(directory):
        if not filename_pdf.lower().endswith(".pdf"):
            continue

        text_lines: List[str] = getTextLinesFromPDF(directory, filename_pdf)

        filename_txt_absolut: str = getFilenameTxtAbsolut(
            directory, filename_pdf
        )

        writeTextLinesToFileInDir(filename_txt_absolut, text_lines)

    # create attach each pdf to an excel file going by year


if __name__ == "__main__":
    main()
