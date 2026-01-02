#include "escapes.h"
#include "../../utils/logger.h"

namespace CubixDLC
{
    namespace Escapes
    {
        // Method pointers cache
        static void* s_EscapeDoor = nullptr;
        static void* s_EscapeCar = nullptr;
        static void* s_EscapeCellar = nullptr;
        static void* s_EscapeRobo = nullptr;
        static bool s_MethodsInitialized = false;

        // Method call type (instance method with no args)
        typedef void (*EscapeMethod_t)(void* instance);

        EscapesModule::EscapesModule()
        {
            IL2CPP::ModuleManager::GetInstance().RegisterModule(this);
            LOG_INFO("Escapes module initialized");
        }

        EscapesModule::~EscapesModule()
        {
        }

        void EscapesModule::OnUpdate()
        {
            // Find Escapes object if needed (for when we need to call methods)
            if (!m_Escapes)
            {
                float currentTime = GetTickCount64() / 1000.0f;
                if (currentTime - m_LastSearchTime > 3.0f)
                {
                    m_LastSearchTime = currentTime;
                    m_Escapes = FindEscapes();
                }
            }
        }

        Unity::CComponent* EscapesModule::FindEscapes()
        {
            Unity::CComponent* result = nullptr;

            __try
            {
                // Try finding Escapes component directly
                Unity::il2cppArray<Unity::CComponent*>* components = Unity::Object::FindObjectsOfType<Unity::CComponent>("Escapes");
                if (components && components->m_uMaxLength > 0)
                {
                    result = components->operator[](0);
                }
                
                // Fallback: Find using GameObject.Find with common names
                if (!result)
                {
                    const char* possibleNames[] = {"Escapes", "GameLogic", "GameManager", "Logic", "Manager", "EscapeManager"};
                    
                    for (int n = 0; n < 6 && !result; n++)
                    {
                        Unity::CGameObject* obj = Unity::GameObject::Find(possibleNames[n]);
                        if (obj)
                        {
                            Unity::CComponent* comp = CubixDLC::IL2CPP::Utils::GetComponent(obj, "Escapes");
                            if (comp)
                            {
                                result = comp;
                                LOG_INFO("Escapes: Found via GameObject.Find(\"%s\")", possibleNames[n]);
                            }
                        }
                    }
                }
                
                if (result)
                {
                    LOG_INFO("Escapes: Found Escapes component at 0x%p", result);
                    
                    // Initialize method pointers if not done
                    if (!s_MethodsInitialized)
                    {
                        s_EscapeDoor = ::IL2CPP::Class::Utils::GetMethodPointer("Escapes", "EscapeDoor", 0);
                        s_EscapeCar = ::IL2CPP::Class::Utils::GetMethodPointer("Escapes", "EscapeCar", 0);
                        s_EscapeCellar = ::IL2CPP::Class::Utils::GetMethodPointer("Escapes", "EscapeCellar", 0);
                        s_EscapeRobo = ::IL2CPP::Class::Utils::GetMethodPointer("Escapes", "EscapeRobo", 0);
                        
                        s_MethodsInitialized = true;
                        LOG_INFO("Escapes: Methods initialized (Door: %p, Car: %p, Cellar: %p, Robo: %p)",
                                s_EscapeDoor, s_EscapeCar, s_EscapeCellar, s_EscapeRobo);
                    }
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                result = nullptr;
            }

            return result;
        }

        bool EscapesModule::TriggerEscape(EscapeType type)
        {
            // Find Escapes if not found
            if (!m_Escapes)
            {
                m_Escapes = FindEscapes();
            }

            if (!m_Escapes)
            {
                LOG_WARNING("Escapes: Escapes component not found (are you in game?)");
                return false;
            }

            if (!s_MethodsInitialized)
            {
                LOG_WARNING("Escapes: Methods not initialized");
                return false;
            }

            void* method = nullptr;
            const char* methodName = nullptr;

            switch (type)
            {
                case EscapeType::Door:
                    method = s_EscapeDoor;
                    methodName = "EscapeDoor";
                    break;
                case EscapeType::Car:
                    method = s_EscapeCar;
                    methodName = "EscapeCar";
                    break;
                case EscapeType::Cellar:
                    method = s_EscapeCellar;
                    methodName = "EscapeCellar";
                    break;
                case EscapeType::Robot:
                    method = s_EscapeRobo;
                    methodName = "EscapeRobo";
                    break;
            }

            if (!method)
            {
                LOG_WARNING("Escapes: Method %s not found", methodName);
                return false;
            }

            __try
            {
                EscapeMethod_t escapeFunc = reinterpret_cast<EscapeMethod_t>(method);
                escapeFunc(m_Escapes);
                LOG_INFO("Escapes: Triggered %s", methodName);
                return true;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                LOG_WARNING("Escapes: Exception while calling %s", methodName);
                m_Escapes = nullptr;  // Reset to refind
                return false;
            }
        }
    }
}

