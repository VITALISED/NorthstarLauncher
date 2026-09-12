#include "rtech/rui/imageatlas.h"
#include "materialsystem/dx11_device.h"
#include "rendersystem/schema/texture.g.h"
#include "rtech/pakfilesystem.h"
#include "rtech/paktools.h"
#include "rtech/rstdlib.h"
#include "rtech/rui/render.h"
#include "util/utils.h"
#include <atomic>
#include <utility>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>

DECLARE_MODULE(RuiImageAtlasHooks)

bool CImageAtlas::s_LoaderConfigured = false;
CImageAtlas::CreateGpuBufferFn CImageAtlas::s_CreateGpuBuffer = nullptr;
CImageAtlas::DestroyGpuBufferFn CImageAtlas::s_DestroyGpuBuffer = nullptr;
PakAssetUnloadFn_t CImageAtlas::s_UnloadAsset = nullptr;
CImageAtlas::FindDescriptorFn CImageAtlas::s_FindDescriptor = nullptr;
RHashMapU32FindOrReserveUnlockedFn CImageAtlas::s_FindOrReserveDescriptor = nullptr;
RHashMapU32RemoveExistingFn CImageAtlas::s_RemoveDescriptor = nullptr;
CImageAtlas::PakStringToGuidFn CImageAtlas::s_PakStringToGuidAligned = nullptr;
CImageAtlas::PakStringToGuidFn CImageAtlas::s_PakStringToGuidUnaligned = nullptr;
RHashMapU32* CImageAtlas::s_DescriptorMap = nullptr;
RuiImageAtlas CImageAtlas::s_Atlases[RUI_IMAGE_ATLAS_CAPACITY]{};
std::mutex* const CImageAtlas::s_AtlasSlotMutex = new std::mutex;
std::array<bool, RUI_IMAGE_ATLAS_CAPACITY>* const CImageAtlas::s_OwnedAtlasSlots = new std::array<bool, RUI_IMAGE_ATLAS_CAPACITY>{};

std::optional<uint8_t> CImageAtlas::ReserveAtlasSlot()
{
    std::lock_guard<std::mutex> lock(*s_AtlasSlotMutex);
    for (int atlasIndex = RUI_DYNAMIC_IMAGE_ATLAS_LAST; atlasIndex >= RUI_DYNAMIC_IMAGE_ATLAS_FIRST; --atlasIndex)
    {
        if ((*s_OwnedAtlasSlots)[atlasIndex])
            continue;

        (*s_OwnedAtlasSlots)[atlasIndex] = true;
        return static_cast<uint8_t>(atlasIndex);
    }

    return std::nullopt;
}

void CImageAtlas::ReleaseAtlasSlot(uint8_t atlasIndex)
{
    if (atlasIndex < RUI_DYNAMIC_IMAGE_ATLAS_FIRST || atlasIndex > RUI_DYNAMIC_IMAGE_ATLAS_LAST)
        return;

    std::lock_guard<std::mutex> lock(*s_AtlasSlotMutex);
    (*s_OwnedAtlasSlots)[atlasIndex] = false;
}

RuiImageAssetDescriptor* CImageAtlas::FindDescriptor(uint32_t nameHash)
{
    if (!s_DescriptorMap || !s_FindDescriptor)
        return nullptr;

    return s_FindDescriptor(s_DescriptorMap, nameHash);
}

RuiImageAssetDescriptor* CImageAtlas::FindOrReserveDescriptor(uint32_t nameHash, uint8_t* reservedNewEntry)
{
    if (!s_DescriptorMap || !s_FindOrReserveDescriptor)
        return nullptr;

    return static_cast<RuiImageAssetDescriptor*>(s_FindOrReserveDescriptor(s_DescriptorMap, nameHash, reservedNewEntry));
}

bool CImageAtlas::HasFreeDescriptor()
{
    return s_DescriptorMap &&
           (s_DescriptorMap->freeListHead != s_DescriptorMap->nextUnusedIndex || s_DescriptorMap->nextUnusedIndex < s_DescriptorMap->bucketPairCount);
}

void CImageAtlas::CommitReservedDescriptor()
{
    s_DescriptorMap->bucketEntryIndices[s_DescriptorMap->pendingBucketIndex] = static_cast<int32_t>(s_DescriptorMap->pendingEntryIndex);
    ++s_DescriptorMap->liveEntryCount;
}

uint32_t* CImageAtlas::RemoveDescriptor(uint32_t nameHash)
{
    if (!s_DescriptorMap || !s_RemoveDescriptor)
        return nullptr;

    return s_RemoveDescriptor(s_DescriptorMap, nameHash);
}

