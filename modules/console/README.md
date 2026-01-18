# Console Module

ImGui-based debug console для вывода логов, предупреждений и ошибок.

## Функциональность

- **F1** - открыть/закрыть консоль
- **Clear** - очистить все логи
- **Auto-scroll** - автоматически скроллить вниз при новых сообщениях
- Цветовая разметка: белый (лог), желтый (warning), красный (error)
- Максимум 1000 сообщений в буфере

## API

```cpp
#include "../modules/console/console.h"

// Инициализировать (вызывается автоматически при загрузке DLL)
Console::Initialize();

// Добавить сообщение
Console::Log("Player position: %.2f, %.2f, %.2f", x, y, z);

// Добавить warning
Console::Warning("Health low: %d", health);

// Добавить error
Console::Error("Failed to load asset: %s", name);

// Проверить открыта ли консоль
bool open = Console::IsOpen();

// Переключить консоль
Console::Toggle();

// Очистить логи
Console::Clear();

// Очистить ресурсы (вызывается автоматически при выгрузке DLL)
Console::Cleanup();
```

## Примеры

```cpp
// В любом месте кода можно писать логи
Console::Log("[Game] Level loaded: %s", levelName);
Console::Warning("[Physics] Object out of bounds");
Console::Error("[Render] Failed to compile shader");

// Форматирование как printf
int value = 42;
float fval = 3.14f;
Console::Log("Value: %d, Float: %.2f", value, fval);
```
