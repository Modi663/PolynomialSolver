#include "TestRegistry.h"

#include <QApplication>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    std::vector<std::string> selected;
    std::vector<char*> arguments{argv[0]};
    bool listOnly = false;
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--list-suites") listOnly = true;
        else if (argument == "--suite")
        {
            if (++index == argc)
            {
                std::cerr << "--suite requires a suite name\n";
                return 2;
            }
            selected.emplace_back(argv[index]);
        }
        else arguments.push_back(argv[index]);
    }

    auto& suites = PolynomialSolver::Tests::suites();
    std::sort(suites.begin(), suites.end(),
        [](const auto& left, const auto& right) { return std::string(left.name) < right.name; });
    for (const auto& name : selected)
        if (std::none_of(suites.begin(), suites.end(),
            [&name](const auto& suite) { return name == suite.name; }))
        {
            std::cerr << "Unknown suite: " << name << '\n';
            return 2;
        }
    if (listOnly)
    {
        for (const auto& suite : suites) std::cout << suite.name << '\n';
        return suites.empty() ? 2 : 0;
    }

    const auto selectedCount = std::count_if(suites.begin(), suites.end(), [&](const auto& suite)
    { return selected.empty() || std::find(selected.begin(), selected.end(), suite.name) != selected.end(); });
    for (std::size_t index = 1; index < arguments.size(); ++index)
        if (std::string(arguments[index]) == "-o" && index + 1 < arguments.size())
        {
            const std::string output = arguments[++index];
            const auto separator = output.find(',');
            const bool standardOutput = output == "-" || output.rfind("-,", 0) == 0;
            const bool textFormat = separator == std::string::npos || output.substr(separator + 1) == "txt";
            if (selectedCount > 1 && (!standardOutput || !textFormat))
            {
                std::cerr << "Named or structured Qt Test reports require exactly one suite; use --suite.\n";
                return 2;
            }
        }

    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "offscreen");
    char outputOption[] = "-o";
    char outputFormat[] = "-,txt";
    if (std::none_of(arguments.begin(), arguments.end(),
        [](const char* argument) { return std::string(argument) == "-o"; }))
    {
        arguments.push_back(outputOption);
        arguments.push_back(outputFormat);
    }
    int applicationArgc = static_cast<int>(arguments.size());
    arguments.push_back(nullptr);
    QApplication application(applicationArgc, arguments.data());
    int failures = 0;
    for (const auto& suite : suites)
        if (selected.empty() || std::find(selected.begin(), selected.end(), suite.name) != selected.end())
        {
            const int result = suite.run(applicationArgc, arguments.data());
            failures += result;
            std::cerr << suite.name << ": " << (result == 0 ? "PASS" : "FAIL") << '\n';
        }
    return suites.empty() ? 2 : std::min(failures, 255);
}
