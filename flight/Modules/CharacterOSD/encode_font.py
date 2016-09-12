#!/usr/bin/env python

import sys

def main(args):
    if (len(args) != 2):
        print("usage: %s fontfile.mcm")
        print("outputs to stdout")
        return

    with open(args[1]) as f:
        font_data = f.readlines()[1:]

    print("/* autogenerated from %s */"%(args[1]))

    for line in font_data:
        print('\t0b' + line.strip() + ',')

if __name__ == '__main__':
    main(sys.argv)