void CImageAtlas::ReplaceBoundAsset(void* boundAsset, const void* newHeader, const void* previousHeader)
{
    auto* destination = static_cast<RuiImageAtlas*>(boundAsset);
    const auto* source = static_cast<const RuiImageAtlas*>(newHeader);
    const auto* previous = static_cast<const RuiImageAtlas*>(previousHeader);
    if (!destination)
        return;

    const uintptr_t destinationOffset = reinterpret_cast<uintptr_t>(destination) - reinterpret_cast<uintptr_t>(s_Atlases);
    if (destinationOffset >= sizeof(s_Atlases) || destinationOffset % sizeof(RuiImageAtlas) != 0)
    {
        if (source)
            *destination = *source;
        return;
    }

    NativeMenuExtension().OnAtlasReplacing(destination);

    const uint8_t atlasIndex = static_cast<uint8_t>(destinationOffset / sizeof(RuiImageAtlas));
    if (!s_DescriptorMap)
    {
        if (source)
            *destination = *source;
        return;
    }

    bool descriptorTableFull = false;
    AcquireSRWLockExclusive(&s_DescriptorMap->lock);
    if (previous && previous->imageNameRecords)
    {
        for (uint16_t imageIndex = 0; imageIndex < previous->imageCount; ++imageIndex)
        {
            const uint32_t nameHash = previous->imageNameRecords[imageIndex].nameHash;
            const RuiImageAssetDescriptor* descriptor = FindDescriptor(nameHash);
            if (descriptor && descriptor->nameHash == nameHash && descriptor->imageIndex == static_cast<int16_t>(imageIndex) &&
                descriptor->atlasIndex == atlasIndex)
            {
                RemoveDescriptor(nameHash);
            }
        }
    }

    if (source)
    {
        *destination = *source;
        if (source->imageNameRecords)
        {
            for (uint16_t imageIndex = 0; imageIndex < source->imageCount; ++imageIndex)
            {
                const RuiImageAtlasNameRecord& nameRecord = source->imageNameRecords[imageIndex];
                uint8_t reservedNewEntry = 0;
                RuiImageAssetDescriptor* descriptor = FindDescriptor(nameRecord.nameHash);
                if (!descriptor)
                {
                    if (!HasFreeDescriptor())
                    {
                        descriptorTableFull = true;
                        continue;
                    }

                    descriptor = FindOrReserveDescriptor(nameRecord.nameHash, &reservedNewEntry);
                }
                if (!descriptor)
                    continue;

                descriptor->nameHash = nameRecord.nameHash;
                descriptor->imageIndex = static_cast<int16_t>(imageIndex);
                descriptor->atlasIndex = atlasIndex;
                descriptor->flags = static_cast<uint8_t>(nameRecord.flags);
                if (reservedNewEntry)
                    CommitReservedDescriptor();
            }
        }
    }
    ReleaseSRWLockExclusive(&s_DescriptorMap->lock);
    if (descriptorTableFull)
    {
        spdlog::error("RUI image descriptor table is full while publishing atlas {}", atlasIndex);
    }
}

void CImageAtlas::UnloadAsset(void* header)
{
    NativeMenuExtension().OnAtlasReplacing(static_cast<RuiImageAtlas*>(header));
    if (s_UnloadAsset)
        s_UnloadAsset(header);
}

void CImageAtlas::ConfigureAssetBinding(PakAssetBinding_s* binding)
{
    if (!binding || std::memcmp(binding->type, "uimg", sizeof(binding->type)) != 0)
        return;

    binding->assetCapacity = RUI_DYNAMIC_IMAGE_ATLAS_FIRST;
    binding->assetStorage = s_Atlases;
    binding->replaceAssetFunc = ReplaceBoundAsset;
    if (binding->unloadAssetFunc != UnloadAsset)
    {
        s_UnloadAsset = binding->unloadAssetFunc;
        binding->unloadAssetFunc = UnloadAsset;
    }
    s_LoaderConfigured = true;
}

DECLARE_HOOK(Pak_RegisterAssetBindingType, rtech_game.DLL + 0x7BE0,
             [](auto& hook, PakAssetBinding_s* binding, JobPriority_e priority, uint32_t affinity) -> JobTypeID_t
{
    CImageAtlas::ConfigureAssetBinding(binding);
    return hook.Original(binding, priority, affinity);
})

DECLARE_HOOK(RuiImageAtlas_CreateGpuBuffer, engine.dll + 0xFBF60,
             [](auto& hook, RuiImageAtlas* atlas, const RuiImageAtlasGpuRecord* records) -> uint32_t
{
    CImageAtlas::CaptureNativeAtlasGpuRecords(atlas, records);
    return hook.Original(atlas, records);
})

bool CImageAtlas::AreRuntimeBindingsReady()
{
    return GetModuleHandleW(L"engine.dll") && s_LoaderConfigured && s_DescriptorMap && s_CreateGpuBuffer && s_DestroyGpuBuffer && s_FindDescriptor &&
           s_FindOrReserveDescriptor && s_RemoveDescriptor;
}

uint64_t CImageAtlas::HashAssetPath(const char* path)
{
    if (!path)
        return 0;

    PakStringToGuidFn hashFunction = (reinterpret_cast<uintptr_t>(path) & 3) ? s_PakStringToGuidUnaligned : s_PakStringToGuidAligned;
    return hashFunction ? hashFunction(path) : Pak_StringToGuid(path);
}

uint32_t CImageAtlas::HashImagePath(const char* path)
{
    const uint64_t guid = HashAssetPath(path);
    return static_cast<uint32_t>(guid) ^ static_cast<uint32_t>(guid >> 32);
}

const RuiImageAssetDescriptor* CImageAtlas::FindAssetDescriptor(uint32_t nameHash)
{
    return FindDescriptor(nameHash);
}

const RuiImageAssetDescriptor* CImageAtlas::GetAssetDescriptor(int32_t descriptorIndex) noexcept
{
    if (!s_DescriptorMap || !s_DescriptorMap->entryStorage || descriptorIndex < 0 ||
        static_cast<uint32_t>(descriptorIndex) >= s_DescriptorMap->bucketPairCount)
    {
        return nullptr;
    }

    return &static_cast<const RuiImageAssetDescriptor*>(s_DescriptorMap->entryStorage)[descriptorIndex];
}

RuiImageAtlas* CImageAtlas::GetAtlas(uint8_t atlasIndex) noexcept
{
    return atlasIndex < RUI_IMAGE_ATLAS_CAPACITY ? &s_Atlases[atlasIndex] : nullptr;
}

CImageAtlas::~CImageAtlas()
{
    Destroy();
}

bool CImageAtlas::DescriptorStillOwned(size_t imageIndex) const
{
    if (!m_AtlasIndex || imageIndex >= m_Descriptors.size() || imageIndex >= m_NameRecords.size())
        return false;

    const RuiImageAssetDescriptor* descriptor = m_Descriptors[imageIndex];
    const uint32_t nameHash = m_NameRecords[imageIndex].nameHash;
    return descriptor && FindDescriptor(nameHash) == descriptor && descriptor->nameHash == nameHash &&
           descriptor->imageIndex == static_cast<int16_t>(imageIndex) && descriptor->atlasIndex == *m_AtlasIndex;
}

