#include "il2cpp.h"

namespace CubixDLC
{
    namespace IL2CPP
    {
        namespace Internal
        {
            void OnUpdateCallback()
            {
                ModuleManager::GetInstance().UpdateAll();
            }

            void OnLateUpdateCallback()
            {
                ModuleManager::GetInstance().LateUpdateAll();
            }
        }
    }
}

