#include "noclip.h"
#include "../il2cpp/il2cpp.h"
#include "../../utils/logger.h"
#include <cmath>

namespace CubixDLC
{
    namespace NoClip
    {
        // Safe helper functions for IL2CPP operations
        static bool SafeSetComponentEnabled(Unity::CComponent* comp, bool enabled)
        {
            if (!comp) return false;
            __try
            {
                // CharacterController inherits from Behaviour which has enabled property
                static void* setEnabledMethod = nullptr;
                if (!setEnabledMethod)
                {
                    setEnabledMethod = ::IL2CPP::Class::Utils::GetMethodPointer("UnityEngine.Behaviour", "set_enabled", 1);
                }
                if (setEnabledMethod)
                {
                    reinterpret_cast<void(*)(Unity::CComponent*, bool)>(setEnabledMethod)(comp, enabled);
                    return true;
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }
            return false;
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

        static void SafeSetPosition(Unity::CTransform* transform, Unity::Vector3 pos)
        {
            if (!transform) return;
            __try
            {
                transform->SetPosition(pos);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }

        // Get forward vector using Transform.TransformDirection
        static Unity::Vector3 SafeGetForward(Unity::CTransform* transform)
        {
            Unity::Vector3 result = {0, 0, 1};
            if (!transform) return result;
            __try
            {
                static void* transformDirectionMethod = nullptr;
                if (!transformDirectionMethod)
                {
                    transformDirectionMethod = ::IL2CPP::Class::Utils::GetMethodPointer("UnityEngine.Transform", "TransformDirection", 1);
                }
                if (transformDirectionMethod)
                {
                    Unity::Vector3 forward = {0, 0, 1};
                    result = reinterpret_cast<Unity::Vector3(*)(Unity::CTransform*, Unity::Vector3)>(transformDirectionMethod)(transform, forward);
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                result = {0, 0, 1};
            }
            return result;
        }

        // Get right vector using Transform.TransformDirection
        static Unity::Vector3 SafeGetRight(Unity::CTransform* transform)
        {
            Unity::Vector3 result = {1, 0, 0};
            if (!transform) return result;
            __try
            {
                static void* transformDirectionMethod = nullptr;
                if (!transformDirectionMethod)
                {
                    transformDirectionMethod = ::IL2CPP::Class::Utils::GetMethodPointer("UnityEngine.Transform", "TransformDirection", 1);
                }
                if (transformDirectionMethod)
                {
                    Unity::Vector3 right = {1, 0, 0};
                    result = reinterpret_cast<Unity::Vector3(*)(Unity::CTransform*, Unity::Vector3)>(transformDirectionMethod)(transform, right);
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                result = {1, 0, 0};
            }
            return result;
        }

        static float GetDeltaTime()
        {
            float deltaTime = 0.016f; // Default ~60fps
            __try
            {
                static void* getDeltaTimeMethod = nullptr;
                if (!getDeltaTimeMethod)
                {
                    getDeltaTimeMethod = ::IL2CPP::Class::Utils::GetMethodPointer("UnityEngine.Time", "get_deltaTime", 0);
                }
                if (getDeltaTimeMethod)
                {
                    deltaTime = reinterpret_cast<float(*)()>(getDeltaTimeMethod)();
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                deltaTime = 0.016f;
            }
            return deltaTime;
        }

        NoClipModule::NoClipModule()
        {
            IL2CPP::ModuleManager::GetInstance().RegisterModule(this);
        }

        NoClipModule::~NoClipModule()
        {
            SetEnabled(false);
            IL2CPP::ModuleManager::GetInstance().UnregisterModule(this);
        }

        void NoClipModule::Toggle()
        {
            SetEnabled(!m_Enabled);
        }

        void NoClipModule::OnEnable()
        {
            LOG_INFO("NoClip Module enabled");
            
            // Find player and disable CharacterController
            m_Player = CubixDLC::IL2CPP::Utils::FindGameObject("Player");
            if (m_Player)
            {
                m_CharController = m_Player->GetComponent("CharacterController");
                if (m_CharController)
                {
                    // Disable CharacterController
                    SafeSetComponentEnabled(m_CharController, false);
                    LOG_DEBUG("NoClip: CharacterController disabled");
                }
            }
        }

        void NoClipModule::OnDisable()
        {
            LOG_INFO("NoClip Module disabled");
            
            // Re-enable CharacterController
            if (m_CharController)
            {
                SafeSetComponentEnabled(m_CharController, true);
                LOG_DEBUG("NoClip: CharacterController re-enabled");
            }
            
            m_Player = nullptr;
            m_CharController = nullptr;
        }

        void NoClipModule::OnUpdate()
        {
            if (!m_Enabled) return;

            // Find player if not cached
            if (!m_Player)
            {
                m_Player = CubixDLC::IL2CPP::Utils::FindGameObject("Player");
                if (!m_Player) return;
                
                m_CharController = m_Player->GetComponent("CharacterController");
                if (m_CharController)
                {
                    SafeSetComponentEnabled(m_CharController, false);
                }
            }

            Unity::CTransform* playerTransform = SafeGetTransform(m_Player);
            if (!playerTransform) return;

            Unity::Vector3 moveDir = {0, 0, 0};
            Unity::Vector3 forward = SafeGetForward(playerTransform);
            Unity::Vector3 right = SafeGetRight(playerTransform);

            // Check keyboard input using GetAsyncKeyState (Windows API)
            // W = 0x57, S = 0x53, A = 0x41, D = 0x44
            // Space = 0x20, LeftControl = 0x11, LeftAlt = 0x12

            if (GetAsyncKeyState(0x57) & 0x8000) // W
            {
                moveDir.x += forward.x;
                moveDir.y += forward.y;
                moveDir.z += forward.z;
            }
            if (GetAsyncKeyState(0x53) & 0x8000) // S
            {
                moveDir.x -= forward.x;
                moveDir.y -= forward.y;
                moveDir.z -= forward.z;
            }
            if (GetAsyncKeyState(0x41) & 0x8000) // A
            {
                moveDir.x -= right.x;
                moveDir.y -= right.y;
                moveDir.z -= right.z;
            }
            if (GetAsyncKeyState(0x44) & 0x8000) // D
            {
                moveDir.x += right.x;
                moveDir.y += right.y;
                moveDir.z += right.z;
            }
            if (GetAsyncKeyState(0x20) & 0x8000) // Space - up
            {
                moveDir.y += 1.0f;
            }
            if (GetAsyncKeyState(0x11) & 0x8000) // Left Control - down
            {
                moveDir.y -= 1.0f;
            }
            if (GetAsyncKeyState(0x12) & 0x8000) // Left Alt - up (alternative)
            {
                moveDir.y += 1.0f;
            }

            // Normalize and apply speed
            float length = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y + moveDir.z * moveDir.z);
            if (length > 0.001f)
            {
                float deltaTime = GetDeltaTime();

                // Normalize
                moveDir.x /= length;
                moveDir.y /= length;
                moveDir.z /= length;

                // Apply speed and delta time
                float speed = m_NoclipSpeed * deltaTime;
                moveDir.x *= speed;
                moveDir.y *= speed;
                moveDir.z *= speed;

                // Get current position and add movement
                Unity::Vector3 currentPos = SafeGetPosition(playerTransform);
                currentPos.x += moveDir.x;
                currentPos.y += moveDir.y;
                currentPos.z += moveDir.z;

                // Set new position
                SafeSetPosition(playerTransform, currentPos);
            }
        }
    }
}
