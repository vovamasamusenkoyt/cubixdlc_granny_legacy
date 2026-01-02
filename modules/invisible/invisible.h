#pragma once

#include "../il2cpp/il2cpp.h"

namespace CubixDLC
{
    namespace Invisible
    {
        class InvisibleModule : public IL2CPP::ModuleBase
        {
        public:
            InvisibleModule();
            ~InvisibleModule();

            void OnEnable() override;
            void OnDisable() override;
            void OnUpdate() override;

            bool IsEnabled() const { return m_Enabled; }
            void SetEnabled(bool enabled) { m_Enabled = enabled; }
            void Toggle() { m_Enabled = !m_Enabled; }

        private:
            bool m_Enabled = false;
            Unity::CComponent* m_Granny = nullptr;
            Unity::CComponent* m_Grandpa = nullptr;
            float m_LastSearchTime = 0.0f;
            
            void FindEnemies();
            void ApplyInvisibility();
            void ApplyToGranny(uintptr_t addr);
            void ApplyToGrandpa(uintptr_t addr);
        };

        inline InvisibleModule* GetModule()
        {
            static InvisibleModule module;
            return &module;
        }
    }
}

