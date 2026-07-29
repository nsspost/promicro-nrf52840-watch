# Проверка пакета v0.1

## Выполнено

- все JSON-файлы синтаксически корректны (`jq`);
- все пять JSON Schema компилируются как Draft 2020-12;
- три Display Profile проходят `display-profile.schema.json`;
- сценарий PUMP-2 проходит `preview-scenario.schema.json`;
- normalized PUMP-2 View Model проходит `ui-view-model.schema.json`;
- safety invariants согласованы с command/event flow прототипа часов;
- роли и channel schemas согласованы с DEC-034/DEC-035;
- migration requirement согласовано с DEC-036;
- forcing не смешан с пользовательскими capabilities и сохраняет DEC-029;
- reference UI не требует React/Theia в embedded runtime.

## Проверенные сценарии

- PUMP-2 overview;
- primary metrics: ток и давление;
- guarded `AUTO → MANUAL`;
- critical `pump.stop` как request-only;
- blocking event precedence;
- square/round 256×256;
- gateway panel 320×240;
- low-memory и missing-skin fallback описаны нормативно.

## Намеренно оставлено до реализации

- окончательная бинарная раскладка `ts_ui_package_v1`;
- точные C struct sizes и целевой RAM budget после измерения target;
- cryptographic signature policy для skins/packages;
- конкретный BLE/LoRa frame mapping;
- authority protocol для реального critical execution;
- advanced profile-specific manual layout overrides;
- pixel-perfect skin editor.

Эти пункты не должны решаться молча внутри первой реализации. Для каждого
нужно измерение/spike и отдельное архитектурное решение.

