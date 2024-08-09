#!/usr/bin/env python
# -*- coding: utf8 -*-


""" Парсинг файлов utmp, wtmp, btmp

Данный скрипт файлы utmp, wtmp, btmp используемые в системах UNIX, 
возвращает csv файл с описанием типов входов в систему.
python version 3.12.4

"""
__author__ = "Андрей Кравцов"
__version__ = "1.0"
__annotations__

from ctypes import LittleEndianStructure, c_uint32, c_uint16, c_char
from socket import inet_ntoa
from pathlib import Path
import csv
from datetime import datetime, timezone
import argparse
import sys


# Типы входов
TypesOfLogin = {0: 'EMPTY UT_UNKNOWN',
                1: 'RUN_LVL',
                2: 'BOOT_TIME',
                3: 'NEW_TIME',
                4: 'OLD_TIME',
                5: 'INIT_PROCESS',
                6: 'LOGIN_PROCESS',
                7: 'USER_PROCESS',
                8: 'DEAD_PROCESS',
                9: 'ACCOUNTING'}

class UtmpLoginRecords (LittleEndianStructure):
    """Структура btmp, utmp, wtmp"""
    _pack_=1
    _fields_ = [('type_of_login', c_uint32),
                ('PID', c_uint32), 
                ('terminal', c_char * 32),
                ('terminal_identifier', c_uint32),
                ('username', c_char * 32),
                ('hostname', c_char * 256),
                ('termination_status', c_uint16),
                ('exit_status', c_uint16),
                ('session', c_uint32),
                ('timestamp', c_uint32),
                ('microseconds', c_uint32),
                ('ip_address', c_char * 16),
                ('unknown', c_char * 20)]

def folder_scan(path_: str, name_list: list, err_list: list) -> list:
    """Recursive directory traversal and returns a list of files"""
    try:
        for i in Path(path_).glob('**/*'):
            if Path(i).is_file():
                name_list.append(str(i))
        return name_list
    except PermissionError as PE_1:
        err_list.append(PE_1)
    except FileNotFoundError as FNF_1:
        err_list.append(FNF_1)
    except ValueError as VE_1:
        err_list.append(VE_1)

def unix_to_human (udate: int) -> str:
    hdate = datetime.fromtimestamp(udate, timezone.utc)
    return hdate.strftime('%d-%m-%Y %H:%M:%S')

def utmp_to_csv(input_file: str, output_file: str):
    """Принимает в качестве аргумента путь к файлу utmp, wtmp, btmp
    создает файл csv.

    Args:
        file (str): путь к файлу 
    """
    with open (output_file + '.csv', 'w', newline='') as csv_f:
        writer = csv.writer(csv_f, delimiter=';')
        writer.writerow(['type_of_login',
                         'PID', 'terminal',
                         'terminal_identifier',
                         'username',
                         'hostname',
                         'termination_status',
                         'exit_status',
                         'session',
                         'timestamp UTC+0',
                         'microseconds',
                         'ip_address',
                         'unknown'])

        with open(input_file, 'rb') as f:
            record = UtmpLoginRecords()
            while True:
                buf = f.readinto(record)
                if buf == 0:
                    break
                try:
                    ip = inet_ntoa(record.ip_address)
                except OSError:
                    ip = record.ip_address

                writer.writerow([TypesOfLogin[record.type_of_login],
                                record.PID,
                                record.terminal.decode("utf-8"),
                                record.terminal_identifier,
                                record.username.decode("utf-8"),
                                record.hostname.decode("utf-8"),
                                record.termination_status,
                                record.exit_status,
                                record.session,
                                unix_to_human(record.timestamp),
                                record.microseconds,
                                ip,
                                record.unknown.decode("utf-8")])

if __name__== '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', type=str, help='input file utmp, wtmp, btmp')
    parser.add_argument('-o', type=str, help='output file, default file result.csv', default='result')
    args = parser.parse_args()
    
    input_file = ''
    output_file = ''

    if not args.i:
        print('Default result.csv')
        print('Input file utmp or wtmp or btmp: ', end='')
        input_file = input()
    elif not args.o:
        output_file = 'result'
    else:
        input_file = str(args.i).replace('"','')
        output_file = str(args.o).replace('"',"").replace('.csv','').replace( '.csv','')
        if not Path(input_file).is_file() or Path(output_file).is_file():
            print('args error, app close!')
            sys.exit()

    utmp_to_csv(input_file, output_file)
    print('done')

