#pragma once

/*
 * IL2CPP_API.hpp
 * 
 * Простая обертка вокруг IL2CPP_Resolver для удобства работы с IL2CPP runtime.
 * Содержит инициализацию и основные вспомогательные функции.
 */

#include <IL2CPP_Resolver.hpp>
#include "../console/console.h"

namespace IL2CPP_API
{
	// ============================================================================
	// ИНИЦИАЛИЗАЦИЯ
	// ============================================================================

	/// Инициализирует IL2CPP runtime
	/// Вызывается один раз при загрузке DLL
	inline bool Initialize()
	{
		Console::Log("[IL2CPP_API] Starting IL2CPP Runtime initialization...");

		// IL2CPP_Resolver requires game assembly to be already loaded
		if (!IL2CPP::Globals.m_GameAssembly)
		{
			Console::Warning("[IL2CPP_API] GameAssembly not yet loaded, waiting...");
			if (!IL2CPP::Initialize(true, 30))  // Wait up to 30 seconds
			{
				Console::Error("[IL2CPP_API] Failed to initialize IL2CPP_Resolver!");
				return false;
			}
		}
		else
		{
			if (!IL2CPP::Initialize())
			{
				Console::Error("[IL2CPP_API] Failed to initialize IL2CPP_Resolver!");
				return false;
			}
		}

		Console::Log("[IL2CPP_API] IL2CPP_Resolver initialized successfully");
		Console::Log("[IL2CPP_API] IL2CPP Runtime is ready!");
		return true;
	}

	// ============================================================================
	// РАБОТА С КЛАССАМИ
	// ============================================================================

	/// Проверяет, инициализирован ли IL2CPP
	inline bool IsInitialized()
	{
		return IL2CPP::Globals.m_GameAssembly != nullptr;
	}

	/// Получает класс по полному имени (e.g., "UnityEngine.Transform")
	inline Unity::il2cppClass* FindClass(const char* fullName)
	{
		if (!fullName || !IsInitialized())
			return nullptr;

		return IL2CPP::Class::Find(fullName);
	}

	/// Получает класс по namespace и name
	inline Unity::il2cppClass* FindClass(const char* namespaceName, const char* className)
	{
		if (!namespaceName || !className || !IsInitialized())
			return nullptr;

		// Используем поиск по полному имени
		std::string fullName = std::string(namespaceName) + "." + className;
		return IL2CPP::Class::Find(fullName.c_str());
	}

	// ============================================================================
	// РАБОТА С ПОЛЯМИ
	// ============================================================================

	/// Получает поле класса по имени
	inline Unity::il2cppFieldInfo* FindField(Unity::il2cppClass* klass, const char* fieldName)
	{
		if (!klass || !fieldName)
			return nullptr;

		void* iterator = nullptr;
		Unity::il2cppFieldInfo* field = nullptr;

		while (true)
		{
			field = IL2CPP::Class::GetFields(klass, &iterator);
			if (!field) break;

			if (field->m_pName && strcmp(field->m_pName, fieldName) == 0)
				return field;
		}

		return nullptr;
	}

	// ============================================================================
	// РАБОТА С МЕТОДАМИ
	// ============================================================================

	/// Получает метод класса по имени
	inline Unity::il2cppMethodInfo* FindMethod(Unity::il2cppClass* klass, const char* methodName)
	{
		if (!klass || !methodName)
			return nullptr;

		void* iterator = nullptr;
		Unity::il2cppMethodInfo* method = nullptr;

		while (true)
		{
			method = IL2CPP::Class::GetMethods(klass, &iterator);
			if (!method) break;

			if (method->m_pName && strcmp(method->m_pName, methodName) == 0)
				return method;
		}

		return nullptr;
	}

	/// Получает метод класса по имени и количеству параметров
	inline Unity::il2cppMethodInfo* FindMethod(Unity::il2cppClass* klass, const char* methodName, int paramCount)
	{
		if (!klass || !methodName || paramCount < 0)
			return nullptr;

		void* iterator = nullptr;
		Unity::il2cppMethodInfo* method = nullptr;

		while (true)
		{
			method = IL2CPP::Class::GetMethods(klass, &iterator);
			if (!method) break;

			if (method->m_pName && strcmp(method->m_pName, methodName) == 0 && method->m_uArgsCount == paramCount)
				return method;
		}

		return nullptr;
	}

	// ============================================================================
	// РАБОТА С ПОЛЯМИ ОБЪЕКТОВ
	// ============================================================================

	/// Получает значение поля объекта
	template<typename T>
	inline T GetFieldValue(void* instance, Unity::il2cppFieldInfo* field)
	{
		if (!instance || !field)
			return T();

		T value = {};
		memcpy(&value, reinterpret_cast<uint8_t*>(instance) + field->m_iOffset, sizeof(T));
		return value;
	}

	/// Устанавливает значение поля объекта
	template<typename T>
	inline void SetFieldValue(void* instance, Unity::il2cppFieldInfo* field, const T& value)
	{
		if (!instance || !field)
			return;

		memcpy(reinterpret_cast<uint8_t*>(instance) + field->m_iOffset, &value, sizeof(T));
	}

	// ============================================================================
	// РАБОТА С ОБЪЕКТАМИ
	// ============================================================================

	/// Получает класс объекта
	inline Unity::il2cppClass* GetObjectClass(void* obj)
	{
		if (!obj)
			return nullptr;

		Unity::il2cppObject* il2cppObj = reinterpret_cast<Unity::il2cppObject*>(obj);
		return il2cppObj->m_pClass;
	}
}
