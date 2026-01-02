# IL2CPP Module System

Модуль-обертка для IL2CPP_Resolver, который упрощает создание модулей и избегает проблем с глобальными переменными.

## Особенности

- ✅ Singleton паттерн для управления инициализацией
- ✅ Автоматическая регистрация модулей
- ✅ Базовый класс для модулей с жизненным циклом (OnEnable/OnDisable/OnUpdate)
- ✅ Упрощенный доступ к Unity объектам
- ✅ Нет проблем с глобальными переменными

## Использование

### Базовый пример модуля

```cpp
#include "modules/modules.h"

class MyModule : public CubixDLC::IL2CPP::ModuleBase
{
public:
    MyModule() 
    {
        // Автоматическая регистрация модуля
        CubixDLC::IL2CPP::ModuleManager::GetInstance().RegisterModule(this);
    }
    
    ~MyModule()
    {
        // Отключение и удаление при уничтожении
        SetEnabled(false);
        CubixDLC::IL2CPP::ModuleManager::GetInstance().UnregisterModule(this);
    }
    
    void OnEnable() override
    {
        // Инициализация при включении модуля
        m_LocalPlayer = CubixDLC::IL2CPP::Utils::FindGameObject("LocalPlayer");
    }
    
    void OnDisable() override
    {
        // Очистка при выключении
        m_LocalPlayer = nullptr;
        m_PlayerData = nullptr;
    }
    
    void OnUpdate() override
    {
        // Вызывается каждый кадр, когда модуль включен
        if (!m_LocalPlayer)
        {
            m_LocalPlayer = CubixDLC::IL2CPP::Utils::FindGameObject("LocalPlayer");
            return;
        }
        
        if (!m_PlayerData)
        {
            m_PlayerData = CubixDLC::IL2CPP::Utils::GetComponent(m_LocalPlayer, "PlayerData");
        }
        
        // Ваш код здесь
    }
    
private:
    Unity::CGameObject* m_LocalPlayer = nullptr;
    Unity::CComponent* m_PlayerData = nullptr;
};

// Создание экземпляра модуля
static MyModule g_MyModule;

// Включение/выключение модуля
void EnableMyModule()
{
    g_MyModule.SetEnabled(true);
}
```

### Работа с Unity объектами

```cpp
// Найти GameObject
Unity::CGameObject* player = CubixDLC::IL2CPP::Utils::FindGameObject("Player");

// Найти все объекты с тегом
Unity::il2cppArray<Unity::CGameObject*>* enemies = 
    CubixDLC::IL2CPP::Utils::FindGameObjectsWithTag("Enemy");

// Получить компонент
Unity::CComponent* health = CubixDLC::IL2CPP::Utils::GetComponent(player, "Health");

// Использование обертки UnityObject
CubixDLC::IL2CPP::UnityObject obj(player);
Unity::CTransform* transform = obj.GetTransform();
```

### Проверка инициализации

```cpp
if (CubixDLC::IL2CPP::Manager::GetInstance().IsInitialized())
{
    // IL2CPP готов к использованию
}
```

## Инициализация

Модуль автоматически инициализируется при загрузке DLL через `hook_render::InitHookThread()`.

Если нужно инициализировать вручную:

```cpp
CubixDLC::IL2CPP::Initialize(true, 60); // waitForModule=true, maxWait=60s
```

## Архитектура

- **Manager** - Singleton для управления инициализацией IL2CPP
- **ModuleBase** - Базовый класс для всех модулей
- **ModuleManager** - Менеджер для регистрации и обновления модулей
- **UnityObject** - Обертка для удобной работы с Unity объектами
- **Utils** - Утилиты для поиска объектов и компонентов

## Преимущества

1. **Нет глобальных переменных** - все через singleton и статические методы
2. **Автоматическая регистрация** - модули регистрируются автоматически
3. **Жизненный цикл** - четкие методы OnEnable/OnDisable/OnUpdate
4. **Простота использования** - минимум кода для создания модуля
5. **Безопасность** - проверки инициализации встроены

