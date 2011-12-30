#include <cutter.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <outputqueue.h>
#include <utils.h>

FILE *output;
char filename[] = "/tmp/cutter-tmp-file.XXXXXX";
#define BUF_SIZE 1024
char buffer[BUF_SIZE];
OQueue *queue;

void
cut_setup()
{
    int fd = mkstemp(filename);
    close(fd);
    output = fopen(filename, "w+");
    queue = oqueue_new();
}

void
cut_teardown()
{
    fclose(output);
    unlink(filename);
    oqueue_free(queue);
    queue = NULL;
}

char * load_output()
{
    memset(buffer, 0, BUF_SIZE * sizeof(char));
    rewind(output);
    fread(buffer, sizeof(char), BUF_SIZE, output);
    rewind(output);
    truncate(filename, 0);
    return buffer;
}

void
test_push_single_line()
{
    oqueue_push(queue, "line");
    oqueue_flush(queue, output);
    cut_assert_equal_string("line", load_output());
}

void
test_push_more_lines()
{
    oqueue_push(queue, "line\n");
    oqueue_push(queue, "second\n");
    oqueue_push(queue, "third\n");
    oqueue_flush(queue, output);
    cut_assert_equal_string("line\nsecond\nthird\n", load_output());
}

void
test_push_many_lines()
{
    char *parts[] = { "first\n", "second\n", "foo", "\n", "bar", "\n", "baz",
        "\n", "quux", "\n", "very long line\n",
        "this one is even longer than the previous one\n",
        "i still have another line\n", "another one coming\n",
        "oh so many lines\n", "last one?\n", "not really!\n", "\n",
        "did you see that?\n", "there was an empty line\n", NULL };
    int i = 0;
    char mybuf[BUF_SIZE];

    memset(mybuf, 0, BUF_SIZE * sizeof(char));
    for (i = 0; parts[i] != NULL; i++) {
        oqueue_push(queue, parts[i]);
        strcat(mybuf, parts[i]);
    }
    oqueue_flush(queue, output);
    cut_assert_equal_string(mybuf, load_output());
}

#define write_data(fd, str) write(fd, str, strlen(str) * sizeof(char))
void
test_copy_from_fd()
{
    int from[2];
    pipe(from);

    write_data(from[PIPE_WRITE], "foo bar baz\n");
    close(from[PIPE_WRITE]);

    oqueue_copy_from_fd(queue, from[PIPE_READ]);
    close(from[PIPE_READ]);
    oqueue_flush(queue, output);

    cut_assert_equal_string("foo bar baz\n", load_output());
}

void
test_copy_data_more_lines()
{
    int from[2];

    pipe(from);

    write_data(from[PIPE_WRITE], "foo\n");
    write_data(from[PIPE_WRITE], "bar\n");
    write_data(from[PIPE_WRITE], "baz\n");
    close(from[PIPE_WRITE]);

    oqueue_copy_from_fd(queue, from[PIPE_READ]);
    close(from[PIPE_READ]);
    oqueue_flush(queue, output);

    cut_assert_equal_string("foo\nbar\nbaz\n", load_output());
}

void
test_push_formatted_no_args()
{
    oqueue_pushf(queue, "hello\n");
    oqueue_flush(queue, output);
    cut_assert_equal_string("hello\n", load_output());
}

void
test_push_formatted_simple()
{
    oqueue_pushf(queue, "int %d\n", 5);
    oqueue_flush(queue, output);
    cut_assert_equal_string("int 5\n", load_output());

    oqueue_pushf(queue, "string %s\n", "foo");
    oqueue_flush(queue, output);
    cut_assert_equal_string("string foo\n", load_output());
}

void
test_push_formatted_many()
{
    oqueue_pushf(queue, "int %d\nstring %s\nanother string %s\nnext int %d",
            5, "foo", "bar", 0);
    oqueue_flush(queue, output);
    cut_assert_equal_string("int 5\nstring foo\nanother string bar\nnext int 0",
            load_output());
}
