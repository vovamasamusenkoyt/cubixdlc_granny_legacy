#pragma once

// Include IL2CPP_Resolver only if not already included
#ifndef IL2CPP_RESOLVER_INCLUDED
#define IL2CPP_RESOLVER_INCLUDED
#include "../../deps/IL2CPP_Resolver/IL2CPP_Resolver.hpp"
#endif

#include <memory>
#include <functional>
#include <vector>
#include <algorithm>

namespace CubixDLC
{
    namespace IL2CPP
    {
        // Singleton для управления IL2CPP инициализацией
        class Manager
        {
        public:
            static Manager& GetInstance()
            {
                static Manager instance;
                return instance;
            }

            // Инициализация IL2CPP (вызывается один раз)
            bool Initialize(bool waitForModule = true, int maxSecondsWait = 60)
            {
                if (m_Initialized)
                    return true;

                m_Initialized = ::IL2CPP::Initialize(waitForModule, maxSecondsWait);
                // Callbacks инициализируются в основной функции Initialize()
                return m_Initialized;
            }

            // Проверка инициализации
            bool IsInitialized() const { return m_Initialized; }

            // Очистка (при выгрузке DLL)
            void Shutdown()
            {
                if (m_Initialized)
                {
                    ::IL2CPP::Callback::Uninitialize();
                    m_Initialized = false;
                }
            }

        private:
            Manager() = default;
            ~Manager() = default;
            Manager(const Manager&) = delete;
            Manager& operator=(const Manager&) = delete;

            bool m_Initialized = false;
        };

        // Обертка для работы с Unity объектами
        class UnityObject
        {
        public:
            UnityObject(Unity::CGameObject* obj) : m_GameObject(obj) {}
            UnityObject(Unity::CComponent* comp) : m_Component(comp) {}

            // Получить GameObject
            Unity::CGameObject* GetGameObject() const
            {
                if (m_GameObject)
                    return m_GameObject;
                if (m_Component)
                    return m_Component->GetGameObject();
                return nullptr;
            }

            // Получить Component по имени
            Unity::CComponent* GetComponent(const char* name) const
            {
                auto* go = GetGameObject();
                if (!go) return nullptr;
                return go->GetComponent(name);
            }

            // Получить Transform
            Unity::CTransform* GetTransform() const
            {
                auto* go = GetGameObject();
                if (!go) return nullptr;
                return go->GetTransform();
            }

            // Проверка валидности
            bool IsValid() const
            {
                return m_GameObject != nullptr || m_Component != nullptr;
            }

        private:
            Unity::CGameObject* m_GameObject = nullptr;
            Unity::CComponent* m_Component = nullptr;
        };

        // Утилиты для поиска объектов
        namespace Utils
        {
            // Найти GameObject по имени
            inline Unity::CGameObject* FindGameObject(const char* name)
            {
                if (!Manager::GetInstance().IsInitialized())
                    return nullptr;
                return Unity::GameObject::Find(name);
            }

            // Найти все GameObject с тегом
            inline Unity::il2cppArray<Unity::CGameObject*>* FindGameObjectsWithTag(const char* tag)
            {
                if (!Manager::GetInstance().IsInitialized())
                    return nullptr;
                return Unity::GameObject::FindWithTag(tag);
            }

            // Получить компонент из GameObject
            inline Unity::CComponent* GetComponent(Unity::CGameObject* obj, const char* componentName)
            {
                if (!obj) return nullptr;
                return obj->GetComponent(componentName);
            }
        }

        // Базовый класс для модулей, использующих IL2CPP
        class ModuleBase
        {
        public:
            virtual ~ModuleBase() = default;

            // Вызывается при включении модуля
            virtual void OnEnable() {}

            // Вызывается при выключении модуля
            virtual void OnDisable() {}

            // Вызывается каждый кадр (если модуль включен)
            virtual void OnUpdate() {}

            // Вызывается после Update (если модуль включен)
            virtual void OnLateUpdate() {}

            // Проверка, включен ли модуль
            bool IsEnabled() const { return m_Enabled; }
            void SetEnabled(bool enabled) 
            { 
                if (m_Enabled != enabled)
                {
                    m_Enabled = enabled;
                    if (enabled)
                        OnEnable();
                    else
                        OnDisable();
                }
            }

        protected:
            bool m_Enabled = false;
        };

        // Менеджер модулей
        class ModuleManager
        {
        public:
            static ModuleManager& GetInstance()
            {
                static ModuleManager instance;
                return instance;
            }

            // Регистрация модуля
            void RegisterModule(ModuleBase* module)
            {
                if (module && std::find(m_Modules.begin(), m_Modules.end(), module) == m_Modules.end())
                {
                    m_Modules.push_back(module);
                }
            }

            // Удаление модуля
            void UnregisterModule(ModuleBase* module)
            {
                auto it = std::find(m_Modules.begin(), m_Modules.end(), module);
                if (it != m_Modules.end())
                {
                    m_Modules.erase(it);
                }
            }

            // Вызов OnUpdate для всех включенных модулей
            void UpdateAll()
            {
                for (auto* module : m_Modules)
                {
                    if (module && module->IsEnabled())
                    {
                        module->OnUpdate();
                    }
                }
            }

            // Вызов OnLateUpdate для всех включенных модулей
            void LateUpdateAll()
            {
                for (auto* module : m_Modules)
                {
                    if (module && module->IsEnabled())
                    {
                        module->OnLateUpdate();
                    }
                }
            }

        private:
            ModuleManager() = default;
            ~ModuleManager() = default;
            ModuleManager(const ModuleManager&) = delete;
            ModuleManager& operator=(const ModuleManager&) = delete;

            std::vector<ModuleBase*> m_Modules;
        };

        // Функции для callbacks (должны быть объявлены вне inline)
        namespace Internal
        {
            void OnUpdateCallback();
            void OnLateUpdateCallback();
        }

        // Инициализация IL2CPP системы
        inline bool Initialize(bool waitForModule = true, int maxSecondsWait = 60)
        {
            bool result = Manager::GetInstance().Initialize(waitForModule, maxSecondsWait);
            
            if (result)
            {
                // Инициализируем callbacks
                ::IL2CPP::Callback::Initialize();
                
                // Регистрируем callbacks для модулей
                ::IL2CPP::Callback::OnUpdate::Add(reinterpret_cast<void*>(Internal::OnUpdateCallback));
                ::IL2CPP::Callback::OnLateUpdate::Add(reinterpret_cast<void*>(Internal::OnLateUpdateCallback));
            }

            return result;
        }

        // Очистка IL2CPP системы
        inline void Shutdown()
        {
            Manager::GetInstance().Shutdown();
        }
    }
}

