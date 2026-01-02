#include "esp.h"
#include "../il2cpp/il2cpp.h"
#include "../../deps/imgui/imgui.h"
#include "../../utils/logger.h"
#include <algorithm>
#include <cmath>

// Field offsets from decompiled game source (granny-legacy-v1.7.1)
// EnemyController class:
//   Grandpa (GameObject) at offset 0x30
//   Granny (GameObject) at offset 0x38
#define ENEMYCONTROLLER_GRANDPA_OFFSET 0x30
#define ENEMYCONTROLLER_GRANNY_OFFSET 0x38

namespace CubixDLC
{
    namespace ESPEnemy
    {
        // Cached EnemyController component pointer
        static Unity::CComponent* s_EnemyControllerComponent = nullptr;

        // Helper functions to safely read memory (SEH compatible - no C++ objects)
        static Unity::CGameObject* SafeReadGameObject(uintptr_t addr)
        {
            Unity::CGameObject* result = nullptr;
            __try
            {
                result = *reinterpret_cast<Unity::CGameObject**>(addr);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                result = nullptr;
            }
            return result;
        }

        static bool SafeIsActive(Unity::CGameObject* obj)
        {
            if (!obj) return false;
            bool result = false;
            __try
            {
                result = obj->GetActive();
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                result = false;
            }
            return result;
        }

        static Unity::CTransform* SafeGetTransform(Unity::CGameObject* obj)
        {
            if (!obj) return nullptr;
            Unity::CTransform* result = nullptr;
            __try
            {
                result = obj->GetTransform();
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                result = nullptr;
            }
            return result;
        }

        static Unity::Vector3 SafeGetPosition(Unity::CTransform* transform)
        {
            Unity::Vector3 result = {0, 0, 0};
            if (!transform) return result;
            __try
            {
                result = transform->GetPosition();
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                result = {0, 0, 0};
            }
            return result;
        }

