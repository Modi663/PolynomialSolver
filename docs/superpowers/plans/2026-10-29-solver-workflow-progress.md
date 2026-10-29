# SDD ledger — plan: docs/superpowers/plans/2026-10-29-solver-workflow.md

Ruling: исполняю план в этом сеансе, в отдельной ветке текущего checkout — пользователь просит внести правки и коммиты; дополнительный worktree не требуется для одиночной последовательной работы.

Pre-flight: task 4 consumes solveOneNumerically (task 1), BisectionSettingsDialog (task 2), updatePolynomial (task 3); сигнатуры согласованы.

Task 1: complete — тест новых вызовов RED (отсутствует solveOneNumerically), затем GREEN; ctest --preset debug -R PolynomialSolverServiceTests: 1/1. Сканирование вычисляет узлы от начала интервала без накопления ошибки шага.

Task 2: complete — RED отсутствует заголовок диалога, GREEN ctest --preset debug -R BisectionSettingsDialogTests: 1/1. Проверены Apply/Cancel, пустые границы, запятая, скрытый шаг, немедленная валидация и сохранение фокуса.
