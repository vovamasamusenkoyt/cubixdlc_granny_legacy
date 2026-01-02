#include "invisible.h"
#include "../../utils/logger.h"

namespace CubixDLC
{
    namespace Invisible
    {
        // AI_Granny field offsets from source
        #define GRANNY_UNSEENPLAYER_OFFSET      0x100   // bool
        #define GRANNY_CAUGHTPLAYER_OFFSET      0x158   // bool
        #define GRANNY_ISANGRY_OFFSET           0x15B   // bool
        #define GRANNY_ISFOLLOWINGSOUND_OFFSET  0x15C   // bool
        #define GRANNY_ISCHASING_OFFSET         0x15D   // bool
        #define GRANNY_PLAYERCLOSE_OFFSET       0x190   // bool
        #define GRANNY_PLAYERTOUCHEDRAY_OFFSET  0x230   // bool

        // AI_Grandpa field offsets from source
        #define GRANDPA_UNSEENPLAYER_OFFSET     0x108   // bool
        #define GRANDPA_ISSHOOTING_OFFSET       0x94    // bool
        #define GRANDPA_CAUGHTPLAYER_OFFSET     0x1B0   // bool
        #define GRANDPA_ISANGRY_OFFSET          0x1B3   // bool
        #define GRANDPA_ISFOLLOWINGSOUND_OFFSET 0x1B4   // bool
        #define GRANDPA_ISCHASING_OFFSET        0x1B5   // bool
        #define GRANDPA_PLAYERCLOSE_OFFSET      0x1E8   // bool
        #define GRANDPA_PLAYERTOUCHEDRAY_OFFSET 0x288   // bool

        InvisibleModule::InvisibleModule()
        {
            IL2CPP::ModuleManager::GetInstance().RegisterModule(this);
            LOG_INFO("Invisible module initialized");
        }

        InvisibleModule::~InvisibleModule()
        {
        }

        void InvisibleModule::OnEnable()
        {
            m_Enabled = true;
            LOG_INFO("Invisible enabled - enemies can't see you!");
        }

        void InvisibleModule::OnDisable()
        {
            m_Enabled = false;
            LOG_INFO("Invisible disabled");
        }

        void InvisibleModule::OnUpdate()
        {
            if (!m_Enabled)
                return;

            // Find enemies if needed
            if (!m_Granny || !m_Grandpa)
            {
                float currentTime = GetTickCount64() / 1000.0f;
                if (currentTime - m_LastSearchTime > 2.0f)
                {
                    m_LastSearchTime = currentTime;
                    FindEnemies();
                }
            }

            // Apply invisibility
            ApplyInvisibility();
        }

        void InvisibleModule::FindEnemies()
        {
            // Find Granny
            if (!m_Granny)
            {
                __try
                {
                    Unity::il2cppArray<Unity::CGameObject*>* grannyObjs = Unity::Object::FindObjectsOfType<Unity::CGameObject>("AI_Granny");
                    if (grannyObjs && grannyObjs->m_uMaxLength > 0)
                    {
                        Unity::CGameObject* grannyObj = grannyObjs->operator[](0);
                        if (grannyObj)
                        {
                            m_Granny = CubixDLC::IL2CPP::Utils::GetComponent(grannyObj, "AI_Granny");
                            if (m_Granny)
                            {
                                LOG_INFO("Invisible: Found Granny AI at 0x%p", m_Granny);
                            }
                        }
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    m_Granny = nullptr;
                }
            }

            // Find Grandpa
            if (!m_Grandpa)
            {
                __try
                {
                    Unity::il2cppArray<Unity::CGameObject*>* grandpaObjs = Unity::Object::FindObjectsOfType<Unity::CGameObject>("AI_Grandpa");
                    if (grandpaObjs && grandpaObjs->m_uMaxLength > 0)
                    {
                        Unity::CGameObject* grandpaObj = grandpaObjs->operator[](0);
                        if (grandpaObj)
                        {
                            m_Grandpa = CubixDLC::IL2CPP::Utils::GetComponent(grandpaObj, "AI_Grandpa");
                            if (m_Grandpa)
                            {
                                LOG_INFO("Invisible: Found Grandpa AI at 0x%p", m_Grandpa);
                            }
                        }
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    m_Grandpa = nullptr;
                }
            }
        }

        void InvisibleModule::ApplyInvisibility()
        {
            if (m_Granny)
            {
                __try
                {
                    ApplyToGranny(reinterpret_cast<uintptr_t>(m_Granny));
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    m_Granny = nullptr;
                    LOG_WARNING("Invisible: Granny access failed, will retry");
                }
            }

            if (m_Grandpa)
            {
                __try
                {
                    ApplyToGrandpa(reinterpret_cast<uintptr_t>(m_Grandpa));
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    m_Grandpa = nullptr;
                    LOG_WARNING("Invisible: Grandpa access failed, will retry");
                }
            }
        }

        void InvisibleModule::ApplyToGranny(uintptr_t addr)
        {
            // UnSeenPlayer = true (enemy doesn't see player)
            *reinterpret_cast<bool*>(addr + GRANNY_UNSEENPLAYER_OFFSET) = true;
            
            // CaughtPlayer = false (player not caught)
            *reinterpret_cast<bool*>(addr + GRANNY_CAUGHTPLAYER_OFFSET) = false;
            
            // IsAngry = false (not angry)
            *reinterpret_cast<bool*>(addr + GRANNY_ISANGRY_OFFSET) = false;
            
            // IsFollowingSound = false (not following sounds)
            *reinterpret_cast<bool*>(addr + GRANNY_ISFOLLOWINGSOUND_OFFSET) = false;
            
            // IsChasing = false (not chasing)
            *reinterpret_cast<bool*>(addr + GRANNY_ISCHASING_OFFSET) = false;
            
            // PlayerClose = false (player not nearby)
            *reinterpret_cast<bool*>(addr + GRANNY_PLAYERCLOSE_OFFSET) = false;
            
            // PlayerTouchedRay = false (raycast didn't hit player)
            *reinterpret_cast<bool*>(addr + GRANNY_PLAYERTOUCHEDRAY_OFFSET) = false;
        }

        void InvisibleModule::ApplyToGrandpa(uintptr_t addr)
        {
            // UnSeenPlayer = true (enemy doesn't see player)
            *reinterpret_cast<bool*>(addr + GRANDPA_UNSEENPLAYER_OFFSET) = true;
            
            // IsShooting = false (not shooting)
            *reinterpret_cast<bool*>(addr + GRANDPA_ISSHOOTING_OFFSET) = false;
            
            // CaughtPlayer = false (player not caught)
            *reinterpret_cast<bool*>(addr + GRANDPA_CAUGHTPLAYER_OFFSET) = false;
            
            // IsAngry = false (not angry)
            *reinterpret_cast<bool*>(addr + GRANDPA_ISANGRY_OFFSET) = false;
            
            // IsFollowingSound = false (not following sounds)
            *reinterpret_cast<bool*>(addr + GRANDPA_ISFOLLOWINGSOUND_OFFSET) = false;
            
            // IsChasing = false (not chasing)
            *reinterpret_cast<bool*>(addr + GRANDPA_ISCHASING_OFFSET) = false;
            
            // PlayerClose = false (player not nearby)
            *reinterpret_cast<bool*>(addr + GRANDPA_PLAYERCLOSE_OFFSET) = false;
            
            // PlayerTouchedRay = false (raycast didn't hit player)
            *reinterpret_cast<bool*>(addr + GRANDPA_PLAYERTOUCHEDRAY_OFFSET) = false;
        }
    }
}

