# Cartesian Scale Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Строгий 1:1 и сохранение ручного вида при вводе.
**Architecture:** PlotDataBuilder строит конечный равномасштабный viewport по центру, масштабу и размеру области; PolynomialChartView хранит эти параметры и синхронизирует оси с plotArea Qt Charts.
**Tech Stack:** C++20, Qt Charts/Widgets/Test, CMake.
**Spec:** docs/superpowers/specs/2026-10-30-cartesian-scale-design.md

## Global Constraints

- main, коммиты на русском без scope, без пуша.
- Только 1:1 с ручным масштабированием; без обзорного режима.
- Сохранять живую кривую, безопасные деления и штатный цикл перерисовки.

## Review Focus

- Изменения plotArea из-за подписей осей не должны вызывать рекурсию или нарушать 1:1.
- Нечисловые/переполненные pan/zoom не меняют корректный вид.
- Временное пустое поле не сбрасывает ручной масштаб.

### Task 1: Геометрия масштаба

- [x] RED: тесты экранных единиц, resize и сохранения вида.
- [x] Заменить fit на equalScaleViewport(center, unitsPerPixel, plotSize) -> optional<PlotViewport>; покрыть пустые и нечисловые размеры и переполнение.

### Task 2: Управление видом

- [x] Состояние center/unitsPerPixel, синхронизация с plotArea, безопасный pan/zoom/reset; удалить анимацию осей и автоматическую подгонку.
- [x] GREEN: геометрические и UI-тесты, включая zoom/pan и изменение функции.

### Task 3: Завершение

- [x] README, независимая проверка, Debug/Release и 200%, обновить готовые копии.
- [x] Коммит `feat: закрепить масштаб графика 1:1 и ручное управление видом`.

## Verification

- Debug: 14/14 CTest-наборов; Release: 14/14; Debug при QT_SCALE_FACTOR=2: 14/14.
- Нативный Windows: PolynomialChartViewTests — 22 passed, 0 failed (включая init/cleanup).
- Проверены 80 последовательных изменений масштаба, предел точности, экстремальный resize, пересечения границ для очень крутых кривых, ручное перемещение к корню за пределами окна и восстановление после неверного ввода.
- Независимая проверка нашла и помогла устранить колебания автоматических полей Qt, переполнение геометрии кривой и устаревший заголовок. Повторная числовая и визуальная проверка при 100%/200% важных дефектов не выявила.
- Обе существующие готовые копии обновлены; SHA-256 совпадает с финальной Release-сборкой.