void CImageAtlas::UnregisterDescriptors()
{
    if (!AreRuntimeBindingsReady() || !m_AtlasIndex)
    {
        m_Descriptors.clear();
        return;
    }

    AcquireSRWLockExclusive(&s_DescriptorMap->lock);
    for (size_t imageIndex = 0; imageIndex < m_Descriptors.size(); ++imageIndex)
    {
        if (DescriptorStillOwned(imageIndex))
            RemoveDescriptor(m_NameRecords[imageIndex].nameHash);
    }
    ReleaseSRWLockExclusive(&s_DescriptorMap->lock);
    m_Descriptors.clear();
}

bool CImageAtlas::EnsureRegistered()
{
    if (!AreRuntimeBindingsReady() || !m_AtlasIndex)
        return false;

    std::vector<uint32_t> failedHashes;
    failedHashes.reserve(m_NameRecords.size());
    bool descriptorTableFull = false;
    bool ownsDescriptor = false;

    AcquireSRWLockExclusive(&s_DescriptorMap->lock);
    for (size_t imageIndex = 0; imageIndex < m_Descriptors.size(); ++imageIndex)
    {
        if (m_Descriptors[imageIndex] && !DescriptorStillOwned(imageIndex))
            m_Descriptors[imageIndex] = nullptr;
        if (m_Descriptors[imageIndex])
            ownsDescriptor = true;
    }

    for (size_t imageIndex = 0; imageIndex < m_NameRecords.size(); ++imageIndex)
    {
        const RuiImageAtlasNameRecord& nameRecord = m_NameRecords[imageIndex];
        if (FindDescriptor(nameRecord.nameHash))
            continue;

        if (!HasFreeDescriptor())
        {
            descriptorTableFull = true;
            break;
        }

        uint8_t reservedNewEntry = 0;
        RuiImageAssetDescriptor* descriptor = FindOrReserveDescriptor(nameRecord.nameHash, &reservedNewEntry);
        if (!descriptor || !reservedNewEntry)
        {
            failedHashes.push_back(nameRecord.nameHash);
            continue;
        }

        descriptor->nameHash = nameRecord.nameHash;
        descriptor->imageIndex = static_cast<int16_t>(imageIndex);
        descriptor->atlasIndex = *m_AtlasIndex;
        descriptor->flags = static_cast<uint8_t>(nameRecord.flags);
        CommitReservedDescriptor();
        m_Descriptors[imageIndex] = descriptor;

        if (FindDescriptor(nameRecord.nameHash) != descriptor)
        {
            failedHashes.push_back(nameRecord.nameHash);
            continue;
        }

        ownsDescriptor = true;
    }
    ReleaseSRWLockExclusive(&s_DescriptorMap->lock);

    if (descriptorTableFull)
    {
        spdlog::error("RUI image descriptor table is full while publishing atlas {}", *m_AtlasIndex);
    }
    for (const uint32_t nameHash : failedHashes)
        spdlog::error("RUI image descriptor insertion failed for hash 0x{:08X}", nameHash);

    return ownsDescriptor;
}

bool CImageAtlas::Create(uint16_t width, uint16_t height, void* texture, std::span<const Image> images)
{
    if (m_AtlasIndex || !AreRuntimeBindingsReady() || !texture || width == 0 || height == 0 || images.empty() || images.size() > INT16_MAX)
    {
        return false;
    }

    for (const Image& image : images)
    {
        const uint64_t maxX = static_cast<uint64_t>(image.m_PosX) + image.m_Width;
        const uint64_t maxY = static_cast<uint64_t>(image.m_PosY) + image.m_Height;
        if (image.m_Width == 0 || image.m_Height == 0 || maxX > width || maxY > height)
        {
            return false;
        }
    }

    m_AtlasEntries.resize(images.size());
    m_ImageDimensions.resize(images.size());
    m_GpuRecords.resize(images.size());
    m_NameRecords.resize(images.size());
    m_Descriptors.assign(images.size(), nullptr);

    const float inverseWidth = 1.0f / static_cast<float>(width);
    const float inverseHeight = 1.0f / static_cast<float>(height);
    for (size_t imageIndex = 0; imageIndex < images.size(); ++imageIndex)
    {
        const Image& image = images[imageIndex];
        RuiImageAtlasEntry& atlasEntry = m_AtlasEntries[imageIndex];
        atlasEntry.pixelBounds[0] = -0.0f;
        atlasEntry.pixelBounds[1] = -0.0f;
        atlasEntry.pixelBounds[2] = 1.0f;
        atlasEntry.pixelBounds[3] = 1.0f;
        atlasEntry.uvBase[0] = 0.0f;
        atlasEntry.uvBase[1] = 0.0f;
        atlasEntry.uvScale[0] = 1.0f;
        atlasEntry.uvScale[1] = 1.0f;

        m_ImageDimensions[imageIndex] = {static_cast<uint16_t>(image.m_Width), static_cast<uint16_t>(image.m_Height)};

        RuiImageAtlasGpuRecord& gpuRecord = m_GpuRecords[imageIndex];
        gpuRecord.uvMin[0] = static_cast<float>(image.m_PosX) * inverseWidth;
        gpuRecord.uvMin[1] = static_cast<float>(image.m_PosY) * inverseHeight;
        gpuRecord.uvSize[0] = static_cast<float>(image.m_Width) * inverseWidth;
        gpuRecord.uvSize[1] = static_cast<float>(image.m_Height) * inverseHeight;

        RuiImageAtlasNameRecord& nameRecord = m_NameRecords[imageIndex];
        nameRecord.nameHash = image.m_NameHash;
        nameRecord.flags = image.m_Flags;
        nameRecord.nameOffset = 0;
    }

    const std::optional<uint8_t> atlasIndex = ReserveAtlasSlot();
    if (!atlasIndex)
    {
        m_AtlasEntries.clear();
        m_ImageDimensions.clear();
        m_GpuRecords.clear();
        m_NameRecords.clear();
        m_Descriptors.clear();
        return false;
    }

    RuiImageAtlas& atlas = s_Atlases[*atlasIndex];
    atlas = {};
    atlas.inverseWidth = inverseWidth;
    atlas.inverseHeight = inverseHeight;
    atlas.width = width;
    atlas.height = height;
    atlas.imageCount = static_cast<uint16_t>(images.size());
    atlas.nineSliceImageCount = 0;
    atlas.images = m_AtlasEntries.data();
    atlas.imageDimensions = m_ImageDimensions.data();
    atlas.nineSliceData = nullptr;
    atlas.imageNameRecords = m_NameRecords.data();
    atlas.imageNames = nullptr;
    atlas.texture = texture;
    atlas.gpuRecordBuffer = UINT_MAX;

    const uint32_t gpuRecordBuffer = s_CreateGpuBuffer(&atlas, m_GpuRecords.data());
    if (gpuRecordBuffer == UINT_MAX || atlas.gpuRecordBuffer != gpuRecordBuffer)
    {
        atlas = {};
        m_AtlasEntries.clear();
        m_ImageDimensions.clear();
        m_GpuRecords.clear();
        m_NameRecords.clear();
        m_Descriptors.clear();
        ReleaseAtlasSlot(*atlasIndex);
        return false;
    }

    m_AtlasIndex = *atlasIndex;
    if (!EnsureRegistered())
    {
        Destroy();
        return false;
    }

    return true;
}