        // Get position directly from ec.Granny.transform.position or ec.Grandpa.transform.position
        static bool GetEnemyPositionFromEC(int offset, Unity::Vector3& outPos)
        {
            if (!s_EnemyControllerComponent)
                return false;

            __try
            {
                uintptr_t ecAddr = reinterpret_cast<uintptr_t>(s_EnemyControllerComponent);
                Unity::CGameObject* enemyObj = *reinterpret_cast<Unity::CGameObject**>(ecAddr + offset);
                if (!enemyObj)
                    return false;

                Unity::CTransform* transform = enemyObj->GetTransform();
                if (!transform)
                    return false;

                outPos = transform->GetPosition();
                return true;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
        }

        ESPEnemyModule::ESPEnemyModule()
        {
            IL2CPP::ModuleManager::GetInstance().RegisterModule(this);
        }

        ESPEnemyModule::~ESPEnemyModule()
        {
            SetEnabled(false);
            IL2CPP::ModuleManager::GetInstance().UnregisterModule(this);
        }

        void ESPEnemyModule::OnEnable()
        {
            LOG_INFO("ESP Enemy Module enabled");
            m_Enemies.clear();
            m_MainCamera = nullptr;
            m_Player = nullptr;
            m_EnemyController = nullptr;
            m_LastUpdateTime = 0.0f;
        }

        void ESPEnemyModule::OnDisable()
        {
            m_Enemies.clear();
        }

        void ESPEnemyModule::OnUpdate()
        {
            if (!m_Settings.enabled)
                return;

            float currentTime = (float)ImGui::GetTime();
            if (currentTime - m_LastUpdateTime >= m_Settings.updateInterval)
            {
                FindEnemies();
                m_LastUpdateTime = currentTime;
            }
        }

        void ESPEnemyModule::OnLateUpdate()
        {
            if (!m_Settings.enabled)
                return;

            // Get camera
            if (!m_MainCamera)
            {
                m_MainCamera = Unity::Camera::GetMain();
                if (m_MainCamera)
                {
                    LOG_DEBUG("ESP Enemy: Main camera found");
                }
            }

            // Get player
            if (!m_Player)
            {
                m_Player = CubixDLC::IL2CPP::Utils::FindGameObject("Player");
                if (m_Player)
                {
                    LOG_DEBUG("ESP Enemy: Player found");
                }
            }

            // Update enemy info
            for (auto& enemy : m_Enemies)
            {
                UpdateEnemyInfo(enemy);
            }
        }

        void ESPEnemyModule::Render()
        {
            if (!m_Settings.enabled)
                return;

            for (const auto& enemy : m_Enemies)
            {
                if (!enemy.isValid)
                    continue;

                auto it = m_Settings.enemyColors.find(enemy.type);
                if (it == m_Settings.enemyColors.end() || !it->second.enabled)
                    continue;

                if (enemy.distance > m_Settings.maxDistance)
                    continue;

                if (!enemy.isOnScreen)
                    continue;

                if (m_Settings.showBox)
                {
                    RenderBox(enemy);
                }

                if (m_Settings.showName || m_Settings.showDistance)
                {
                    RenderInfo(enemy);
                }
            }
        }

        void ESPEnemyModule::FindEnemies()
        {
            if (!IL2CPP::Manager::GetInstance().IsInitialized())
            {
                return;
            }

            m_Enemies.clear();

            // Find EnemyController first - it contains references to Granny and Grandpa
            if (!m_EnemyController || !SafeIsActive(m_EnemyController))
            {
                m_EnemyController = CubixDLC::IL2CPP::Utils::FindGameObject("EnemyController");
                if (!m_EnemyController)
                {
                    // Try alternative names
                    m_EnemyController = CubixDLC::IL2CPP::Utils::FindGameObject("Enemy Controller");
                }
                // Reset cached component when GameObject changes
                s_EnemyControllerComponent = nullptr;
            }

            if (m_EnemyController)
            {
                // Cache the EnemyController component for direct position access
                if (!s_EnemyControllerComponent)
                {
                    s_EnemyControllerComponent = CubixDLC::IL2CPP::Utils::GetComponent(m_EnemyController, "EnemyController");
                }

                if (s_EnemyControllerComponent)
                {
                    uintptr_t ecAddr = reinterpret_cast<uintptr_t>(s_EnemyControllerComponent);

                    // Read Granny GameObject using field offset 0x38
                    // Position will be read directly via ec.Granny.transform.position in UpdateEnemyInfo
                    Unity::CGameObject* grannyObj = SafeReadGameObject(ecAddr + ENEMYCONTROLLER_GRANNY_OFFSET);
                    if (grannyObj && SafeIsActive(grannyObj))
                    {
                        EnemyInfo info;
                        info.gameObject = grannyObj;
                        info.type = EnemyType::Granny;
                        info.name = "Granny";
                        info.isValid = true;
                        info.worldPos = {0, 0, 0};
                        m_Enemies.push_back(info);
                        LOG_DEBUG("ESP Enemy: Found Granny at 0x%p (using ec.Granny.transform.position)", grannyObj);
                    }

                    // Read Grandpa GameObject using field offset 0x30
                    // Position will be read directly via ec.Grandpa.transform.position in UpdateEnemyInfo
                    Unity::CGameObject* grandpaObj = SafeReadGameObject(ecAddr + ENEMYCONTROLLER_GRANDPA_OFFSET);
                    if (grandpaObj && SafeIsActive(grandpaObj))
                    {
                        EnemyInfo info;
                        info.gameObject = grandpaObj;
                        info.type = EnemyType::Grandpa;
                        info.name = "Grandpa";
                        info.isValid = true;
                        info.worldPos = {0, 0, 0};
                        m_Enemies.push_back(info);
                        LOG_DEBUG("ESP Enemy: Found Grandpa at 0x%p (using ec.Grandpa.transform.position)", grandpaObj);
                    }
                }
            }

            static int logCounter = 0;
            if (logCounter++ % 50 == 0)
            {
                LOG_DEBUG("ESP Enemy: Found %zu enemies (using EnemyController)", m_Enemies.size());
            }
        }

        void ESPEnemyModule::UpdateEnemyInfo(EnemyInfo& enemy)
        {
            if (!enemy.isValid)
            {
                enemy.isValid = false;
                return;
            }

            // Get position directly via ec.Granny.transform.position or ec.Grandpa.transform.position
            Unity::Vector3 worldPos = {0, 0, 0};
            int offset = (enemy.type == EnemyType::Granny) ? ENEMYCONTROLLER_GRANNY_OFFSET : ENEMYCONTROLLER_GRANDPA_OFFSET;
            
            if (!GetEnemyPositionFromEC(offset, worldPos))
            {
                // Fallback to old method if EC fails
                if (enemy.gameObject && SafeIsActive(enemy.gameObject))
                {
                    Unity::CTransform* transform = SafeGetTransform(enemy.gameObject);
                    if (transform)
                    {
                        worldPos = SafeGetPosition(transform);
                    }
                    else
                    {
                        enemy.isValid = false;
                        return;
                    }
                }
                else
                {
                    enemy.isValid = false;
                    return;
                }
            }

            // Store world position for box calculation
            enemy.worldPos = worldPos;
            
            // Convert to screen coordinates
            enemy.isOnScreen = WorldToScreen(worldPos, enemy.screenPos);

            if (enemy.isOnScreen)
            {
                // Calculate distance to player
                if (m_Player)
                {
                    Unity::CTransform* playerTransform = SafeGetTransform(m_Player);
                    if (playerTransform)
                    {
                        Unity::Vector3 playerPos = SafeGetPosition(playerTransform);
                        float dx = worldPos.x - playerPos.x;
                        float dy = worldPos.y - playerPos.y;
                        float dz = worldPos.z - playerPos.z;
                        enemy.distance = sqrtf(dx * dx + dy * dy + dz * dz);
                    }
                }

                CalculateBox(enemy);
            }
        }

        bool ESPEnemyModule::WorldToScreen(const Unity::Vector3& worldPos, ImVec2& screenPos)
        {
            if (!m_MainCamera)
            {
                m_MainCamera = Unity::Camera::GetMain();
                if (!m_MainCamera)
                {
                    return false;
                }
            }

            Unity::Vector3 worldPosRef = worldPos;
            Unity::Vector3 screenVec;
            
            __try
            {
                m_MainCamera->WorldToScreen(worldPosRef, screenVec, 2);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }

            // Check if point is in front of camera
            if (screenVec.z <= 0.0f)
                return false;

            ImVec2 screenSize = ImGui::GetIO().DisplaySize;
            screenPos.x = screenVec.x;
            screenPos.y = screenSize.y - screenVec.y; // Invert Y

            const float margin = 50.0f;
            return (screenPos.x >= -margin && screenPos.x <= screenSize.x + margin &&
                    screenPos.y >= -margin && screenPos.y <= screenSize.y + margin);
        }

        void ESPEnemyModule::CalculateBox(EnemyInfo& enemy)
        {
            if (!enemy.isOnScreen)
                return;

            float enemyHeight = 2.0f;

            // Use stored worldPos from ec.Granny/Grandpa.transform.position
            Unity::Vector3 basePos = enemy.worldPos;
            Unity::Vector3 topPos = basePos;
            topPos.y += enemyHeight;

            Unity::Vector3 bottomPos = basePos;
            bottomPos.y -= 0.5f;

            ImVec2 topScreen, bottomScreen;
            if (WorldToScreen(topPos, topScreen) && WorldToScreen(bottomPos, bottomScreen))
            {
                float height = bottomScreen.y - topScreen.y;
                float width = height * 0.5f;

                enemy.boxMin = ImVec2(enemy.screenPos.x - width * 0.5f, topScreen.y);
                enemy.boxMax = ImVec2(enemy.screenPos.x + width * 0.5f, bottomScreen.y);
            }
            else
            {
                float boxSize = 30.0f;
                enemy.boxMin = ImVec2(enemy.screenPos.x - boxSize, enemy.screenPos.y - boxSize);
                enemy.boxMax = ImVec2(enemy.screenPos.x + boxSize, enemy.screenPos.y + boxSize);
            }
        }

        void ESPEnemyModule::RenderBox(const EnemyInfo& enemy)
        {
            auto it = m_Settings.enemyColors.find(enemy.type);
            if (it == m_Settings.enemyColors.end())
                return;

            ImDrawList* drawList = ImGui::GetForegroundDrawList();
            ImU32 color = ImGui::ColorConvertFloat4ToU32(it->second.color);

            drawList->AddRect(enemy.boxMin, enemy.boxMax, color, 0.0f, 0, 2.0f);

            float cornerSize = 8.0f;
            float thickness = 2.0f;

            // Top left
            drawList->AddLine(enemy.boxMin, ImVec2(enemy.boxMin.x + cornerSize, enemy.boxMin.y), color, thickness);
            drawList->AddLine(enemy.boxMin, ImVec2(enemy.boxMin.x, enemy.boxMin.y + cornerSize), color, thickness);

            // Top right
            drawList->AddLine(ImVec2(enemy.boxMax.x, enemy.boxMin.y), ImVec2(enemy.boxMax.x - cornerSize, enemy.boxMin.y), color, thickness);
            drawList->AddLine(ImVec2(enemy.boxMax.x, enemy.boxMin.y), ImVec2(enemy.boxMax.x, enemy.boxMin.y + cornerSize), color, thickness);

            // Bottom left
            drawList->AddLine(ImVec2(enemy.boxMin.x, enemy.boxMax.y), ImVec2(enemy.boxMin.x + cornerSize, enemy.boxMax.y), color, thickness);
            drawList->AddLine(ImVec2(enemy.boxMin.x, enemy.boxMax.y), ImVec2(enemy.boxMin.x, enemy.boxMax.y - cornerSize), color, thickness);

            // Bottom right
            drawList->AddLine(enemy.boxMax, ImVec2(enemy.boxMax.x - cornerSize, enemy.boxMax.y), color, thickness);
            drawList->AddLine(enemy.boxMax, ImVec2(enemy.boxMax.x, enemy.boxMax.y - cornerSize), color, thickness);
        }

        void ESPEnemyModule::RenderInfo(const EnemyInfo& enemy)
        {
            auto it = m_Settings.enemyColors.find(enemy.type);
            if (it == m_Settings.enemyColors.end())
                return;

            ImDrawList* drawList = ImGui::GetForegroundDrawList();
            ImU32 color = ImGui::ColorConvertFloat4ToU32(it->second.color);

            ImFont* font = ImGui::GetFont();
            float font_size = ImGui::GetFontSize();

            // Build text without std::string for simpler memory
            char text[64] = {0};
            int textLen = 0;
            
            if (m_Settings.showName)
            {
                textLen = snprintf(text, sizeof(text), "%s", enemy.name.c_str());
            }
            if (m_Settings.showDistance && enemy.distance > 0.0f)
            {
                if (textLen > 0)
                {
                    textLen += snprintf(text + textLen, sizeof(text) - textLen, " %.1fm", enemy.distance);
                }
                else
                {
                    textLen = snprintf(text, sizeof(text), "%.1fm", enemy.distance);
                }
            }

            if (textLen == 0)
                return;

            ImVec2 textSize = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text);
            ImVec2 textPos = ImVec2(enemy.screenPos.x - textSize.x * 0.5f, enemy.boxMin.y - textSize.y - 2.0f);

            ImVec2 bgMin = ImVec2(textPos.x - 2.0f, textPos.y - 1.0f);
            ImVec2 bgMax = ImVec2(textPos.x + textSize.x + 2.0f, textPos.y + textSize.y + 1.0f);
            drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 150), 2.0f);

            drawList->AddText(font, font_size, textPos, color, text);
        }
    }
}
