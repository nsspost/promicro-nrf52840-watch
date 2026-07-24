# Стартовая задача для Codex

Скопируйте пакет в репозиторий, затем передайте Codex этот текст:

> Реализуй графический слой часов ЦехоСенс по пакету
> `tsehosense-watch-graphics-spec-v1`. Сначала прочитай `AGENTS_GRAPHICS.md` и
> документы в указанном там порядке. До изменений исследуй репозиторий и
> покажи mapping существующих модулей на renderer IR, host renderer, asset
> compiler, embedded backend и IDE preview. Первый исполнимый этап — общий
> renderer-neutral IR и host renderer главного экрана 256×256 для square и
> round с PNG golden comparisons. Не переноси React/CSS в firmware, не меняй
> transport/safety semantics и не начинай embedded backend до совпадения
> host-кадров с эталонами. После реализации выполни `VALIDATION.md` и приложи
> comparison/diff, memory report и список известных отличий.

Если в репозитории уже есть `AGENTS.md`, не заменяйте его автоматически.
Добавьте ссылку на `AGENTS_GRAPHICS.md` в секцию, область действия которой
охватывает renderer/IDE. Если `AGENTS.md` отсутствует, можно использовать
`AGENTS_GRAPHICS.md` как основу.

Для первого PR рекомендуется ограничить scope:

```text
graphics IR
host mono1 renderer
Roboto/Phosphor asset loading
home square 256
home round 256
golden tests
```

Сценарии устройств, событий и команд уже входят в эталоны, но должны
подключаться следующими независимыми изменениями.