void CImageAtlas::Destroy()
{
    if (!m_AtlasIndex)
        return;

    const uint8_t atlasIndex = *m_AtlasIndex;
    UnregisterDescriptors();
    RuiImageAtlas& atlas = s_Atlases[atlasIndex];
    if (atlas.gpuRecordBuffer != UINT_MAX && s_DestroyGpuBuffer && GetModuleHandleW(L"engine.dll"))
    {
        s_DestroyGpuBuffer(&atlas);
    }
    atlas = {};

    m_AtlasIndex.reset();
    m_AtlasEntries.clear();
    m_ImageDimensions.clear();
    m_GpuRecords.clear();
    m_NameRecords.clear();
    ReleaseAtlasSlot(atlasIndex);
}

bool CImageAtlas::IsResident() const noexcept
{
    return m_AtlasIndex.has_value();
}

std::optional<uint8_t> CImageAtlas::GetAtlasIndex() const noexcept
{
    return m_AtlasIndex;
}

uint32_t CImageAtlas::CreateGpuRecordBuffer(RuiImageAtlas& atlas, std::span<const RuiImageAtlasGpuRecord> records)
{
    if (!s_CreateGpuBuffer || records.size() != atlas.imageCount)
        return UINT_MAX;
    atlas.gpuRecordBuffer = UINT_MAX;
    return s_CreateGpuBuffer(&atlas, records.data());
}

void CImageAtlas::DestroyGpuRecordBuffer(uint32_t buffer)
{
    if (!s_DestroyGpuBuffer || buffer == UINT_MAX)
        return;
    RuiImageAtlas owner{};
    owner.gpuRecordBuffer = buffer;
    s_DestroyGpuBuffer(&owner);
}

bool CImageAtlas::RedirectImageDescriptor(uint32_t nameHash, uint8_t atlasIndex, int16_t imageIndex)
{
    if (!s_DescriptorMap || nameHash == 0 || imageIndex < 0 || atlasIndex >= RUI_IMAGE_ATLAS_CAPACITY ||
        static_cast<uint16_t>(imageIndex) >= s_Atlases[atlasIndex].imageCount)
        return false;
    AcquireSRWLockExclusive(&s_DescriptorMap->lock);
    uint8_t reservedNewEntry = 0;
    RuiImageAssetDescriptor* descriptor = FindDescriptor(nameHash);
    if (!descriptor && HasFreeDescriptor())
        descriptor = FindOrReserveDescriptor(nameHash, &reservedNewEntry);
    if (descriptor)
    {
        descriptor->nameHash = nameHash;
        descriptor->imageIndex = imageIndex;
        descriptor->atlasIndex = atlasIndex;
        descriptor->flags = 0;
        if (reservedNewEntry)
            CommitReservedDescriptor();
    }
    ReleaseSRWLockExclusive(&s_DescriptorMap->lock);
    return descriptor != nullptr;
}

void CImageAtlas::InvalidateImageDescriptor(uint32_t nameHash, uint8_t atlasIndex, int16_t imageIndex)
{
    if (!s_DescriptorMap)
        return;
    AcquireSRWLockExclusive(&s_DescriptorMap->lock);
    RuiImageAssetDescriptor* const descriptor = FindDescriptor(nameHash);
    if (descriptor && descriptor->nameHash == nameHash && descriptor->atlasIndex == atlasIndex && descriptor->imageIndex == imageIndex)
    {
        descriptor->imageIndex = -1;
        descriptor->atlasIndex = UINT8_MAX;
        descriptor->flags = 0;
    }
    ReleaseSRWLockExclusive(&s_DescriptorMap->lock);
}

void CImageAtlas::InitializeEngineBindings(CModule module)
{
    s_DescriptorMap = module.Offset(0x12A4E508).RCast<RHashMapU32*>();
    s_PakStringToGuidAligned = module.Offset(0x4305D0).RCast<PakStringToGuidFn>();
    s_PakStringToGuidUnaligned = module.Offset(0x4305E0).RCast<PakStringToGuidFn>();
    s_CreateGpuBuffer = HookSys::GetOriginalFunction<CreateGpuBufferFn>(HookSys::FindHook("RuiImageAtlas_CreateGpuBuffer"));
    s_DestroyGpuBuffer = module.Offset(0xFC4F0).RCast<DestroyGpuBufferFn>();
    s_FindDescriptor = module.Offset(0xF3C60).RCast<FindDescriptorFn>();
    s_FindOrReserveDescriptor = module.Offset(0xF3BB0).RCast<RHashMapU32FindOrReserveUnlockedFn>();
    s_RemoveDescriptor = module.Offset(0xF3E30).RCast<RHashMapU32RemoveExistingFn>();
}

