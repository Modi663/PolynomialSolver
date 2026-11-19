#pragma once

#include <QtTest/QTest>
#include <vector>

namespace PolynomialSolver::Tests
{
    struct TestSuite
    {
        const char* name;
        int (*run)(int, char**);
    };

    inline std::vector<TestSuite>& suites()
    {
        static std::vector<TestSuite> registered;
        return registered;
    }

    inline bool registerSuite(TestSuite suite)
    {
        suites().push_back(suite);
        return true;
    }
}

#define REGISTER_TEST_SUITE(Type) \
    namespace { \
        const bool registered##Type = PolynomialSolver::Tests::registerSuite( \
            {#Type, [](int argc, char** argv) { Type suite; return QTest::qExec(&suite, argc, argv); }}); \
    }
