#include "status.h"

const char *status_message(status_t status)
{
    switch (status)
    {
        case STATUS_OK:
            return "успех";
        case STATUS_NULL_POINTER:
            return "нулевой указатель";
        case STATUS_NO_MEMORY:
            return "не удалось выделить память";
        case STATUS_INVALID_ARGUMENT:
            return "некорректное значение";
        case STATUS_OVERFLOW:
            return "значение слишком велико";
        case STATUS_INVALID_DATETIME:
            return "некорректная дата или время (нужен формат dd:MM:yyyy hh:mm:ss)";
        case STATUS_INVALID_TIME_ORDER:
            return "время вручения не может быть раньше времени создания";
        case STATUS_DUPLICATE_ID:
            return "отправление с таким идентификатором уже есть";
        case STATUS_NOT_FOUND:
            return "отправление с таким идентификатором не найдено";
        case STATUS_ALREADY_DELIVERED:
            return "отправление уже отмечено как врученное";
        case STATUS_INPUT_ERROR:
            return "ввод завершён";
        case STATUS_LINE_TOO_LONG:
            return "слишком длинная строка";
        case STATUS_SYSTEM_TIME_ERROR:
            return "не удалось получить системное время";
        default:
            return "неизвестная ошибка";
    }
}