ON_DLL_LOAD("engine.dll", RuiImageAtlasEngine, [](CModule module)
{
    RuiImageAtlasHooks.DispatchForModule("engine.dll");
    CImageAtlas::InitializeEngineBindings(module);
})

ON_DLL_LOAD("rtech_game.DLL", RuiImageAtlasRtech, [](CModule module)
{
    (void)module;
    RuiImageAtlasHooks.DispatchForModule("rtech_game.DLL");
})

uint32_t CImageAtlas::ClampBlockFloor(double pixel, uint32_t blockCount)
{
    if (pixel <= 0.0)
        return 0;
    return std::min(static_cast<uint32_t>(std::floor(pixel)) / kCompressionBlock, blockCount);
}

uint32_t CImageAtlas::ClampBlockCeil(double pixel, uint32_t blockCount)
{
    if (pixel <= 0.0)
        return 0;
    return std::min(static_cast<uint32_t>(std::ceil(pixel / kCompressionBlock)), blockCount);
}

std::optional<CImageAtlas::Placement> CImageAtlas::FindFreePlacement(std::span<const RuiImageAtlasGpuRecord> currentImages, uint16_t atlasWidth,
                                                                     uint16_t atlasHeight, uint16_t imageWidth, uint16_t imageHeight)
{
    if (atlasWidth == 0 || atlasHeight == 0 || imageWidth == 0 || imageHeight == 0 || atlasWidth % kCompressionBlock != 0 ||
        atlasHeight % kCompressionBlock != 0 || imageWidth % kCompressionBlock != 0 || imageHeight % kCompressionBlock != 0 ||
        imageWidth > atlasWidth || imageHeight > atlasHeight)
    {
        return std::nullopt;
    }

    const uint32_t columns = atlasWidth / kCompressionBlock;
    const uint32_t rows = atlasHeight / kCompressionBlock;
    std::vector<uint8_t> occupied(static_cast<size_t>(columns) * rows);
    for (const RuiImageAtlasGpuRecord& image : currentImages)
    {
        const double u = image.uvMin[0];
        const double v = image.uvMin[1];
        const double width = image.uvSize[0];
        const double height = image.uvSize[1];
        if (!std::isfinite(u) || !std::isfinite(v) || !std::isfinite(width) || !std::isfinite(height) || u < 0.0 || v < 0.0 || width < 0.0 ||
            height < 0.0 || u + width > 1.000001 || v + height > 1.000001)
        {
            return std::nullopt;
        }

        const uint32_t left = ClampBlockFloor(u * atlasWidth - 1.0, columns);
        const uint32_t top = ClampBlockFloor(v * atlasHeight - 1.0, rows);
        const uint32_t right = ClampBlockCeil((u + width) * atlasWidth + 1.0, columns);
        const uint32_t bottom = ClampBlockCeil((v + height) * atlasHeight + 1.0, rows);
        for (uint32_t y = top; y < bottom; ++y)
            std::fill_n(occupied.begin() + static_cast<size_t>(y) * columns + left, right - left, uint8_t{1});
    }

    const uint32_t neededColumns = imageWidth / kCompressionBlock;
    const uint32_t neededRows = imageHeight / kCompressionBlock;
    std::vector<uint32_t> heights(columns);
    for (uint32_t y = 0; y < rows; ++y)
    {
        uint32_t run = 0;
        for (uint32_t x = 0; x < columns; ++x)
        {
            heights[x] = occupied[static_cast<size_t>(y) * columns + x] ? 0 : heights[x] + 1;
            run = heights[x] >= neededRows ? run + 1 : 0;
            if (run < neededColumns)
                continue;

            Placement region;
            region.x = static_cast<uint16_t>((x + 1 - neededColumns) * kCompressionBlock);
            region.y = static_cast<uint16_t>((y + 1 - neededRows) * kCompressionBlock);
            region.width = imageWidth;
            region.height = imageHeight;
            region.gpuRecord.uvMin[0] = (static_cast<float>(region.x) + 0.5f) / atlasWidth;
            region.gpuRecord.uvMin[1] = (static_cast<float>(region.y) + 0.5f) / atlasHeight;
            region.gpuRecord.uvSize[0] = (static_cast<float>(imageWidth) - 1.0f) / atlasWidth;
            region.gpuRecord.uvSize[1] = (static_cast<float>(imageHeight) - 1.0f) / atlasHeight;
            return region;
        }
    }

    return std::nullopt;
}

std::optional<CImageAtlas::AtlasData> CImageAtlas::ExtendAtlasData(const RuiImageAtlas& atlas,
                                                                   std::span<const RuiImageAtlasGpuRecord> currentGpuRecords, uint32_t nameHash,
                                                                   const RuiImageAtlasEntry& entry, uint16_t imageWidth, uint16_t imageHeight,
                                                                   const RuiImageAtlasGpuRecord& gpuRecord)
{
    if (!atlas.images || !atlas.imageDimensions || !atlas.imageNameRecords || atlas.imageCount == 0 || atlas.imageCount >= INT16_MAX ||
        currentGpuRecords.size() != atlas.imageCount || nameHash == 0 || imageWidth == 0 || imageHeight == 0)
    {
        return std::nullopt;
    }

    for (const RuiImageAtlasNameRecord& current : std::span(atlas.imageNameRecords, atlas.imageCount))
    {
        if (current.nameHash == nameHash)
            return std::nullopt;
    }

    AtlasData storage;
    const size_t newCount = static_cast<size_t>(atlas.imageCount) + 1;
    storage.entries.reserve(newCount);
    storage.dimensions.reserve(newCount);
    storage.names.reserve(newCount);
    storage.gpuRecords.reserve(newCount);
    storage.entries.assign(atlas.images, atlas.images + atlas.imageCount);
    storage.dimensions.assign(atlas.imageDimensions, atlas.imageDimensions + atlas.imageCount);
    storage.names.assign(atlas.imageNameRecords, atlas.imageNameRecords + atlas.imageCount);
    storage.gpuRecords.assign(currentGpuRecords.begin(), currentGpuRecords.end());
    storage.entries.push_back(entry);
    storage.dimensions.push_back({imageWidth, imageHeight});
    storage.names.push_back({nameHash, 0, 0});
    storage.gpuRecords.push_back(gpuRecord);
    return storage;
}

