#include "console.h"
#include "../../deps/imgui/imgui.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>

namespace Console
{
	// ============================================================================
	// ВНУТРЕННИЕ ПЕРЕМЕННЫЕ
	// ============================================================================

	static std::vector<LogMessage> g_Logs;
	static bool g_IsOpen = false;
	static float g_LastTime = 0.0f;
	static bool g_AutoScroll = true;
	static char g_InputBuffer[256] = {};

	// ============================================================================
	// РЕАЛИЗАЦИЯ
	// ============================================================================

	void Initialize()
	{
		g_Logs.clear();
		g_IsOpen = false;
		g_AutoScroll = true;
		Log("[Console] Initialized");
	}

	bool IsOpen()
	{
		return g_IsOpen;
	}

	void Toggle()
	{
		g_IsOpen = !g_IsOpen;
	}

	void Clear()
	{
		g_Logs.clear();
	}

	void Log(const char* format, ...)
	{
		if (!format) return;

		char buffer[512] = {};
		va_list args;
		va_start(args, format);
		vsprintf_s(buffer, sizeof(buffer), format, args);
		va_end(args);

		LogMessage msg;
		msg.text = buffer;
		msg.timestamp = static_cast<float>(ImGui::GetTime());
		msg.type = 0;  // Normal

		g_Logs.push_back(msg);

		// Выводим в отладчик
		printf("[LOG] %s\n", buffer);

		// Ограничиваем размер буфера до 1000 сообщений
		if (g_Logs.size() > 1000)
		{
			g_Logs.erase(g_Logs.begin());
		}
	}

	void Warning(const char* format, ...)
	{
		if (!format) return;

		char buffer[512] = {};
		va_list args;
		va_start(args, format);
		vsprintf_s(buffer, sizeof(buffer), format, args);
		va_end(args);

		LogMessage msg;
		msg.text = std::string("[WARNING] ") + buffer;
		msg.timestamp = static_cast<float>(ImGui::GetTime());
		msg.type = 1;  // Warning

		g_Logs.push_back(msg);

		printf("[WARNING] %s\n", buffer);

		if (g_Logs.size() > 1000)
		{
			g_Logs.erase(g_Logs.begin());
		}
	}

	void Error(const char* format, ...)
	{
		if (!format) return;

		char buffer[512] = {};
		va_list args;
		va_start(args, format);
		vsprintf_s(buffer, sizeof(buffer), format, args);
		va_end(args);

		LogMessage msg;
		msg.text = std::string("[ERROR] ") + buffer;
		msg.timestamp = static_cast<float>(ImGui::GetTime());
		msg.type = 2;  // Error

		g_Logs.push_back(msg);

		printf("[ERROR] %s\n", buffer);

		if (g_Logs.size() > 1000)
		{
			g_Logs.erase(g_Logs.begin());
		}
	}

	void Update()
	{
		// Проверяем нажатие F1
		if ((GetAsyncKeyState(VK_F1) & 0x8000) != 0)
		{
			float currentTime = static_cast<float>(ImGui::GetTime());
			if (currentTime - g_LastTime > 0.5f)  // Debounce 500ms
			{
				Toggle();
				g_LastTime = currentTime;
			}
		}
	}

	void Render()
	{
		if (!g_IsOpen) return;

		ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Console##debug", &g_IsOpen, ImGuiWindowFlags_NoMove))
		{
			// Панель с кнопками
			if (ImGui::Button("Clear"))
			{
				Clear();
			}

			ImGui::SameLine();
			ImGui::Checkbox("Auto-scroll", &g_AutoScroll);

			ImGui::Separator();

			// Область с логами
			ImGui::BeginChild("LogArea", ImVec2(0, -30), true, ImGuiWindowFlags_HorizontalScrollbar);

			for (const auto& log : g_Logs)
			{
				// Цвет в зависимости от типа сообщения
				switch (log.type)
				{
				case 0:  // Normal
					ImGui::TextColored(ImVec4(1, 1, 1, 1), "%s", log.text.c_str());
					break;
				case 1:  // Warning
					ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", log.text.c_str());
					break;
				case 2:  // Error
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", log.text.c_str());
					break;
				}
			}

			if (g_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			{
				ImGui::SetScrollHereY(1.0f);
			}

			ImGui::EndChild();

			// Поле ввода (для будущего использования)
			ImGui::InputText("##input", g_InputBuffer, sizeof(g_InputBuffer));

			ImGui::End();
		}
	}

	void Cleanup()
	{
		g_Logs.clear();
		g_IsOpen = false;
		Log("[Console] Cleaned up");
	}
}
