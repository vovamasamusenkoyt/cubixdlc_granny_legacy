#pragma once

#include "../il2cpp/il2cpp.h"
#include <vector>
#include <string>

namespace CubixDLC
{
    namespace ItemSpawner
    {
        // Item category
        enum class ItemCategory
        {
            Tools,
            Keys,
            Weapons,
            Electronics,
            Puzzle,
            Food,
            Vases,
            Christmas,
            Spider,
            Parts,
            All
        };

        // Item definition with offset
        struct ItemDef
        {
            const char* name;
            const char* displayName;
            uintptr_t offset;
            ItemCategory category;
        };

        class ItemSpawnerModule : public IL2CPP::ModuleBase
        {
        public:
            ItemSpawnerModule();
            ~ItemSpawnerModule();

            void OnEnable() override {}
            void OnDisable() override {}
            void OnUpdate() override;

            // Get ItemSpawn component
            Unity::CComponent* GetItemSpawn();
            
            // Spawn item by name
            bool SpawnItem(const char* itemName);
            
            // Spawn item by offset
            bool SpawnItemByOffset(uintptr_t offset);
            
            // Get all item definitions
            static const std::vector<ItemDef>& GetItemDefs();
            
            // Get items by category
            static std::vector<const ItemDef*> GetItemsByCategory(ItemCategory category);
            
            // Category name
            static const char* GetCategoryName(ItemCategory category);

        private:
            Unity::CComponent* m_ItemSpawn = nullptr;
            float m_LastSearchTime = 0.0f;
            
            Unity::CComponent* FindItemSpawn();
            
            // Get player camera for spawn position
            Unity::CTransform* GetPlayerCameraTransform();
        };

        inline ItemSpawnerModule* GetModule()
        {
            static ItemSpawnerModule module;
            return &module;
        }
    }
}