CImageAtlas& CImageAtlas::NativeMenuExtension()
{
    // Renderer services may already be gone during CRT teardown.
    static CImageAtlas* atlas = new CImageAtlas;
    return *atlas;
}

uint64_t CImageAtlas::HashBytes(std::span<const std::byte> bytes)
{
    uint64_t hash = 0xCBF29CE484222325;
    for (const std::byte value : bytes)
    {
        hash ^= static_cast<uint8_t>(value);
        hash *= 0x100000001B3;
    }
    return hash;
}

void CImageAtlas::CaptureNativeAtlasGpuRecords(const RuiImageAtlas* atlas, const RuiImageAtlasGpuRecord* records)
{
    if (atlas && records)
        NativeMenuExtension().CaptureNativeGpuRecords(*atlas, std::span(records, atlas->imageCount));
}

void CImageAtlas::CaptureNativeGpuRecords(const RuiImageAtlas& atlas, std::span<const RuiImageAtlasGpuRecord> records)
{
    if (!IsExpectedNativeAtlas(atlas) || records.size() != atlas.imageCount)
        return;
    std::scoped_lock lock(m_NativeMenuMutex);
    m_CapturedNativeGpuRecords.assign(records.begin(), records.end());
    m_NativeMenuDirty.store(true, std::memory_order_release);
}

bool CImageAtlas::IsExpectedNativeAtlas(const RuiImageAtlas& atlas) const
{
    return atlas.width == kNativeMenuWidth && atlas.height == kNativeMenuHeight && atlas.imageCount == kNativeMenuImageCount &&
           atlas.nineSliceImageCount == kNativeMenuNineSliceCount && atlas.images && atlas.imageDimensions && atlas.nineSliceData &&
           atlas.imageNameRecords && atlas.imageNames && atlas.texture &&
           HashBytes(std::as_bytes(std::span(atlas.images, atlas.imageCount))) == 0x09F1F22FE0B835BB &&
           HashBytes(std::as_bytes(std::span(atlas.imageDimensions, atlas.imageCount))) == 0x5B01024650D924DE &&
           HashBytes(std::as_bytes(std::span(atlas.nineSliceData, atlas.nineSliceImageCount))) == 0x8C7E62CD3AB03F80 &&
           HashBytes(std::as_bytes(std::span(atlas.imageNameRecords, atlas.imageCount))) == 0x78F3916CD6A6C8AA;
}

bool CImageAtlas::OwnsExtendedNativeAtlas(const RuiImageAtlas* atlas) const
{
    return m_NativeMenuApplied && atlas && atlas == GetAtlas(m_NativeMenuAtlasIndex) && atlas->images == m_NativeMenuData.entries.data() &&
           atlas->imageDimensions == m_NativeMenuData.dimensions.data() && atlas->imageNameRecords == m_NativeMenuData.names.data() &&
           atlas->imageCount == m_OriginalNativeMenuAtlas.imageCount + 1 && atlas->texture == m_NativeMenuTextureHeader.get() &&
           atlas->gpuRecordBuffer == m_NativeMenuGpuBuffer;
}

bool CImageAtlas::DetachNativeExtension()
{
    if (!m_NativeMenuApplied)
        return true;
    RuiImageAtlas* const atlas = GetAtlas(m_NativeMenuAtlasIndex);
    InvalidateImageDescriptor(m_NativeMenuNameHash, m_NativeMenuAtlasIndex, static_cast<int16_t>(m_OriginalNativeMenuAtlas.imageCount));
    if (OwnsExtendedNativeAtlas(atlas))
        *atlas = m_OriginalNativeMenuAtlas;
    else if (atlas && (atlas->images == m_NativeMenuData.entries.data() || atlas->imageDimensions == m_NativeMenuData.dimensions.data() ||
                       atlas->imageNameRecords == m_NativeMenuData.names.data() || atlas->texture == m_NativeMenuTextureHeader.get() ||
                       atlas->gpuRecordBuffer == m_NativeMenuGpuBuffer))
    {
        spdlog::error("Nexus atlas extension retains resources after unexpected partial ownership change");
        return false;
    }
    m_NativeMenuApplied = false;
    return true;
}

void CImageAtlas::RetireNativeExtensionResources()
{
    if (m_NativeMenuGpuBuffer != UINT_MAX)
        DestroyGpuRecordBuffer(m_NativeMenuGpuBuffer);
    m_NativeMenuGpuBuffer = UINT_MAX;
    m_NativeMenuView.Reset();
    m_NativeMenuTexture.Reset();
    m_NativeMenuTextureHeader.reset();
    m_NativeMenuData = {};
    m_OriginalNativeMenuAtlas = {};
}

bool CImageAtlas::IsTextureOnDevice(ID3D11Texture2D* texture, ID3D11Device* device) const
{
    if (!texture)
        return false;
    Microsoft::WRL::ComPtr<ID3D11Device> owner;
    texture->GetDevice(&owner);
    return owner.Get() == device;
}

