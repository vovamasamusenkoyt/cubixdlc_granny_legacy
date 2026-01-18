#pragma once

#include <vector>
#include <string>
#include <ctime>

namespace Console
{
	struct LogMessage
	{
		std::string text;
		float timestamp;
		int type;  // 0 = normal, 1 = warning, 2 = error
	};

	/// Инициализирует консоль
	void Initialize();

	/// Проверяет, открыта ли консоль
	bool IsOpen();

	/// Переключает консоль
	void Toggle();

	/// Очищает логи
	void Clear();

	/// Добавляет сообщение в лог
	void Log(const char* format, ...);

	/// Добавляет warning в лог
	void Warning(const char* format, ...);

	/// Добавляет error в лог
	void Error(const char* format, ...);

	/// Рендерит консоль (вызывается из hud)
	void Render();

	/// Обновляет консоль (вызывается каждый кадр)
	void Update();

	/// Очищает ресурсы консоли
	void Cleanup();
}
