#pragma once

#include "../il2cpp/il2cpp.h"

// Пример модуля, использующего IL2CPP систему
// Этот модуль демонстрирует, как легко создавать новые модули без глобальных переменных

namespace CubixDLC
{
    namespace ExampleModule
    {
        class ExampleModule : public IL2CPP::ModuleBase
        {
        public:
            ExampleModule()
            {
                // Автоматическая регистрация модуля
                IL2CPP::ModuleManager::GetInstance().RegisterModule(this);
            }

            ~ExampleModule()
            {
                // Отключение модуля при уничтожении
                SetEnabled(false);
                IL2CPP::ModuleManager::GetInstance().UnregisterModule(this);
            }

            // Вызывается при включении модуля
            void OnEnable() override
            {
                // Здесь можно инициализировать модуль
                // Например, найти нужные GameObject или Component
                m_LocalPlayer = IL2CPP::Utils::FindGameObject("LocalPlayer");
            }

            // Вызывается при выключении модуля
            void OnDisable() override
            {
                // Очистка при выключении
                m_LocalPlayer = nullptr;
            }

            // Вызывается каждый кадр
            void OnUpdate() override
            {
                if (!m_LocalPlayer)
                {
                    m_LocalPlayer = IL2CPP::Utils::FindGameObject("LocalPlayer");
                    return;
                }

                // Пример работы с компонентом
                if (!m_PlayerData)
                {
                    m_PlayerData = IL2CPP::Utils::GetComponent(m_LocalPlayer, "PlayerData");
                }

                if (m_PlayerData)
                {
                    // Пример изменения значения через IL2CPP
                    // m_PlayerData->SetMemberValue<bool>("CanFly", true);
                }
            }

        private:
            Unity::CGameObject* m_LocalPlayer = nullptr;
            Unity::CComponent* m_PlayerData = nullptr;
        };

        // Глобальный экземпляр модуля (можно сделать singleton если нужно)
        inline ExampleModule* GetModule()
        {
            static ExampleModule module;
            return &module;
        }
    }
}