CImageAtlas::UpdateResult CImageAtlas::AcquireSourceTexture(ID3D11Device* device)
{
    const RuiImageAssetDescriptor* const descriptor = FindAssetDescriptor(m_NativeMenuNameHash);
    const bool ownDescriptor = m_NativeMenuApplied && descriptor && descriptor->atlasIndex == m_NativeMenuAtlasIndex &&
                               descriptor->imageIndex == static_cast<int16_t>(m_OriginalNativeMenuAtlas.imageCount);
    if (!m_SourceEntryKnown || (descriptor && descriptor->imageIndex >= 0 && !ownDescriptor))
    {
        if (!descriptor || descriptor->imageIndex < 0)
            return UpdateResult::Waiting;
        const RuiImageAtlas* const atlas = GetAtlas(descriptor->atlasIndex);
        if (!atlas || !atlas->images || !atlas->texture || static_cast<uint16_t>(descriptor->imageIndex) >= atlas->imageCount)
            return UpdateResult::Waiting;
        const RuiImageAtlasEntry entry = atlas->images[descriptor->imageIndex];
        const auto* const texture = static_cast<const TextureAsset_s*>(atlas->texture);
        if (atlas->width != kSourceWidth || atlas->height != kSourceHeight || atlas->imageCount != 1 || descriptor->flags != 0 ||
            texture->assetGuid != kSourceTextureGuid || entry.pixelBounds[0] != 0.0f || entry.pixelBounds[1] != 0.0f ||
            entry.pixelBounds[2] != 1.0f || entry.pixelBounds[3] != 1.0f || entry.uvBase[0] != 0.0f || entry.uvBase[1] != 0.0f ||
            entry.uvScale[0] != 1.0f || entry.uvScale[1] != 1.0f)
        {
            spdlog::error("Nexus atlas extension refused unexpected source atlas contract");
            return UpdateResult::Refused;
        }
        m_SourceEntry = entry;
        m_SourceEntryKnown = true;
    }
    const bool cachedSourceReady = IsTextureOnDevice(m_SourceTexture.Get(), device);
    const auto* const source = g_pakLoadApi ? reinterpret_cast<const TextureAsset_s*>(g_pakLoadApi->GetAssetBinding(kSourceTextureGuid)) : nullptr;
    if (!source || source->assetGuid != kSourceTextureGuid || !source->d3d11Resource)
        return cachedSourceReady ? UpdateResult::Ready : UpdateResult::Waiting;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> current;
    if (FAILED(source->d3d11Resource->QueryInterface(IID_PPV_ARGS(&current))) || !IsTextureOnDevice(current.Get(), device))
        return UpdateResult::Waiting;
    D3D11_TEXTURE2D_DESC description{};
    current->GetDesc(&description);
    if (description.Width != kSourceWidth || description.Height != kSourceHeight || description.MipLevels != 1 || description.ArraySize != 1 ||
        description.SampleDesc.Count != 1 || description.Format != DXGI_FORMAT_BC7_UNORM_SRGB)
    {
        spdlog::error("Nexus atlas extension refused unexpected source texture contract");
        return UpdateResult::Refused;
    }
    m_SourceTexture = std::move(current);
    return UpdateResult::Ready;
}

CImageAtlas::UpdateResult CImageAtlas::UpdateNativeMenuAtlas(ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (m_NativeMenuNameHash == 0)
        m_NativeMenuNameHash = HashImagePath(kNativeMenuImagePath);
    const auto previousSource = m_SourceTexture;
    const UpdateResult sourceResult = AcquireSourceTexture(device);
    if (m_NativeMenuApplied)
    {
        if (sourceResult == UpdateResult::Ready && previousSource == m_SourceTexture && OwnsExtendedNativeAtlas(GetAtlas(m_NativeMenuAtlasIndex)) &&
            IsTextureOnDevice(m_NativeMenuTexture.Get(), device))
        {
            return RedirectImageDescriptor(m_NativeMenuNameHash, m_NativeMenuAtlasIndex, static_cast<int16_t>(m_OriginalNativeMenuAtlas.imageCount))
                       ? UpdateResult::Ready
                       : UpdateResult::Waiting;
        }
        if (!DetachNativeExtension())
            return UpdateResult::Refused;
    }
    RetireNativeExtensionResources();
    if (sourceResult != UpdateResult::Ready)
        return sourceResult;

    const RuiImageAssetDescriptor* const border = FindAssetDescriptor(HashImagePath(kNativeMenuBorderPath));
    RuiImageAtlas* const atlas = border && border->imageIndex >= 0 ? GetAtlas(border->atlasIndex) : nullptr;
    if (!atlas || !atlas->images || !atlas->texture || m_CapturedNativeGpuRecords.size() != atlas->imageCount)
        return UpdateResult::Waiting;
    if (border->imageIndex != 0 || !IsExpectedNativeAtlas(*atlas))
    {
        spdlog::error("Nexus atlas extension refused unexpected native menu atlas layout");
        return UpdateResult::Refused;
    }
    const auto* const nativeTexture = static_cast<const TextureAsset_s*>(atlas->texture);
    if (nativeTexture->assetGuid != kNativeMenuTextureGuid || nativeTexture->permanentMipLevels != 1 || nativeTexture->streamedMipLevels != 0)
    {
        spdlog::error("Nexus atlas extension refused unexpected native texture contract");
        return UpdateResult::Refused;
    }
    Microsoft::WRL::ComPtr<ID3D11Texture2D> nativeResource;
    if (!nativeTexture->d3d11Resource || !nativeTexture->shaderResourceView ||
        FAILED(nativeTexture->d3d11Resource->QueryInterface(IID_PPV_ARGS(&nativeResource))) || !IsTextureOnDevice(nativeResource.Get(), device))
        return UpdateResult::Waiting;
    D3D11_TEXTURE2D_DESC nativeDescription{};
    nativeResource->GetDesc(&nativeDescription);
    if (nativeDescription.Width != atlas->width || nativeDescription.Height != atlas->height || nativeDescription.MipLevels != 1 ||
        nativeDescription.ArraySize != 1 || nativeDescription.SampleDesc.Count != 1 || nativeDescription.Format != DXGI_FORMAT_BC7_UNORM_SRGB)
    {
        spdlog::error("Nexus atlas extension refused unexpected native GPU resource");
        return UpdateResult::Refused;
    }
    const auto placement = FindFreePlacement(m_CapturedNativeGpuRecords, atlas->width, atlas->height, kSourceWidth, kSourceHeight);
    if (!placement)
    {
        spdlog::error("Nexus atlas extension found no protected native-atlas space");
        return UpdateResult::Refused;
    }
    auto data =
        ExtendAtlasData(*atlas, m_CapturedNativeGpuRecords, m_NativeMenuNameHash, m_SourceEntry, kSourceWidth, kSourceHeight, placement->gpuRecord);
    if (!data)
        return UpdateResult::Refused;
    m_NativeMenuData = std::move(*data);

    D3D11_TEXTURE2D_DESC patchedDescription = nativeDescription;
    patchedDescription.Usage = D3D11_USAGE_DEFAULT;
    patchedDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    patchedDescription.CPUAccessFlags = 0;
    patchedDescription.MiscFlags = 0;
    if (FAILED(device->CreateTexture2D(&patchedDescription, nullptr, &m_NativeMenuTexture)))
        return UpdateResult::Waiting;
    context->CopyResource(m_NativeMenuTexture.Get(), nativeResource.Get());
    const D3D11_BOX sourceBox{.left = 0, .top = 0, .front = 0, .right = kSourceWidth, .bottom = kSourceHeight, .back = 1};
    context->CopySubresourceRegion(m_NativeMenuTexture.Get(), 0, placement->x, placement->y, 0, m_SourceTexture.Get(), 0, &sourceBox);
    if (FAILED(device->CreateShaderResourceView(m_NativeMenuTexture.Get(), nullptr, &m_NativeMenuView)))
    {
        RetireNativeExtensionResources();
        return UpdateResult::Waiting;
    }
    m_NativeMenuTextureHeader = std::make_unique<TextureAsset_s>(*nativeTexture);
    m_NativeMenuTextureHeader->d3d11Resource = m_NativeMenuTexture.Get();
    m_NativeMenuTextureHeader->shaderResourceView = m_NativeMenuView.Get();

    m_OriginalNativeMenuAtlas = *atlas;
    m_NativeMenuAtlasIndex = border->atlasIndex;
    RuiImageAtlas candidate = m_OriginalNativeMenuAtlas;
    ++candidate.imageCount;
    candidate.images = m_NativeMenuData.entries.data();
    candidate.imageDimensions = m_NativeMenuData.dimensions.data();
    candidate.imageNameRecords = m_NativeMenuData.names.data();
    candidate.texture = m_NativeMenuTextureHeader.get();
    m_NativeMenuGpuBuffer = CreateGpuRecordBuffer(candidate, m_NativeMenuData.gpuRecords);
    if (m_NativeMenuGpuBuffer == UINT_MAX || candidate.gpuRecordBuffer != m_NativeMenuGpuBuffer)
    {
        RetireNativeExtensionResources();
        return UpdateResult::Waiting;
    }
    *atlas = candidate;
    if (!RedirectImageDescriptor(m_NativeMenuNameHash, m_NativeMenuAtlasIndex, static_cast<int16_t>(m_OriginalNativeMenuAtlas.imageCount)))
    {
        *atlas = m_OriginalNativeMenuAtlas;
        RetireNativeExtensionResources();
        return UpdateResult::Waiting;
    }
    m_NativeMenuApplied = true;
    spdlog::info("Extended native menu atlas {} at ({}, {})", m_NativeMenuAtlasIndex, placement->x, placement->y);
    return UpdateResult::Ready;
}

