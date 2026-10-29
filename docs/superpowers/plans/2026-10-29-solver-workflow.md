# Solver Workflow Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Упростить управление бисекцией, плавно обновлять график, проверять ввод сразу и сохранять историю со сравнением методов.

**Architecture:** Существующее ядро получает отдельный путь бисекции одного корня. Новый диалог отвечает за применение численных параметров. MainWindow связывает независимый предварительный график, явное решение и журнал; PolynomialChartView переиспользует серии с анимированным переходом.

**Tech Stack:** C++20, Qt 6 Widgets/Charts/Test, CMake/Ninja, MinGW.

**Spec:** docs/superpowers/specs/2026-10-29-solver-workflow-design.md

## Global Constraints

- Степени 1–5; аналитическое решение и сравнение доступны только для 1–3.
- Границы пустые и обязательные; точность 0–15 знаков, по умолчанию 6.
- Один корень: бисекция целого интервала без шага и сканирования.
- Все корни: положительный шаг, максимум 100 000 участков, уточнение бисекцией.
- При вводе только inline-ошибки, при «Решить» с ошибкой — QMessageBox.
- График обновляется автоматически с анимацией около 200 мс; поля после решения доступны.
- Коммиты: Conventional Commits, русский текст, без scope: `feat: описание`.

## Review Focus

- Повторное изменение коэффициентов посреди анимации должно завершиться графиком последних данных; тест task 3.
- Отмена черновика и скрытый некорректный шаг не должны менять применённые настройки; тест task 2.
- Слишком мелкий шаг или огромный интервал должны завершаться ошибкой без зависания; тест task 1.
- Ошибка одного метода не должна скрывать результат другого; тест task 4.
- Быстрое исправление ошибки не должно уводить фокус или оставлять старые маркеры; тест task 4.

### Task 1: Бисекция одного корня и ограничение сканирования

**Files:** BisectionSolver.{h,cpp}, PolynomialSolverService.{h,cpp}, tests/QtTests/PolynomialSolverServiceTests.cpp.
**Interfaces:** Produces `BisectionSolver::solveOne(const Polynomial&, double left, double right, int decimalPlaces = 6) -> SolveResult`, `PolynomialSolverService::solveOneNumerically(std::vector<double>, double left, double right, int decimalPlaces = 6) const -> SolvedPolynomial`; сохраняет существующий solveNumerically для поиска всех.

- [x] Добавить тесты одного корня, обеих границ, отсутствия сканирования при равных знаках, ошибок интервала/точности и ограничения сканирования.
- [x] Собрать PolynomialSolverServiceTests: новые вызовы должны не компилироваться до реализации.
- [x] Реализовать solveOne через bisect и общую валидацию; ограничить сканирование до начала и в цикле, сохранять проверку продвижения double.
- [x] Собрать и выполнить `ctest --preset debug -R PolynomialSolverServiceTests`: PASS.
- [x] Коммит `feat: разделить поиск одного и всех корней бисекцией`.

### Task 2: Диалог настроек

**Files:** создать BisectionSettingsDialog.{h,cpp}, tests/QtTests/BisectionSettingsDialogTests.cpp; изменить CMakeLists обоих каталогов.
**Interfaces:** Produces `BisectionSettings { double left, right, step; int decimalPlaces; bool findAll; }`, `BisectionSettingsDialog::settings() const -> std::optional<BisectionSettings>`, `beginEditing()`, `validateInputs() -> bool`; accepted означает применённые валидные настройки.

- [x] Тесты: пустые границы отклоняются, step скрыт/проверяется только при findAll, запятая принимается, отмена восстанавливает применённые значения, ошибки исправляются немедленно.
- [x] Настроить и собрать новый тест: ожидается отсутствие заголовка.
- [x] Реализовать диалог с QFormLayout, QDialogButtonBox, Apply/Cancel, валидацией всех активных полей, красным подчёркиванием и текстом; применение закрывает окно.
- [x] Выполнить новый тест: PASS.
- [x] Коммит `feat: вынести настройки бисекции в отдельный диалог`.

### Task 3: Анимированное обновление графика

**Files:** PolynomialChartView.{h,cpp}, tests/QtTests/PolynomialChartViewTests.cpp.
**Interfaces:** сохраняется `setPolynomial(std::vector<double>, const std::vector<std::complex<double>>&)`; добавляется `updatePolynomial(std::vector<double>)` для предпросмотра без маркеров. QVariantAnimation управляет переходом viewport и точек, серии переиспользуются.

- [x] Добавить тесты сохранения идентичности серий при обновлении, промежуточных данных/конечного диапазона, повторного изменения в процессе перехода, очистки и отмены перехода.
- [x] Собрать тест: отсутствие updatePolynomial должно вызвать ошибку.
- [x] Реализовать переход 200 мс от текущих точек к целевым на существующих сериях; прерывать при clear, zoom, pan, reset. Первое построение и явное setPolynomial остаются немедленными.
- [x] Выполнить PolynomialChartViewTests: PASS.
- [x] Коммит `feat: добавить плавное обновление графика при вводе`.

### Task 4: Главное окно и журнал

**Files:** MainWindow.{h,cpp,ui}, tests/QtTests/MainWindowTests.cpp.
**Interfaces:** consumes настройки task 2, solveOneNumerically task 1, updatePolynomial task 3. Produces редактируемая форма, диалог из кнопки, 3 метода, немедленная валидация, история с ExtraSelection для последней записи.

- [x] Обновить UI-тесты под новую спецификацию: график до решения, красные поля сразу без QMessageBox, сообщение только при Solve, обязательные настройки, история из двух решений, один/все, оба метода и частичный сбой, степени 4–5, очистка истории, нет потери фокуса.
- [x] Собрать и запустить MainWindowTests: FAIL на прежней форме/поведении.
- [x] Удалить встроенные настройки из .ui, добавить settingsButton, третий метод, всегда доступное решение и очистку истории. Сохранить структуру Designer и расположение результата/графика.
- [x] Реализовать валидацию всех коэффициентов, живой предпросмотр, явное решение и сравнительную запись с параметрами/невязками; текстовый моноширинный журнал сохраняет прошлые записи, последняя выделяется целиком.
- [x] Полный Debug `cmake --build --preset debug`, `ctest --preset debug`: PASS.
- [x] Коммит `feat: добавить сравнение методов и историю расчётов`.

### Task 5: Документация и финальная проверка

**Files:** README.md, spec и этот план.

- [x] Обновить управление, состояния и ограничения README под итоговую реализацию.
- [x] Проверить полноту требований; свежий независимый reviewer читает весь diff, spec и тесты. Существенные находки исправить с тестами.
- [x] Debug и Release: сборка и все ctest, затем визуальная проверка окна и диалога.
- [x] Коммит `docs: описать новый интерфейс и режимы бисекции`; оставить изменения в отдельной ветке текущего checkout.
