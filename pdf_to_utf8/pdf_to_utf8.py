#!/usr/bin/env python3

import sys

from tika import parser

def pdfToUTF8(argv):
    print(f"Arguments count: {len(argv)}")
    if len(argv) != 2:
        return -1

    print(str(parser.from_file(argv[1])))

    return 0


if __name__ == "__main__":
    pdfToUTF8(sys.argv)