void CImageAtlas::UpdateNativeMenuAtlasOnRenderThread()
{
    if (!g_pPakLoadManager || !g_pPakLoadManager->TryAcquireIdlePakLock())
    {
        m_NativeMenuQueued.store(false, std::memory_order_release);
        return;
    }
    const ScopeGuard pakLock([&]() { g_pPakLoadManager->ReleasePakLock(); });
    std::scoped_lock lock(m_NativeMenuMutex);
    const CDx11Device::Snapshot dx11 = CDx11Device::GetSnapshot();
    if (dx11)
    {
        const UpdateResult result = UpdateNativeMenuAtlas(dx11.m_pDevice, dx11.m_pContext);
        m_NativeMenuDevice.store(dx11.m_pDevice, std::memory_order_release);
        m_NativeMenuDirty.store(result == UpdateResult::Waiting, std::memory_order_release);
    }
    m_NativeMenuQueued.store(false, std::memory_order_release);
}

void CImageAtlas::RequestImage(const char* imagePath)
{
    if (!imagePath || _stricmp(imagePath, kNativeMenuImagePath) != 0)
        return;
    m_NativeMenuRequested.store(true, std::memory_order_release);
    m_NativeMenuDirty.store(true, std::memory_order_release);
}

void CImageAtlas::RequestNativeMenuImage(const char* imagePath)
{
    NativeMenuExtension().RequestImage(imagePath);
}

void CImageAtlas::ScheduleNativeMenuUpdate()
{
    if (!m_NativeMenuRequested.load(std::memory_order_acquire))
        return;
    const CDx11Device::Snapshot dx11 = CDx11Device::GetSnapshot();
    if (!dx11 || !AreRuntimeBindingsReady() || !CRuiRenderTaskQueue::Get().IsInitialized())
        return;
    if (!m_NativeMenuDirty.load(std::memory_order_acquire) && m_NativeMenuDevice.load(std::memory_order_acquire) == dx11.m_pDevice)
        return;
    if (!m_NativeMenuQueued.exchange(true, std::memory_order_acq_rel))
        CRuiRenderTaskQueue::Get().Dispatch([this]() { UpdateNativeMenuAtlasOnRenderThread(); });
}

void CImageAtlas::RunFrame()
{
    NativeMenuExtension().ScheduleNativeMenuUpdate();
}

void CImageAtlas::OnAtlasReplacing(RuiImageAtlas* atlas)
{
    std::scoped_lock lock(m_NativeMenuMutex);
    if (m_NativeMenuApplied && atlas &&
        (atlas == GetAtlas(m_NativeMenuAtlasIndex) ||
         (atlas->images == m_OriginalNativeMenuAtlas.images && atlas->gpuRecordBuffer == m_OriginalNativeMenuAtlas.gpuRecordBuffer)))
        DetachNativeExtension();
    m_NativeMenuDirty.store(true, std::memory_order_release);
}
