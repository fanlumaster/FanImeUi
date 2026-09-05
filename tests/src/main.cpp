#include <exception>
#include <filesystem>
#include <fmt/core.h>
#include "tests/includes/test_framework.h"

namespace test
{
std::vector<TestCase> &registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

Registrar::Registrar(const char *name, std::function<void()> fn)
{
    registry().push_back({name, std::move(fn)});
}

[[noreturn]] void skip(const std::string &reason)
{
    throw SkippedError(reason);
}

void require_data_files(const std::vector<std::string> &paths)
{
    for (const auto &path : paths)
    {
        if (!std::filesystem::exists(path))
        {
            skip(fmt::format("needs dictionary data that is not installed: {}", path));
        }
    }
}
} // namespace test

int main()
{
    int failures = 0;
    int skipped = 0;
    for (const auto &test_case : test::registry())
    {
        try
        {
            test_case.fn();
            fmt::print("[PASS] {}\n", test_case.name);
        }
        catch (const test::SkippedError &ex)
        {
            ++skipped;
            fmt::print("[SKIP] {}: {}\n", test_case.name, ex.what());
        }
        catch (const std::exception &ex)
        {
            ++failures;
            fmt::print(stderr, "[FAIL] {}: {}\n", test_case.name, ex.what());
        }
        catch (...)
        {
            ++failures;
            fmt::print(stderr, "[FAIL] {}: unknown exception\n", test_case.name);
        }
    }

    if (failures > 0)
    {
        fmt::print(stderr, "{} test(s) failed, {} skipped\n", failures, skipped);
        return 1;
    }

    fmt::print("{} test(s) passed, {} skipped\n", test::registry().size() - skipped, skipped);
    return 0;
}
