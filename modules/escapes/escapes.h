#pragma once

#include "../il2cpp/il2cpp.h"

namespace CubixDLC
{
    namespace Escapes
    {
        enum class EscapeType
        {
            Door,
            Car,
            Cellar,
            Robot
        };

        class EscapesModule : public IL2CPP::ModuleBase
        {
        public:
            EscapesModule();
            ~EscapesModule();

            void OnEnable() override {}
            void OnDisable() override {}
            void OnUpdate() override;

            // Trigger escape
            bool TriggerEscape(EscapeType type);

        private:
            Unity::CComponent* m_Escapes = nullptr;
            float m_LastSearchTime = 0.0f;
            
            Unity::CComponent* FindEscapes();
        };

        inline EscapesModule* GetModule()
        {
            static EscapesModule module;
            return &module;
        }
    }
}

