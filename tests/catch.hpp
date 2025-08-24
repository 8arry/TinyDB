// Simplified Catch2-compatible header for TinyDB testing
// This is a minimal implementation that provides the essential macros

#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Catch {

struct TestCase {
    std::string name;
    std::string tags;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void addTest(const std::string& name, const std::string& tags, std::function<void()> func) {
        tests.push_back({name, tags, func});
    }

    int runAllTests() {
        std::cout
            << "==============================================================================="
            << std::endl;
        std::cout << "Running TinyDB Tests with Catch2-compatible framework" << std::endl;
        std::cout
            << "==============================================================================="
            << std::endl;

        int passed = 0;
        int failed = 0;

        for (const auto& test : tests) {
            try {
                std::cout << "PASSED: " << test.name << std::endl;
                test.func();
                passed++;
            } catch (const std::exception& e) {
                std::cout << "FAILED: " << test.name << std::endl;
                std::cout << "        " << e.what() << std::endl;
                failed++;
            } catch (...) {
                std::cout << "FAILED: " << test.name << std::endl;
                std::cout << "        Unknown exception" << std::endl;
                failed++;
            }
        }

        std::cout << std::endl;
        std::cout
            << "==============================================================================="
            << std::endl;
        std::cout << "test cases: " << (passed + failed) << " | " << passed << " passed | "
                  << failed << " failed" << std::endl;
        std::cout
            << "==============================================================================="
            << std::endl;

        return failed > 0 ? 1 : 0;
    }

private:
    std::vector<TestCase> tests;
};

struct TestCaseRegistrar {
    TestCaseRegistrar(const std::string& name, const std::string& tags,
                      std::function<void()> func) {
        TestRegistry::instance().addTest(name, tags, func);
    }
};

class AssertionException : public std::runtime_error {
public:
    AssertionException(const std::string& msg) : std::runtime_error(msg) {
    }
};

template <typename T> std::string toString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

template <typename T, typename U>
void require_equal(const T& expected, const U& actual, const std::string& file, int line) {
    if (!(expected == actual)) {
        std::ostringstream oss;
        oss << "REQUIRE failed at " << file << ":" << line << std::endl;
        oss << "  Expected: " << toString(expected) << std::endl;
        oss << "  Actual:   " << toString(actual);
        throw AssertionException(oss.str());
    }
}

template <typename T>
void require_true(const T& condition, const std::string& expr, const std::string& file, int line) {
    if (!condition) {
        std::ostringstream oss;
        oss << "REQUIRE failed at " << file << ":" << line << std::endl;
        oss << "  Expression: " << expr;
        throw AssertionException(oss.str());
    }
}

template <typename T>
void require_false(const T& condition, const std::string& expr, const std::string& file, int line) {
    if (condition) {
        std::ostringstream oss;
        oss << "REQUIRE_FALSE failed at " << file << ":" << line << std::endl;
        oss << "  Expression: " << expr;
        throw AssertionException(oss.str());
    }
}

template <typename ExceptionType, typename Func>
void require_throws_as(Func&& func, const std::string& expr, const std::string& file, int line) {
    bool threw_expected = false;
    try {
        func();
    } catch (const ExceptionType&) {
        threw_expected = true;
    } catch (...) {
        std::ostringstream oss;
        oss << "REQUIRE_THROWS_AS failed at " << file << ":" << line << std::endl;
        oss << "  Expression: " << expr << std::endl;
        oss << "  Wrong exception type thrown";
        throw AssertionException(oss.str());
    }

    if (!threw_expected) {
        std::ostringstream oss;
        oss << "REQUIRE_THROWS_AS failed at " << file << ":" << line << std::endl;
        oss << "  Expression: " << expr << std::endl;
        oss << "  Expected exception was not thrown";
        throw AssertionException(oss.str());
    }
}

template <typename Func>
void require_nothrow(Func&& func, const std::string& expr, const std::string& file, int line) {
    try {
        func();
    } catch (...) {
        std::ostringstream oss;
        oss << "REQUIRE_NOTHROW failed at " << file << ":" << line << std::endl;
        oss << "  Expression: " << expr << std::endl;
        oss << "  Unexpected exception thrown";
        throw AssertionException(oss.str());
    }
}
} // namespace Catch

// Macros
#define TEST_CASE(name, tags)                                                                      \
    static void CATCH_INTERNAL_FUNCTION_NAME(__LINE__)();                                          \
    static Catch::TestCaseRegistrar CATCH_INTERNAL_REGISTRAR_NAME(__LINE__)(                       \
        name, tags, CATCH_INTERNAL_FUNCTION_NAME(__LINE__));                                       \
    static void CATCH_INTERNAL_FUNCTION_NAME(__LINE__)()

#define CATCH_INTERNAL_FUNCTION_NAME(line) CATCH_INTERNAL_UNIQUE_NAME_LINE(testFunc, line)
#define CATCH_INTERNAL_REGISTRAR_NAME(line) CATCH_INTERNAL_UNIQUE_NAME_LINE(testReg, line)
#define CATCH_INTERNAL_UNIQUE_NAME_LINE(name, line) CATCH_INTERNAL_UNIQUE_NAME(name, line)
#define CATCH_INTERNAL_UNIQUE_NAME(name, line) name##line

#define REQUIRE(expr) Catch::require_true((expr), #expr, __FILE__, __LINE__)

#define REQUIRE_FALSE(expr) Catch::require_false((expr), #expr, __FILE__, __LINE__)

#define REQUIRE_THROWS_AS(expr, exception)                                                         \
    Catch::require_throws_as<exception>([&] { expr; }, #expr, __FILE__, __LINE__)

#define REQUIRE_NOTHROW(expr) Catch::require_nothrow([&] { expr; }, #expr, __FILE__, __LINE__)

#ifdef CATCH_CONFIG_MAIN
int main() {
    return Catch::TestRegistry::instance().runAllTests();
}
#endif
