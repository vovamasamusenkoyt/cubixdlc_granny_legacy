# IL2CPP_API

Простая обертка вокруг **IL2CPP_Resolver** для удобства работы с Unity IL2CPP runtime.

## Функциональность

### Инициализация
```cpp
IL2CPP_API::Initialize();  // Инициализировать runtime один раз
IL2CPP_API::IsInitialized();  // Проверить инициализацию
```

### Работа с классами
```cpp
// По полному имени
auto* klass = IL2CPP_API::FindClass("UnityEngine.Transform");

// По namespace и имени
auto* klass = IL2CPP_API::FindClass("UnityEngine", "Transform");
```

### Работа с полями
```cpp
// Найти поле
auto* field = IL2CPP_API::FindField(klass, "m_Position");

// Получить значение
Unity::Vector3 pos = IL2CPP_API::GetFieldValue<Unity::Vector3>(instance, field);

// Установить значение
IL2CPP_API::SetFieldValue<bool>(instance, field, true);
```

### Работа с методами
```cpp
// По имени
auto* method = IL2CPP_API::FindMethod(klass, "GetPosition");

// По имени и количеству параметров
auto* method = IL2CPP_API::FindMethod(klass, "SetPosition", 1);

// Вызвать метод
IL2CPP_API::InvokeMethod(instance, method, params);
```

### Работа с объектами
```cpp
// Создать новый экземпляр
void* obj = IL2CPP_API::CreateInstance(klass);

// Получить класс объекта
auto* objClass = IL2CPP_API::GetObjectClass(obj);
```

## Использование в модулях

```cpp
#include "../il2cpp_api/IL2CPP_API.hpp"

namespace MyModule
{
	void Init()
	{
		// Инициализировать IL2CPP
		IL2CPP_API::Initialize();

		// Получить класс
		auto* playerClass = IL2CPP_API::FindClass("Player");
		
		// Найти метод
		auto* updateMethod = IL2CPP_API::FindMethod(playerClass, "Update");
	}
}
```
