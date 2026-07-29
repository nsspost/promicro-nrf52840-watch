# Инструкция Codex: Universal UI / Renderer

Использовать этот блок в корневом `AGENTS.md` или в локальном
`docs/ui/AGENTS.md` после адаптации путей к реальному репозиторию.

## Перед изменениями

1. Прочитай `DECISIONS.md`, особенно DEC-027, DEC-029, DEC-032, DEC-034,
   DEC-035 и DEC-036.
2. Найди существующие catalog/passport/channel-schema validators и расширяй
   их, не создавай параллельный registry.
3. Найди существующие Theia services/widgets и C common-library conventions.
4. Сначала обнови/добавь contract tests, затем реализацию.

## Архитектурная граница

- Устройство сообщает semantics, values, quality, events и capabilities.
- Gateway/IDE строит normalized UI View Model.
- Renderer выбирает bounded template под Display Profile.
- Skin меняет visual tokens/resources, но не semantics и safety.
- Device-supplied executable code, JavaScript, native callbacks и
  general-purpose bytecode запрещены.

## Safety invariants

- Risk может быть повышен, но не понижен downstream-слоем.
- `critical` на часах и малых renderer-ах — request-only.
- `approved` не означает `executing`.
- Blocking event всегда выше command/request surfaces.
- Desired/draft value не является confirmed value.
- Команды имеют TTL, idempotency key и precondition revision.
- Deferred execution выключено по умолчанию.
- Forcing остаётся только в service mode с TTL и allow-list.

## Реализация

- TypeScript reference model не зависит от Theia.
- Theia widget является adapter-ом над reference model.
- Embedded code — C с bounded tables и fixed pools.
- JSON — authoring/interchange; embedded получает compiled binary.
- Layout не рекурсивный и использует фиксированные template families.
- Все fallbacks детерминированы и покрыты golden tests.

## Проверка

Для каждого изменения прогоняй:

- schema/catalog validation;
- ui-model unit/golden tests;
- Theia build/tests, если затронута IDE;
- host C tests, если затронут parser/layout;
- PUMP-2 guarded/critical scenario;
- square/round/panel preview;
- malformed package tests для binary layer.

Не заявляй готовность только по screenshot. Проверь interaction, accessibility,
safe-area, console/diagnostics и сравнение с утверждённым визуальным языком.

