#include "itemspawner.h"
#include "../../utils/logger.h"

namespace CubixDLC
{
    namespace ItemSpawner
    {
        // Static item definitions - use item names for Inventory.PickupItem
        static std::vector<ItemDef> s_ItemDefs = {
            // Weapons
            {"Crossbow", "Crossbow", 0, ItemCategory::Weapons},
            {"Shotgun", "Shotgun", 0, ItemCategory::Weapons},
            {"PepperSpray", "Pepper Spray", 0, ItemCategory::Weapons},
            {"Syringe", "Syringe", 0, ItemCategory::Weapons},
            {"Baton", "Stun Baton", 0, ItemCategory::Weapons},
            
            // Tools
            {"Hammer", "Hammer", 0, ItemCategory::Tools},
            {"Wrench", "Wrench", 0, ItemCategory::Tools},
            {"Plier", "Pliers", 0, ItemCategory::Tools},
            {"Cutter", "Cutting Pliers", 0, ItemCategory::Tools},
            {"Screwdriver", "Screwdriver", 0, ItemCategory::Tools},
            
            // Keys
            {"PadlockKey", "Padlock Key", 0, ItemCategory::Keys},
            {"SafeKey", "Safe Key", 0, ItemCategory::Keys},
            {"CarKey", "Car Key", 0, ItemCategory::Keys},
            {"MasterKey", "Master Key", 0, ItemCategory::Keys},
            {"SpecialKey", "Special Key", 0, ItemCategory::Keys},
            {"WeaponKey", "Weapon Key", 0, ItemCategory::Keys},
            {"WinchKey", "Winch Key", 0, ItemCategory::Keys},
            {"SpiderKey", "Spider Key", 0, ItemCategory::Spider},
            {"PlayhouseKey", "Playhouse Key", 0, ItemCategory::Keys},
            
            // Electronics
            {"Battery", "Battery", 0, ItemCategory::Electronics},
            {"CarBattery", "Car Battery", 0, ItemCategory::Electronics},
            {"Remote", "Remote", 0, ItemCategory::Electronics},
            {"SparkPlug", "Spark Plug", 0, ItemCategory::Electronics},
            {"Fuse", "Fuse", 0, ItemCategory::Electronics},
            
            // Puzzle Items
            {"Melon", "Watermelon", 0, ItemCategory::Puzzle},
            {"Book", "Book", 0, ItemCategory::Puzzle},
            {"Winch", "Winch Handle", 0, ItemCategory::Puzzle},
            {"Wheel", "Wheel", 0, ItemCategory::Puzzle},
            {"Plank", "Plank", 0, ItemCategory::Puzzle},
            {"Teddy", "Teddy Bear", 0, ItemCategory::Puzzle},
            {"Gear1", "Gear 1", 0, ItemCategory::Puzzle},
            {"Gear2", "Gear 2", 0, ItemCategory::Puzzle},
            
            // Food/Other
            {"Meat", "Meat", 0, ItemCategory::Food},
            {"Gas", "Gas Can", 0, ItemCategory::Food},
            
            // Vases
            {"Vase", "Vase", 0, ItemCategory::Vases},
        };
        ItemSpawnerModule::ItemSpawnerModule()
        {
            IL2CPP::ModuleManager::GetInstance().RegisterModule(this);
            LOG_INFO("ItemSpawner module initialized");
        }

        ItemSpawnerModule::~ItemSpawnerModule()
        {
        }

        void ItemSpawnerModule::OnUpdate()
        {
            // Periodically search for ItemSpawn if not found
            if (!m_ItemSpawn)
            {
                float currentTime = GetTickCount64() / 1000.0f;
                if (currentTime - m_LastSearchTime > 5.0f)
                {
                    m_LastSearchTime = currentTime;
                    m_ItemSpawn = FindItemSpawn();
                }
            }
        }

        // Cached method pointers
        static void* s_PickupItemMethod = nullptr;
        static bool s_MethodsSearched = false;

