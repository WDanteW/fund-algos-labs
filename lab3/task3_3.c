#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTE_VALUES 256
#define MAX_CODE_LENGTH 255
#define READ_CHUNK 4096

typedef enum
{
    STATUS_OK,
    STATUS_NULL_POINTER,
    STATUS_FILE_OPEN_ERROR,
    STATUS_FILE_READ_ERROR,
    STATUS_NO_MEMORY,
    STATUS_OVERFLOW,
    STATUS_HEAP_FULL,
    STATUS_HEAP_EMPTY,
    STATUS_CODE_TOO_LONG,
    STATUS_INTERNAL_ERROR
} status_t;

typedef struct node
{
    uint64_t weight;
    int symbol;
    struct node *left;
    struct node *right;
} node_t;

typedef struct
{
    node_t **items;
    size_t size;
    size_t capacity;
} min_heap_t;

typedef struct
{
    uint64_t original_bytes;
    uint64_t encoded_bits;
    uint64_t encoded_bytes;
    double savings_percent;
} stats_t;

typedef struct
{
    uint64_t frequencies[BYTE_VALUES];
    char codes[BYTE_VALUES][MAX_CODE_LENGTH + 1];
    size_t code_lengths[BYTE_VALUES];
    stats_t stats;
} report_t;

static status_t heap_init(min_heap_t *heap, size_t capacity)
{
    if (heap == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    heap->items = NULL;
    heap->size = 0;
    heap->capacity = 0;
    if (capacity == 0 || capacity > SIZE_MAX / sizeof(node_t *))
    {
        return STATUS_INTERNAL_ERROR;
    }
    heap->items = (node_t **)malloc(capacity * sizeof(node_t *));
    if (heap->items == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    heap->capacity = capacity;
    return STATUS_OK;
}

static void heap_destroy(min_heap_t *heap)
{
    if (heap == NULL)
    {
        return;
    }
    free(heap->items);
    heap->items = NULL;
    heap->size = 0;
    heap->capacity = 0;
}

static void heap_sift_up(const min_heap_t *heap, size_t index)
{
    while (index > 0)
    {
        size_t parent = (index - 1) / 2;
        if (heap->items[parent]->weight <= heap->items[index]->weight)
        {
            break;
        }
        node_t *tmp = heap->items[parent];
        heap->items[parent] = heap->items[index];
        heap->items[index] = tmp;
        index = parent;
    }
}

static void heap_sift_down(const min_heap_t *heap, size_t index)
{
    for (;;)
    {
        size_t left = 2 * index + 1;
        size_t right = left + 1;
        size_t smallest = index;
        if (left < heap->size &&
            heap->items[left]->weight < heap->items[smallest]->weight)
        {
            smallest = left;
        }
        if (right < heap->size &&
            heap->items[right]->weight < heap->items[smallest]->weight)
        {
            smallest = right;
        }
        if (smallest == index)
        {
            break;
        }
        node_t *tmp = heap->items[smallest];
        heap->items[smallest] = heap->items[index];
        heap->items[index] = tmp;
        index = smallest;
    }
}

static status_t heap_push(min_heap_t *heap, node_t *node)
{
    if (heap == NULL || node == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (heap->size >= heap->capacity)
    {
        return STATUS_HEAP_FULL;
    }
    heap->items[heap->size] = node;
    heap_sift_up(heap, heap->size);
    heap->size++;
    return STATUS_OK;
}

static status_t heap_pop(min_heap_t *heap, node_t **node)
{
    if (heap == NULL || node == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (heap->size == 0)
    {
        return STATUS_HEAP_EMPTY;
    }
    *node = heap->items[0];
    heap->size--;
    if (heap->size > 0)
    {
        heap->items[0] = heap->items[heap->size];
        heap_sift_down(heap, 0);
    }
    return STATUS_OK;
}

static status_t count_frequencies(const char *path,
                                  uint64_t frequencies[BYTE_VALUES])
{
    if (path == NULL || frequencies == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        return STATUS_FILE_OPEN_ERROR;
    }

    unsigned char buffer[READ_CHUNK];
    size_t got = 0;
    status_t status = STATUS_OK;
    while ((got = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        for (size_t i = 0; i < got; i++)
        {
            frequencies[buffer[i]]++;
        }
    }
    if (ferror(file))
    {
        status = STATUS_FILE_READ_ERROR;
    }
    fclose(file);
    return status;
}

static status_t build_tree(const uint64_t frequencies[BYTE_VALUES],
                           node_t **pool, node_t **root)
{
    if (frequencies == NULL || pool == NULL || root == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    *pool = NULL;
    *root = NULL;

    size_t distinct = 0;
    for (size_t i = 0; i < BYTE_VALUES; i++)
    {
        if (frequencies[i] > 0)
        {
            distinct++;
        }
    }
    if (distinct == 0)
    {
        return STATUS_OK;
    }

    node_t *nodes = (node_t *)calloc(2 * distinct - 1, sizeof(node_t));
    if (nodes == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    min_heap_t heap;
    status_t status = heap_init(&heap, distinct);
    if (status != STATUS_OK)
    {
        free(nodes);
        return status;
    }

    size_t used = 0;
    for (size_t i = 0; i < BYTE_VALUES && status == STATUS_OK; i++)
    {
        if (frequencies[i] == 0)
        {
            continue;
        }
        nodes[used].weight = frequencies[i];
        nodes[used].symbol = (int)i;
        status = heap_push(&heap, &nodes[used]);
        used++;
    }

    while (status == STATUS_OK && heap.size > 1)
    {
        node_t *first = NULL;
        node_t *second = NULL;
        status = heap_pop(&heap, &first);
        if (status == STATUS_OK)
        {
            status = heap_pop(&heap, &second);
        }
        if (status == STATUS_OK && first->weight > UINT64_MAX - second->weight)
        {
            status = STATUS_OVERFLOW;
        }
        if (status == STATUS_OK)
        {
            node_t *parent = &nodes[used];
            used++;
            parent->weight = first->weight + second->weight;
            parent->symbol = -1;
            parent->left = first;
            parent->right = second;
            status = heap_push(&heap, parent);
        }
    }

    node_t *top = NULL;
    if (status == STATUS_OK)
    {
        status = heap_pop(&heap, &top);
    }
    heap_destroy(&heap);

    if (status != STATUS_OK)
    {
        free(nodes);
        return status;
    }
    *pool = nodes;
    *root = top;
    return STATUS_OK;
}

static status_t assign_codes(const node_t *node, char *path, size_t depth,
                             report_t *report)
{
    if (node == NULL || path == NULL || report == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (node->left == NULL && node->right == NULL)
    {
        size_t index = (size_t)node->symbol;
        memcpy(report->codes[index], path, depth);
        report->codes[index][depth] = '\0';
        report->code_lengths[index] = depth;
        return STATUS_OK;
    }
    if (node->left == NULL || node->right == NULL)
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (depth >= MAX_CODE_LENGTH)
    {
        return STATUS_CODE_TOO_LONG;
    }
    path[depth] = '0';
    status_t status = assign_codes(node->left, path, depth + 1, report);
    if (status != STATUS_OK)
    {
        return status;
    }
    path[depth] = '1';
    return assign_codes(node->right, path, depth + 1, report);
}

static status_t build_codes(const node_t *root, report_t *report)
{
    if (report == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    if (root == NULL)
    {
        return STATUS_OK;
    }
    if (root->left == NULL && root->right == NULL)
    {
        size_t index = (size_t)root->symbol;
        report->codes[index][0] = '0';
        report->codes[index][1] = '\0';
        report->code_lengths[index] = 1;
        return STATUS_OK;
    }
    char path[MAX_CODE_LENGTH + 1];
    return assign_codes(root, path, 0, report);
}

static status_t compute_stats(report_t *report)
{
    if (report == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    uint64_t original = 0;
    uint64_t bits = 0;
    for (size_t i = 0; i < BYTE_VALUES; i++)
    {
        uint64_t freq = report->frequencies[i];
        if (freq == 0)
        {
            continue;
        }
        uint64_t length = (uint64_t)report->code_lengths[i];
        if (length == 0)
        {
            return STATUS_INTERNAL_ERROR;
        }
        if (freq > UINT64_MAX / length)
        {
            return STATUS_OVERFLOW;
        }
        uint64_t symbol_bits = freq * length;
        if (bits > UINT64_MAX - symbol_bits || original > UINT64_MAX - freq)
        {
            return STATUS_OVERFLOW;
        }
        bits += symbol_bits;
        original += freq;
    }

    report->stats.original_bytes = original;
    report->stats.encoded_bits = bits;
    report->stats.encoded_bytes = bits / 8 + (bits % 8 != 0 ? 1 : 0);
    if (original == 0)
    {
        report->stats.savings_percent = 0.0;
    }
    else
    {
        report->stats.savings_percent =
            100.0 * (1.0 - (double)report->stats.encoded_bytes /
                               (double)original);
    }
    return STATUS_OK;
}

static status_t analyze_file(const char *path, report_t *report)
{
    if (path == NULL || report == NULL)
    {
        return STATUS_NULL_POINTER;
    }
    memset(report, 0, sizeof(*report));

    status_t status = count_frequencies(path, report->frequencies);
    node_t *pool = NULL;
    node_t *root = NULL;
    if (status == STATUS_OK)
    {
        status = build_tree(report->frequencies, &pool, &root);
    }
    if (status == STATUS_OK)
    {
        status = build_codes(root, report);
    }
    free(pool);
    if (status == STATUS_OK)
    {
        status = compute_stats(report);
    }
    return status;
}

static const char *status_message(status_t status)
{
    switch (status)
    {
    case STATUS_OK:
        return "успех";
    case STATUS_NULL_POINTER:
        return "нулевой указатель";
    case STATUS_FILE_OPEN_ERROR:
        return "не удалось открыть файл";
    case STATUS_FILE_READ_ERROR:
        return "ошибка чтения файла";
    case STATUS_NO_MEMORY:
        return "не удалось выделить память";
    case STATUS_OVERFLOW:
        return "переполнение при подсчёте размеров";
    case STATUS_HEAP_FULL:
        return "переполнение кучи";
    case STATUS_HEAP_EMPTY:
        return "куча пуста";
    case STATUS_CODE_TOO_LONG:
        return "код символа слишком длинный";
    case STATUS_INTERNAL_ERROR:
        return "внутренняя ошибка";
    default:
        return "неизвестная ошибка";
    }
}

static void print_report(const report_t *report)
{
    printf("Байт Частота Код\n");
    for (size_t i = 0; i < BYTE_VALUES; i++)
    {
        if (report->frequencies[i] > 0)
        {
            printf("%zu %" PRIu64 " %s\n", i, report->frequencies[i],
                   report->codes[i]);
        }
    }
    printf("Размер исходного файла: %" PRIu64 " байт\n",
           report->stats.original_bytes);
    printf("Размер закодированных данных: %" PRIu64 " бит (%" PRIu64
           " байт)\n",
           report->stats.encoded_bits, report->stats.encoded_bytes);
    printf("Экономия: %.2f%%\n", report->stats.savings_percent);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Использование: %s <путь к файлу>\n",
                argc > 0 ? argv[0] : "huffman");
        return 1;
    }

    report_t *report = (report_t *)malloc(sizeof(report_t));
    if (report == NULL)
    {
        fprintf(stderr, "Ошибка: %s\n", status_message(STATUS_NO_MEMORY));
        return 1;
    }

    status_t status = analyze_file(argv[1], report);
    if (status != STATUS_OK)
    {
        fprintf(stderr, "Ошибка: %s\n", status_message(status));
        free(report);
        return 1;
    }

    print_report(report);
    free(report);
    return 0;
}