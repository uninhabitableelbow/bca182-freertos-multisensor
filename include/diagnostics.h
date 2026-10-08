#pragma once

namespace app_runtime {
[[noreturn]] void fail_stop();
void print_diagnostic(const char *message, const char *detail = nullptr);
void task_a(void *);
void task_b(void *);
}
