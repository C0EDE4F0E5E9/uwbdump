#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>
#include <getopt.h>
#include <ctime>

// Типы входов в систему
#define UT_UNKNOWN 0
#define RUN_LVL 1
#define BOOT_TIME 2
#define NEW_TIME 3
#define OLD_TIME 4
#define INIT_PROCESS 5
#define LOGIN_PROCESS 6
#define USER_PROCESS 7
#define DEAD_PROCESS 8
#define ACCOUNTING 9

// Размеры строк в структуре utmp
#define UT_LINESIZE 32
#define UT_NAMESIZE 32
#define UT_HOSTSIZE 256

struct exit_status
// Структура для хранения информации о завершении процесса
{
    short int e_termination; /* Статус завершения процесса. */
    short int e_exit;        /* Код завершения процесса. */
};

struct ut_tv
// Структура для хранения информации о времени входа
{
    int32_t tv_sec;  /* Секунды */
    int32_t tv_usec; /* Миллисекунды */
}; /* Время создания записи */

struct utmp
// Структура записи в файлах utmp, wtmp, btmp
{
    short ut_type;              /* Тип входа */
    uint32_t ut_pid;            /* PID процесса входа */
    char ut_line[UT_LINESIZE];  /* Терминал входа - "/dev/" */
    char ut_id[4];              /* Идентификатор терминала или сокращенное имя ttyname */
    char ut_user[UT_NAMESIZE];  /* Имя пользователя для входа */
    char ut_host[UT_HOSTSIZE];  /* Имя хоста для удаленного входа */
    struct exit_status ut_exit; /* Код завершения процесса, помеченного как DEAD_PROCESS. */
    uint32_t ut_session;        /* идентификатор сеанса, используемый для оконного режима */
    struct ut_tv ut_tv;         /* Время создания записи. */
    uint32_t ut_addr_v6[4];     /* IP-адрес удаленного хоста (IPv4) */
    char pad[20];               /* Зарезервировано для будущего использования. */
};

// Типы входов для заполнения файла csv
std::string TypeOfLogin[] = {
    "UNKNOWN",
    "RUN_LVL",
    "BOOT_TIME",
    "NEW_TIME",
    "OLD_TIME",
    "INIT_PROCESS",
    "LOGIN_PROCESS",
    "USER_PROCESS",
    "DEAD_PROCESS",
    "ACCOUNTING",
};

std::string intToIpV4(uint32_t integer)
// Функция преобразует 32-битное целое число в строку IP-адреса.
// Входные параметры: integer - 32-битное целое число.
// Выходные параметры: IP-адрес в виде строки
// Для инвертирования 32-битного числа используется функция GCC __builtin_bswap32
{
    integer = __builtin_bswap32(integer);
    int octet1 = (integer >> 24) & 0xFF;
    int octet2 = (integer >> 16) & 0xFF;
    int octet3 = (integer >> 8) & 0xFF;
    int octet4 = integer & 0xFF;

    return std::to_string(octet1) + "." + std::to_string(octet2) + "." +
           std::to_string(octet3) + "." + std::to_string(octet4);
}

std::string DateTime()
{
    time_t now = time(0);
    struct tm *lt = localtime(&now);
    return std::to_string(lt->tm_year + 1900) + "-" + std::to_string(lt->tm_mon + 1) + "-" + std::to_string(lt->tm_mday) + "_" + std::to_string(lt->tm_hour)  + std::to_string(lt->tm_min) + std::to_string(lt->tm_sec);
}

int main(int argc, char *argv[])
{
    // проверка параметров командной строки
    int options;
    int help = 0;
    std::string open_file;
    std::string save_file;
    bool save = false;
    while ((options = getopt(argc, argv, "hf:")) != -1)
    {
        switch (options)
        {
        case 'h':
            help = 1;
            break;
        case 'f':
            open_file = optarg;
            break;
        default:
            std::cout << "Error: Unknown options." << std::endl;
            return 1;
        }
    }

    if (help)
    {
        std::cout << "Example: utmp [-f filename] [-h]\n"
                  << std::endl;
        return 0;
    }

    if (open_file.empty())
    {
        std::cout << "Error: No filename specified." << std::endl;
        return 1;
    }

    // Открытие файла utmp, wtmp, btmp
    try
    {
        std::string save_file = DateTime() + ".csv";
        std::fstream sf(save_file, std::ios::out);
        sf << "Type_of_login,PID,Terminal,Id,User,Host,Exit_status,Session,Time,IPv4" << std::endl;

        std::fstream of(open_file, std::ios::binary | std::ios::in);
        if (!of.is_open())
        {
            std::cout << "Error opening file!" << std::endl;
            return 1;
        }

        if (!sf.is_open())
        {
            std::cout << "Error opening save file!" << std::endl;
            return 1;
        }

        // Проверка размера файла
        of.seekg(0, std::ios::end);
        size_t fileSize = of.tellg();
        of.seekg(0, std::ios::beg);

        if (fileSize % sizeof(utmp) != 0)
        {
            std::cout << "File error: Invalid file size" << std::endl;
            return 1;
        }

        std::vector<utmp> utmpEntries;
        utmp entry;
        // заполнение массива структур
        while (of.read((char *)&entry, sizeof(entry)))
        {
            if (entry.ut_type > 9 || entry.ut_type < 0)
            {
                std::cout << "File error: Invalid login type." << std::endl;
                return 1;
            }

            utmpEntries.push_back(entry);
        }

        for (auto it = utmpEntries.begin(); it != utmpEntries.end(); it++)
        {
            sf << TypeOfLogin[it->ut_type]
               << "," << it->ut_pid
               << "," << it->ut_line
               << "," << it->ut_id
               << "," << it->ut_user
               << "," << it->ut_host
               << "," << it->ut_exit.e_termination 
               << "," << it->ut_session 
               << "," << it->ut_tv.tv_sec 
               << "," << intToIpV4(it->ut_addr_v6[0]) << std::endl;
        }

        of.close();
        sf.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
