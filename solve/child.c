/*
 * ============================================================================
 * Лабораторная работа №1 - Дочерний процесс (child)
 * ============================================================================
 * Назначение:
 *   - Читает строки с числами из stdin (перенаправленного на файл)
 *   - Суммирует числа в каждой строке
 *   - Выводит результат в формате "Sum: XX.XX"
 * ============================================================================
 */

#include <unistd.h>      // Системные вызовы: read, write
#include <stdlib.h>      // Преобразование строк: strtof, _exit
#include <string.h>      // Работа со строками: strlen
#include <errno.h>       // Обработка ошибок: errno, EINTR, ERANGE

#define BUF_SIZE 256

/*
 * safe_write - надёжная запись данных
 * 
 * @fd: файловый дескриптор
 * @buf: данные для записи
 * @count: количество байт
 * @return: count при успехе, -1 при ошибке
 */
static ssize_t safe_write(int fd, const void *buf, size_t count) {
    const char *p = buf;
    size_t left = count;
    
    while (left > 0) {
        ssize_t written = write(fd, p, left);
        if (written < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += written;
        left -= written;
    }
    return count;
}

/*
 * write_float - вывод числа типа float в формате "Sum: XX.XX\n"
 * 
 * Формирует строку вручную без использования printf/sprintf
 * 
 * @num: число для вывода
 */
void write_float(float num) {
    char buf[64];
    int pos = 0;

    /* Префикс "Sum: " */
    buf[pos++] = 'S'; buf[pos++] = 'u'; buf[pos++] = 'm';
    buf[pos++] = ':'; buf[pos++] = ' ';

    /* Знак */
    if (num < 0) {
        buf[pos++] = '-';
        num = -num;
    }

    /* Целая часть */
    int whole = (int)num;
    if (whole == 0) {
        buf[pos++] = '0';
    } else {
        char tmp[20];
        int tp = 0;
        while (whole > 0) {
            tmp[tp++] = '0' + (whole % 10);
            whole /= 10;
        }
        while (tp > 0) {
            buf[pos++] = tmp[--tp];
        }
    }

    buf[pos++] = '.';

    /* Дробная часть (2 знака) */
    int frac = (int)((num - (int)num) * 100 + 0.5);
    if (frac >= 100) frac = 99;
    buf[pos++] = '0' + (frac / 10);
    buf[pos++] = '0' + (frac % 10);
    buf[pos++] = '\n';

    safe_write(STDOUT_FILENO, buf, pos);
}

/*
 * process_line - обработка одной строки с числами
 * 
 * Парсит строку, извлекает числа, суммирует их и выводит результат
 * 
 * @line: строка для обработки
 */
void process_line(char *line) {
    float sum = 0.0;
    char *p = line;
    
    while (*p) {
        /* Пропускаем пробелы */
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;
        
        /* Парсим число */
        char *end;
        errno = 0;
        float val = strtof(p, &end);
        
        /* Проверка ошибок */
        if (end == p) {
            safe_write(STDERR_FILENO, "Parse error\n", 12);
            _exit(1);
        }
        if (errno == ERANGE) {
            safe_write(STDERR_FILENO, "Number too large\n", 17);
            _exit(1);
        }
        
        sum += val;
        p = end;
    }
    
    write_float(sum);
}

int main() {
    char line[BUF_SIZE];
    int line_pos = 0;
    char buf[BUF_SIZE];
    ssize_t n;

    /* Читаем данные из stdin порциями */
    while ((n = read(STDIN_FILENO, buf, BUF_SIZE)) > 0) {
        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                /* Конец строки - обрабатываем */
                if (line_pos > 0) {
                    line[line_pos] = '\0';
                    process_line(line);
                    line_pos = 0;
                }
            } else {
                /* Накапливаем символы */
                if (line_pos < BUF_SIZE - 1) {
                    line[line_pos++] = buf[i];
                }
            }
        }
    }

    /* Последняя строка без \n */
    if (line_pos > 0) {
        line[line_pos] = '\0';
        process_line(line);
    }

    return 0;
}