/*
 * ============================================================================
 * Лабораторная работа №1 - Родительский процесс (parent)
 * ============================================================================
 * Назначение:
 *   - Получает имя файла от пользователя
 *   - Создаёт дочерний процесс для обработки чисел из файла
 *   - Получает результаты через pipe и выводит их пользователю
 * ============================================================================
 */

#include <unistd.h>      // Системные вызовы: read, write, pipe, fork, dup2, close, execv
#include <fcntl.h>       // Открытие файлов: open, O_RDONLY
#include <sys/types.h>   // Типы данных: pid_t, ssize_t
#include <sys/wait.h>    // Ожидание дочернего процесса: wait
#include <stdlib.h>      // Завершение процесса: _exit
#include <string.h>      // Работа со строками: strlen, strchr
#include <errno.h>       // Коды ошибок: errno, EINTR

#define BUF_SIZE 256

/*
 * safe_write - надёжная запись данных
 * 
 * Гарантирует запись всех байт, обрабатывая прерывания (EINTR)
 * и частичные записи.
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

int main() {
    char filename[BUF_SIZE] = {0};
    
    /* Шаг 1: Запрос имени файла */
    safe_write(STDOUT_FILENO, "Enter filename: ", 16);
    
    ssize_t len = read(STDIN_FILENO, filename, BUF_SIZE - 1);
    if (len <= 0) {
        safe_write(STDERR_FILENO, "Error reading filename\n", 23);
        return 1;
    }
    
    /* Убираем символ новой строки */
    char *newline = strchr(filename, '\n');
    if (newline) *newline = '\0';

    /* Шаг 2: Создание pipe для связи parent <-> child */
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        safe_write(STDERR_FILENO, "pipe error\n", 11);
        return 1;
    }

    /* Шаг 3: Создание дочернего процесса */
    pid_t pid = fork();
    if (pid < 0) {
        safe_write(STDERR_FILENO, "fork error\n", 11);
        return 1;
    }
    
    if (pid == 0) {
        /* === ДОЧЕРНИЙ ПРОЦЕСС === */
        
        close(pipefd[0]);

        /* Открываем файл с числами */
        int filefd = open(filename, O_RDONLY);
        if (filefd < 0) {
            safe_write(STDERR_FILENO, "Cannot open file\n", 17);
            _exit(1);
        }

        /* Перенаправляем stdin -> файл */
        if (dup2(filefd, STDIN_FILENO) == -1) {
            safe_write(STDERR_FILENO, "dup2 stdin error\n", 17);
            _exit(1);
        }
        close(filefd);

        /* Перенаправляем stdout -> pipe */
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            safe_write(STDERR_FILENO, "dup2 stdout error\n", 18);
            _exit(1);
        }
        close(pipefd[1]);

        /* Запускаем программу child */
        char *args[] = {"./build/child", NULL};
        execv("./build/child", args);
        
        safe_write(STDERR_FILENO, "exec error\n", 11);
        _exit(1);
    }
    else {
        /* === РОДИТЕЛЬСКИЙ ПРОЦЕСС === */
        
        close(pipefd[1]);
        
        safe_write(STDOUT_FILENO, "Result:\n", 8);
        
        /* Читаем результаты от дочернего */
        char buffer[BUF_SIZE];
        ssize_t n;
        while ((n = read(pipefd[0], buffer, BUF_SIZE)) > 0) {
            if (safe_write(STDOUT_FILENO, buffer, n) < 0) {
                safe_write(STDERR_FILENO, "Write error\n", 12);
                break;
            }
        }
        
        close(pipefd[0]);
        wait(NULL);
    }
    
    return 0;
}