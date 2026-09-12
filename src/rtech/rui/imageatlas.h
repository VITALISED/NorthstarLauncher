#pragma once

#include "rtech/pakasset.h"
#include "rtech/rstdlib.h"
#include "rtech/rui/rui_image_atlas_types.h"
#include "tier0/module.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <vector>
#include <wrl/client.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;
struct ID3D11Texture2D;
struct TextureAsset_s;

// Owns one runtime atlas or the resources used to extend the native menu atlas.
// RuiImageAtlas remains an ABI POD because RTech uses the global atlas array as
// allocator backing.
class CImageAtlas final
{
  public:
    struct Image
    {
        uint32_t m_NameHash = 0;
        uint32_t m_PosX = 0;
        uint32_t m_PosY = 0;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        uint8_t m_Flags = 0;
    };

    struct Placement
    {
        uint16_t x = 0;
        uint16_t y = 0;
        uint16_t width = 0;
        uint16_t height = 0;
        RuiImageAtlasGpuRecord gpuRecord{};
    };

    struct AtlasData
    {
        std::vector<RuiImageAtlasEntry> entries;
        std::vector<RuiImageDimensions> dimensions;
        std::vector<RuiImageAtlasNameRecord> names;
        std::vector<RuiImageAtlasGpuRecord> gpuRecords;
    };

    CImageAtlas() = default;
    ~CImageAtlas();

    CImageAtlas(const CImageAtlas&) = delete;
    CImageAtlas& operator=(const CImageAtlas&) = delete;
    CImageAtlas(CImageAtlas&&) = delete;
    CImageAtlas& operator=(CImageAtlas&&) = delete;

    bool Create(uint16_t width, uint16_t height, void* texture, std::span<const Image> images);
    bool EnsureRegistered();
    void Destroy();

    bool IsResident() const noexcept;
    std::optional<uint8_t> GetAtlasIndex() const noexcept;

    static bool AreRuntimeBindingsReady();
    static uint64_t HashAssetPath(const char* path);
    static uint32_t HashImagePath(const char* path);
    static const RuiImageAssetDescriptor* FindAssetDescriptor(uint32_t nameHash);
    static const RuiImageAssetDescriptor* GetAssetDescriptor(int32_t descriptorIndex) noexcept;
    static RuiImageAtlas* GetAtlas(uint8_t atlasIndex) noexcept;

    static void ConfigureAssetBinding(PakAssetBinding_s* binding);
    static void InitializeEngineBindings(CModule module);
    static void CaptureNativeAtlasGpuRecords(const RuiImageAtlas* atlas, const RuiImageAtlasGpuRecord* records);
    static void RequestNativeMenuImage(const char* imagePath);
    static void RunFrame();

    // Pure metadata operations used by the native extension and its contract test.
    static std::optional<Placement> FindFreePlacement(std::span<const RuiImageAtlasGpuRecord> images, uint16_t atlasWidth, uint16_t atlasHeight,
                                                      uint16_t imageWidth, uint16_t imageHeight);
    static std::optional<AtlasData> ExtendAtlasData(const RuiImageAtlas& atlas, std::span<const RuiImageAtlasGpuRecord> gpuRecords, uint32_t nameHash,
                                                    const RuiImageAtlasEntry& entry, uint16_t imageWidth, uint16_t imageHeight,
                                                    const RuiImageAtlasGpuRecord& gpuRecord);

  private:
    enum class UpdateResult
    {
        Waiting,
        Ready,
        Refused
    };

    using CreateGpuBufferFn = uint32_t (*)(RuiImageAtlas* atlas, const RuiImageAtlasGpuRecord* records);
    using DestroyGpuBufferFn = void (*)(RuiImageAtlas* atlas);
    using FindDescriptorFn = RuiImageAssetDescriptor* (*)(RHashMapU32 * map, uint32_t nameHash);
    using PakStringToGuidFn = uint64_t (*)(const char* path);

    static CImageAtlas& NativeMenuExtension();
    static std::optional<uint8_t> ReserveAtlasSlot();
    static void ReleaseAtlasSlot(uint8_t atlasIndex);
    static RuiImageAssetDescriptor* FindDescriptor(uint32_t nameHash);
    static RuiImageAssetDescriptor* FindOrReserveDescriptor(uint32_t nameHash, uint8_t* reservedNewEntry);
    static bool HasFreeDescriptor();
    static void CommitReservedDescriptor();
    static uint32_t* RemoveDescriptor(uint32_t nameHash);
    static void ReplaceBoundAsset(void* boundAsset, const void* newHeader, const void* previousHeader);
    static void UnloadAsset(void* header);
    static bool RedirectImageDescriptor(uint32_t nameHash, uint8_t atlasIndex, int16_t imageIndex);
    static void InvalidateImageDescriptor(uint32_t nameHash, uint8_t atlasIndex, int16_t imageIndex);
    static uint32_t CreateGpuRecordBuffer(RuiImageAtlas& atlas, std::span<const RuiImageAtlasGpuRecord> records);
    static void DestroyGpuRecordBuffer(uint32_t buffer);
    static uint64_t HashBytes(std::span<const std::byte> bytes);
    static uint32_t ClampBlockFloor(double pixel, uint32_t blockCount);
    static uint32_t ClampBlockCeil(double pixel, uint32_t blockCount);