        Unity::CComponent* ItemSpawnerModule::FindItemSpawn()
        {
            Unity::CComponent* result = nullptr;

            __try
            {
                // Find Inventory component instead of ItemSpawn
                // Inventory has PickupItem(string) method which is easier to use
                Unity::il2cppArray<Unity::CComponent*>* components = Unity::Object::FindObjectsOfType<Unity::CComponent>("Inventory");
                if (components && components->m_uMaxLength > 0)
                {
                    result = components->operator[](0);
                    LOG_INFO("ItemSpawner: Found Inventory at 0x%p", result);
                    
                    // Cache PickupItem method
                    if (!s_MethodsSearched)
                    {
                        s_PickupItemMethod = ::IL2CPP::Class::Utils::GetMethodPointer("Inventory", "PickupItem", 1);
                        s_MethodsSearched = true;
                        
                        if (s_PickupItemMethod)
                        {
                            LOG_INFO("ItemSpawner: Found PickupItem method at 0x%p", s_PickupItemMethod);
                        }
                        else
                        {
                            LOG_WARNING("ItemSpawner: PickupItem method not found");
                        }
                    }
                    
                    return result;
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                LOG_WARNING("ItemSpawner: Exception while searching for Inventory");
                result = nullptr;
            }

            return result;
        }

        Unity::CComponent* ItemSpawnerModule::GetItemSpawn()
        {
            if (!m_ItemSpawn)
            {
                m_ItemSpawn = FindItemSpawn();
            }
            return m_ItemSpawn;
        }

        // Get forward vector from transform rotation
        static Unity::Vector3 GetForwardFromTransform(Unity::CTransform* transform)
        {
            Unity::Quaternion rot = transform->GetRotation();
            Unity::Vector3 forward;
            forward.x = 2.0f * (rot.x * rot.z + rot.w * rot.y);
            forward.y = 2.0f * (rot.y * rot.z - rot.w * rot.x);
            forward.z = 1.0f - 2.0f * (rot.x * rot.x + rot.y * rot.y);
            return forward;
        }

        Unity::CTransform* ItemSpawnerModule::GetPlayerCameraTransform()
        {
            Unity::CTransform* result = nullptr;
            
            __try
            {
                Unity::CCamera* mainCamera = Unity::Camera::GetMain();
                if (mainCamera)
                {
                    // Camera inherits from Component, get transform through Component
                    result = reinterpret_cast<Unity::CComponent*>(mainCamera)->GetTransform();
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                result = nullptr;
            }
            
            return result;
        }

        bool ItemSpawnerModule::SpawnItem(const char* itemName)
        {
            // Use PickupItem method from Inventory
            Unity::CComponent* inventory = GetItemSpawn();  // Now returns Inventory
            if (!inventory)
            {
                LOG_WARNING("ItemSpawner: Inventory not found");
                return false;
            }

            if (!s_PickupItemMethod)
            {
                LOG_WARNING("ItemSpawner: PickupItem method not found");
                return false;
            }

            __try
            {
                // Create IL2CPP string for item name
                Unity::System_String* itemNameStr = ::IL2CPP::String::New(itemName);
                if (!itemNameStr)
                {
                    LOG_WARNING("ItemSpawner: Failed to create item name string");
                    return false;
                }

                // Call Inventory.PickupItem(string itemName)
                typedef void (*PickupItem_t)(Unity::CComponent*, Unity::System_String*);
                reinterpret_cast<PickupItem_t>(s_PickupItemMethod)(inventory, itemNameStr);
                
                LOG_INFO("ItemSpawner: Picked up item '%s'", itemName);
                return true;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                LOG_WARNING("ItemSpawner: Exception while picking up item");
                return false;
            }
        }

        bool ItemSpawnerModule::SpawnItemByOffset(uintptr_t offset)
        {
            // Find item by offset and spawn it by name
            for (const auto& item : s_ItemDefs)
            {
                if (item.offset == offset || offset == 0)
                {
                    // For new system, offset is not used - use item name directly
                    // This is called from UI with offset=0, so we'll handle it in the UI
                    return SpawnItem(item.name);
                }
            }
            LOG_WARNING("ItemSpawner: No item found for offset 0x%X", (unsigned int)offset);
            return false;
        }

        const std::vector<ItemDef>& ItemSpawnerModule::GetItemDefs()
        {
            return s_ItemDefs;
        }

        std::vector<const ItemDef*> ItemSpawnerModule::GetItemsByCategory(ItemCategory category)
        {
            std::vector<const ItemDef*> result;
            for (const auto& item : s_ItemDefs)
            {
                if (category == ItemCategory::All || item.category == category)
                {
                    result.push_back(&item);
                }
            }
            return result;
        }

        const char* ItemSpawnerModule::GetCategoryName(ItemCategory category)
        {
            switch (category)
            {
                case ItemCategory::Tools: return "Tools";
                case ItemCategory::Keys: return "Keys";
                case ItemCategory::Weapons: return "Weapons";
                case ItemCategory::Electronics: return "Electronics";
                case ItemCategory::Puzzle: return "Puzzle";
                case ItemCategory::Food: return "Food";
                case ItemCategory::Vases: return "Vases";
                case ItemCategory::Christmas: return "Christmas";
                case ItemCategory::Spider: return "Spider";
                case ItemCategory::Parts: return "Parts";
                case ItemCategory::All: return "All";
                default: return "Unknown";
            }
        }
    }
}

