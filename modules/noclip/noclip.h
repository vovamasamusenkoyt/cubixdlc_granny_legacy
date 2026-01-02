#pragma once

#include "../il2cpp/il2cpp.h"

namespace CubixDLC
{
    namespace NoClip
    {
        class NoClipModule : public IL2CPP::ModuleBase
        {
        public:
            NoClipModule();
            ~NoClipModule();

            void OnEnable() override;
            void OnDisable() override;
            void OnUpdate() override;

            bool IsEnabled() const { return m_Enabled; }
            void Toggle();

            float GetSpeed() const { return m_NoclipSpeed; }
            void SetSpeed(float speed) { m_NoclipSpeed = speed; }

        private:
            float m_NoclipSpeed = 0.2f;
            Unity::CGameObject* m_Player = nullptr;
            Unity::CComponent* m_CharController = nullptr;
            bool m_WasCharControllerEnabled = true;
        };

        inline NoClipModule* GetModule()
        {
            static NoClipModule module;
            return &module;
        }
    }
}