    bool DescriptorStillOwned(size_t imageIndex) const;
    void UnregisterDescriptors();
    void RequestImage(const char* imagePath);
    void CaptureNativeGpuRecords(const RuiImageAtlas& atlas, std::span<const RuiImageAtlasGpuRecord> records);
    void OnAtlasReplacing(RuiImageAtlas* atlas);
    bool IsExpectedNativeAtlas(const RuiImageAtlas& atlas) const;
    bool OwnsExtendedNativeAtlas(const RuiImageAtlas* atlas) const;
    bool DetachNativeExtension();
    void RetireNativeExtensionResources();
    bool IsTextureOnDevice(ID3D11Texture2D* texture, ID3D11Device* device) const;
    UpdateResult AcquireSourceTexture(ID3D11Device* device);
    UpdateResult UpdateNativeMenuAtlas(ID3D11Device* device, ID3D11DeviceContext* context);
    void UpdateNativeMenuAtlasOnRenderThread();
    void ScheduleNativeMenuUpdate();

    static constexpr uint32_t kCompressionBlock = 4;
    static constexpr char kNativeMenuImagePath[] = "loadscreens/mp_nexus_lobby";
    static constexpr char kNativeMenuBorderPath[] = "rui/borders/menu_border_button";
    static constexpr uint64_t kSourceTextureGuid = 0x917045E5418C5B49;
    static constexpr uint64_t kNativeMenuTextureGuid = 0xCAA3F7556579B304;
    static constexpr uint16_t kNativeMenuWidth = 8192;
    static constexpr uint16_t kNativeMenuHeight = 4256;
    static constexpr uint16_t kNativeMenuImageCount = 578;
    static constexpr uint16_t kNativeMenuNineSliceCount = 15;
    static constexpr uint16_t kSourceWidth = 500;
    static constexpr uint16_t kSourceHeight = 288;

    static bool s_LoaderConfigured;
    static CreateGpuBufferFn s_CreateGpuBuffer;
    static DestroyGpuBufferFn s_DestroyGpuBuffer;
    static PakAssetUnloadFn_t s_UnloadAsset;
    static FindDescriptorFn s_FindDescriptor;
    static RHashMapU32FindOrReserveUnlockedFn s_FindOrReserveDescriptor;
    static RHashMapU32RemoveExistingFn s_RemoveDescriptor;
    static PakStringToGuidFn s_PakStringToGuidAligned;
    static PakStringToGuidFn s_PakStringToGuidUnaligned;
    static RHashMapU32* s_DescriptorMap;
    static RuiImageAtlas s_Atlases[RUI_IMAGE_ATLAS_CAPACITY];
    static std::mutex* const s_AtlasSlotMutex;
    static std::array<bool, RUI_IMAGE_ATLAS_CAPACITY>* const s_OwnedAtlasSlots;

    std::vector<RuiImageAtlasEntry> m_AtlasEntries;
    std::vector<RuiImageDimensions> m_ImageDimensions;
    std::vector<RuiImageAtlasGpuRecord> m_GpuRecords;
    std::vector<RuiImageAtlasNameRecord> m_NameRecords;
    std::vector<RuiImageAssetDescriptor*> m_Descriptors;
    std::optional<uint8_t> m_AtlasIndex;

    std::mutex m_NativeMenuMutex;
    std::atomic<bool> m_NativeMenuRequested = false;
    std::atomic<bool> m_NativeMenuDirty = false;
    std::atomic<bool> m_NativeMenuQueued = false;
    std::atomic<ID3D11Device*> m_NativeMenuDevice = nullptr;
    bool m_NativeMenuApplied = false;
    bool m_SourceEntryKnown = false;
    uint8_t m_NativeMenuAtlasIndex = 0;
    uint32_t m_NativeMenuNameHash = 0;
    RuiImageAtlas m_OriginalNativeMenuAtlas{};
    RuiImageAtlasEntry m_SourceEntry{};
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_SourceTexture;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_NativeMenuTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_NativeMenuView;
    std::unique_ptr<TextureAsset_s> m_NativeMenuTextureHeader;
    AtlasData m_NativeMenuData;
    std::vector<RuiImageAtlasGpuRecord> m_CapturedNativeGpuRecords;
    uint32_t m_NativeMenuGpuBuffer = UINT_MAX;
};
