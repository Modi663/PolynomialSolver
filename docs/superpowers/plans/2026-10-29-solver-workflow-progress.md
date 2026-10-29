# SDD ledger — plan: docs/superpowers/plans/2026-10-29-solver-workflow.md

Ruling: исполняю план в этом сеансе, в отдельной ветке текущего checkout — пользователь просит внести правки и коммиты; дополнительный worktree не требуется для одиночной последовательной работы.

Pre-flight: task 4 consumes solveOneNumerically (task 1), BisectionSettingsDialog (task 2), updatePolynomial (task 3); сигнатуры согласованы.

Task 1: complete — тест новых вызовов RED (отсутствует solveOneNumerically), затем GREEN; ctest --preset debug -R PolynomialSolverServiceTests: 1/1. Сканирование вычисляет узлы от начала интервала без накопления ошибки шага.

Task 2: complete — RED отсутствует заголовок диалога, GREEN ctest --preset debug -R BisectionSettingsDialogTests: 1/1. Проверены Apply/Cancel, пустые границы, запятая, скрытый шаг, немедленная валидация и сохранение фокуса.

Task 3: complete — RED отсутствует updatePolynomial, GREEN PolynomialChartViewTests: 1/1. Проверены промежуточная анимация, повторный ввод, сохранение серии и остановка при очистке.

Task 4: complete — RED 3/3 новых сценария не проходили на старом UI, GREEN полный Debug: 11/11. Главная форма остаётся редактируемой; сообщения только при явном Solve, история накапливается, сравнение сохраняет успешный метод при ошибке другого.

Final review: независимый reviewer обнаружил P2: щелчок во время анимации оставлял промежуточные точки. Исправлено синхронизацией серии в stopAnimation; clickDuringAnimationDisplaysLatestPolynomial RED→GREEN. Полные Debug и Release: по 11/11. Согласованное ограничение сканирования и старые аналитические алгоритмы не расширялись.
