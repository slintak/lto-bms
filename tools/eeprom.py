#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
from struct import pack

def validator_uint16(astring):
    try:
        value = int(astring)
    except ValueError:
        try:
            value = int(astring, 16)
        except ValueError:
            raise argparse.ArgumentTypeError("Argument should be unsigned int!")

    if value < 0 or value > 0xFFFF:
        raise argparse.ArgumentTypeError("Argument is 16-bit integer. Allowed values 0-65535.")

    return value


def validator_int16(astring):
    try:
        value = int(astring)
    except ValueError:
        try:
            value = int(astring, 16)
        except ValueError:
            raise argparse.ArgumentTypeError("Argument should be signed int!")

    if value < -32768 or value > 32767:
        raise argparse.ArgumentTypeError("Argument is 16-bit integer. Allowed values -32768-32767.")

    return value


MAGIC_NUMBER = 0x7337
CHEMISTRY_CHOICES = ['LTO', 'NAION']

DEFAULTS_LTO = {
    'sn': {
        'value': 0x0000,
        'required': True,
        'help': 'Unique serial number of the BMS.',
    },
    'temp-offset': {
        'value': 0,
        'validator': validator_int16,
        'help': 'Temperature offset for the internal sensor.',
    },
    'ovlo-cutoff': {
        'value': 2800,  # mV
        'help': 'Over-voltage cutoff. Unit: mV.',
    },
    'ovlo-release': {
        'value': 2700,  # mV
        'help': 'Over-voltage release. Unit: mV.',
    },
    'uvlo-release': {
        'value': 1800,  # mV
        'help': 'Under-voltage release. Unit: mV.',
    },
    'uvlo-cutoff': {
        'value': 1700,  # mV
        'help': 'Under-voltage cutoff. Unit: mV.',
    },
    'max-current': {
        'value': 1000,  # mA
        'help': 'Maximum allowed current. Unit: mA.',
    },
    'oclo-timeout': {
        'value': 10,  # seconds
        'help': 'Time to hold over-current lockout. Unit: seconds.',
    },
}

DEFAULTS_NAION = {
    **DEFAULTS_LTO,
    'ovlo-cutoff': {
        'value': 3900,  # mV
        'help': 'Over-voltage cutoff. Unit: mV.',
    },
    'ovlo-release': {
        'value': 3800,  # mV
        'help': 'Over-voltage release. Unit: mV.',
    },
    'uvlo-release': {
        'value': 2200,  # mV
        'help': 'Under-voltage release. Unit: mV.',
    },
    'uvlo-cutoff': {
        'value': 2000,  # mV
        'help': 'Under-voltage cutoff. Unit: mV.',
    },
}

DEFAULTS_BY_CHEMISTRY = {
    'LTO': DEFAULTS_LTO,
    'NAION': DEFAULTS_NAION,
}


def make_args(parser, defaults):
    for k, v in defaults.items():
        parser.add_argument(
            f'--{k}',
            type=v.get('validator', validator_uint16),
            action='store',
            required=v.get('required', False),
            default=v['value'],
            help=v['help'],
        )


def crc16_byte(ch, crc):
    '''
    Calculate CRC16 for one byte.
    '''
    for i in range(8):
        if ((ch & 0x01) ^ (crc & 0x0001)) != 0x00:
            crc = (crc >> 1) ^ 0xA001
        else:
            crc = crc >> 1
        ch = ch >> 1
    return crc


def crc16(data):
    '''
    Return CRC16 for byte array.
    '''
    crc = 0xFFFF
    for d in data:
        crc = crc16_byte(d, crc)
    return crc


def main():
    base_parser = argparse.ArgumentParser(add_help=False)
    base_parser.add_argument(
        '--chemistry',
        choices=CHEMISTRY_CHOICES,
        default='LTO',
        help='Battery chemistry preset (affects default limits).',
    )
    base_args, _ = base_parser.parse_known_args()
    defaults = DEFAULTS_BY_CHEMISTRY[base_args.chemistry]

    parser = argparse.ArgumentParser(description='EEPROM configuration generator for LTO/NAION-BMS firmware.')
    parser.add_argument(
        '--chemistry',
        choices=CHEMISTRY_CHOICES,
        default=base_args.chemistry,
        help='Battery chemistry preset (affects default limits).',
    )
    make_args(parser, defaults)
    parser.add_argument(
        '-f', '--file',
        type=str,
        action='store',
        default=None,
        help='Filename to write a binary data.'
    )
    args = parser.parse_args()

    eep = pack(
        "<2Hh6H",
        MAGIC_NUMBER,
        args.sn,
        args.temp_offset,
        args.ovlo_cutoff,
        args.ovlo_release,
        args.uvlo_release,
        args.uvlo_cutoff,
        args.max_current,
        args.oclo_timeout,
    )

    eep = eep + crc16(eep).to_bytes(2, byteorder='little')

    if args.file:
        with open(args.file, 'wb') as fw:
            fw.write(eep)
    else:
        print(eep.hex())


if __name__ == '__main__':
    main()
