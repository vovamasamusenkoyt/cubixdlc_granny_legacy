#pragma once

#include "../il2cpp/il2cpp.h"
#include "../../deps/imgui/imgui.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace CubixDLC
{
    namespace ESPEnemy
    {
        // Типы врагов (только Granny и Grandpa)
        enum class EnemyType
        {
            Granny,
            Grandpa
        };

        // Structure for storing enemy info
        struct EnemyInfo
        {
            Unity::CGameObject* gameObject = nullptr;
            EnemyType type = EnemyType::Granny;
            std::string name;
            bool isValid = false;
            
            // World position from ec.Granny/Grandpa.transform.position
            Unity::Vector3 worldPos = {0, 0, 0};
            
            // Cached render data
            bool isOnScreen = false;
            ImVec2 screenPos = ImVec2(0, 0);
            ImVec2 boxMin = ImVec2(0, 0);
            ImVec2 boxMax = ImVec2(0, 0);
            float distance = 0.0f;
        };

        // Настройки цвета для каждого типа врага
        struct EnemyColorSettings
        {
            ImVec4 color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            bool enabled = true;
        };

        // Настройки ESP модуля
        struct ESPSettings
        {
            bool enabled = false;
            bool showBox = true;
            bool showName = true;
            bool showDistance = true;
            float maxDistance = 100.0f;
            float updateInterval = 0.1f;
            
            std::unordered_map<EnemyType, EnemyColorSettings> enemyColors;
            
            ESPSettings()
            {
                enemyColors[EnemyType::Granny] = { ImVec4(1.0f, 0.0f, 0.0f, 1.0f), true };   // Красный
                enemyColors[EnemyType::Grandpa] = { ImVec4(0.0f, 0.0f, 1.0f, 1.0f), true };  // Синий
            }
        };

        // Модуль ESP Enemy
        class ESPEnemyModule : public IL2CPP::ModuleBase
        {
        public:
            ESPEnemyModule();
            ~ESPEnemyModule();

            void OnEnable() override;
            void OnDisable() override;
            void OnUpdate() override;
            void OnLateUpdate() override;

            void Render();
            ESPSettings& GetSettings() { return m_Settings; }
            const std::vector<EnemyInfo>& GetEnemies() const { return m_Enemies; }

        private:
            void FindEnemies();
            void UpdateEnemyInfo(EnemyInfo& enemy);
            bool WorldToScreen(const Unity::Vector3& worldPos, ImVec2& screenPos);
            void CalculateBox(EnemyInfo& enemy);
            void RenderBox(const EnemyInfo& enemy);
            void RenderInfo(const EnemyInfo& enemy);

            ESPSettings m_Settings;
            std::vector<EnemyInfo> m_Enemies;
            Unity::CCamera* m_MainCamera = nullptr;
            Unity::CGameObject* m_Player = nullptr;
            Unity::CGameObject* m_EnemyController = nullptr;
            float m_LastUpdateTime = 0.0f;
        };

        inline ESPEnemyModule* GetModule()
        {
            static ESPEnemyModule module;
            return &module;
        }
    }
}
