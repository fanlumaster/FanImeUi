#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace test
{
struct TestCase
{
    std::string name;
    std::function<void()> fn;
};

std::vector<TestCase> &registry();

struct Registrar
{
    Registrar(const char *name, std::function<void()> fn);
};

// Some cases need dictionary data that only exists once the IME is installed: msime.db, others.db,
// the helpcode tables. Failing them on a machine without that data says nothing about the code, so
// they report themselves as skipped instead. The runner counts skips separately and keeps going.
struct SkippedError : std::runtime_error
{
    explicit SkippedError(const std::string &reason) : std::runtime_error(reason) {}
};

[[noreturn]] void skip(const std::string &reason);

// Skip unless every path exists. Pass the paths the case reads.
void require_data_files(const std::vector<std::string> &paths);
}

#define TEST_CASE(name)                                                                                                 \
    static void name();                                                                                                 \
    static test::Registrar name##_registrar(#name, name);                                                              \
    static void name()

#define REQUIRE(expr)                                                                                                   \
    do                                                                                                                  \
    {                                                                                                                   \
        if (!(expr))                                                                                                    \
        {                                                                                                               \
            throw std::runtime_error("Requirement failed: " #expr);                                                    \
        }                                                                                                               \
    } while (false)

#define REQUIRE_EQ(lhs, rhs)                                                                                            \
    do                                                                                                                  \
    {                                                                                                                   \
        const auto &_lhs = (lhs);                                                                                       \
        const auto &_rhs = (rhs);                                                                                       \
        if (!(_lhs == _rhs))                                                                                            \
        {                                                                                                               \
            throw std::runtime_error("Requirement failed: " #lhs " == " #rhs);                                        \
        }                                                                                                               \
    } while (false)
