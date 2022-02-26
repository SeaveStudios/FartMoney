#pragma once

#include "../SDK/ItemSchema.h"
#include "../SDK/WeaponId.h"
#include "StaticData.h"
#include "StaticDataStorage.h"
#include "../Helpers.h"

class StaticDataContainer {
private:
    auto findItems(WeaponId weaponID) const noexcept
    {
        struct Comp {
            bool operator()(WeaponId weaponID, const game_items::Item& item) const noexcept { return weaponID < item.weaponID; }
            bool operator()(const game_items::Item& item, WeaponId weaponID) const noexcept { return item.weaponID < weaponID; }
        };

        return std::equal_range(storage.getGameItems().cbegin(), storage.getGameItems().cend(), weaponID, Comp{}); // not using std::ranges::equal_range() here because clang 12 on linux doesn't support it yet
    }

public:
    StaticDataContainer() = default;
    explicit StaticDataContainer(StaticDataStorage dataStorage) : storage{ sorted(std::move(dataStorage)) }
    {
        const auto stickers = findItems(WeaponId::Sticker);
        tournamentStickersSorted = { stickers.first, stickers.second };

        std::ranges::sort(tournamentStickersSorted, [this](const game_items::Item& itemA, const game_items::Item& itemB) {
            assert(itemA.isSticker() && itemB.isSticker());

            const auto& paintKitA = storage.getStickerKit(itemA);
            const auto& paintKitB = storage.getStickerKit(itemB);
            if (paintKitA.tournamentID != paintKitB.tournamentID)
                return paintKitA.tournamentID < paintKitB.tournamentID;
            if (paintKitA.tournamentTeam != paintKitB.tournamentTeam)
                return paintKitA.tournamentTeam < paintKitB.tournamentTeam;
            if (paintKitA.tournamentPlayerID != paintKitB.tournamentPlayerID)
                return paintKitA.tournamentPlayerID < paintKitB.tournamentPlayerID;
            if (paintKitA.isGoldenSticker != paintKitB.isGoldenSticker)
                return paintKitA.isGoldenSticker;
            return itemA.rarity > itemB.rarity;
        });
    }

    const StaticDataStorage& getStorage() const noexcept
    {
        return storage;
    }

private:
    auto getTournamentStickers(std::uint32_t tournamentID) const noexcept
    {
        // not using std::ranges::equal_range() here because clang 12 on linux doesn't support it yet
        const auto begin = std::lower_bound(tournamentStickersSorted.begin(), tournamentStickersSorted.end(), tournamentID, [this](const game_items::Item& item, std::uint32_t tournamentID) {
            return storage.getStickerKit(item).tournamentID < tournamentID;
        });

        const auto end = std::upper_bound(tournamentStickersSorted.begin(), tournamentStickersSorted.end(), tournamentID, [this](std::uint32_t tournamentID, const game_items::Item& item) {
            return storage.getStickerKit(item).tournamentID > tournamentID;
        });

        return std::make_pair(begin, end);
    }

public:
    int getTournamentEventStickerID(std::uint32_t tournamentID) const noexcept
    {
        if (tournamentID == 1) // DreamHack 2013
            return Helpers::random(1, 12);
        else if (tournamentID == 3) // EMS One Katowice 2014
            return Helpers::random(99, 100); // EMS Wolf / Skull
        else if (tournamentID == 4) // ELS One Cologne 2014
            return 172;

        const auto it = getTournamentStickers(tournamentID).first;
        if (it == tournamentStickersSorted.end())
            return 0;
        return storage.getStickerKit(*it).tournamentID == tournamentID ? storage.getStickerKit(*it).id : 0;
    }

    int getTournamentTeamGoldStickerID(std::uint32_t tournamentID, TournamentTeam team) const noexcept
    {
        if (tournamentID == 0 || team == TournamentTeam::None)
            return 0;

        if (team == TournamentTeam::AllStarTeamAmerica)
            return 1315;
        if (team == TournamentTeam::AllStarTeamEurope)
            return 1316;

        const auto range = getTournamentStickers(tournamentID);

        const auto it = std::ranges::lower_bound(range.first, range.second, team, {}, [this](const game_items::Item& item) {
            return storage.getStickerKit(item).tournamentTeam;
        });
        if (it == range.second)
            return 0;
        return storage.getStickerKit(*it).tournamentTeam == team ? storage.getStickerKit(*it).id : 0;
    }

    int getTournamentPlayerGoldStickerID(std::uint32_t tournamentID, int tournamentPlayerID) const noexcept
    {
        const auto range = getTournamentStickers(tournamentID);
        const auto it = std::ranges::find(range.first, range.second, tournamentPlayerID, [this](const game_items::Item& item) { return storage.getStickerKit(item).tournamentPlayerID; });
        return (it != range.second ? storage.getStickerKit(*it).id : 0);
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const game_items::Item>> getItem(WeaponId weaponID, int paintKit) const noexcept
    {
        const auto [begin, end] = findItems(weaponID);
        if (begin != end && !begin->isSkin() && !begin->isGloves())
            return {};

        if (const auto it = std::lower_bound(begin, end, paintKit, [this](const game_items::Item& item, int paintKit) { return storage.getPaintKit(item).id < paintKit; }); it != end && storage.getPaintKit(*it).id == paintKit)
            return *it;
        return {};
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const game_items::Item>> getItem(WeaponId weaponID) const noexcept
    {
        if (const auto it = std::ranges::lower_bound(storage.getGameItems(), weaponID, {}, &game_items::Item::weaponID); it != storage.getGameItems().end())
            return *it;
        return {};
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const game_items::Item>> getMusic(int musicKit) const noexcept
    {
        return findItem(WeaponId::MusicKit, musicKit, [this](const game_items::Item& item) { return storage.getMusicKit(item).id; });
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const game_items::Item>> getSticker(int stickerKit) const noexcept
    {
        return findItem(WeaponId::Sticker, stickerKit, [this](const game_items::Item& item) { return storage.getStickerKit(item).id; });
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const game_items::Item>> getGraffiti(int graffitiID) const noexcept
    {
        return findItem(WeaponId::Graffiti, graffitiID, [this](const game_items::Item& item) { return storage.getGraffitiKit(item).id; });
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const game_items::Item>> getSealedGraffiti(int graffitiID) const noexcept
    {
        return findItem(WeaponId::SealedGraffiti, graffitiID, [this](const game_items::Item& item) { return storage.getGraffitiKit(item).id; });
    }

    [[nodiscard]] std::optional<std::reference_wrapper<const game_items::Item>> getPatch(int patchID) const noexcept
    {
        return findItem(WeaponId::Patch, patchID, [this](const game_items::Item& item) { return storage.getPatchKit(item).id; });
    }

private:
    template <typename T, typename Projection>
    [[nodiscard]] std::optional<std::reference_wrapper<const game_items::Item>> findItem(WeaponId weaponID, const T& value, Projection projection) const
    {
        const auto [begin, end] = findItems(weaponID);
        if (const auto it = std::ranges::find(begin, end, value, projection); it != end)
            return *it;
        return {};
    }

    [[nodiscard]] static StaticDataStorage sorted(StaticDataStorage storage)
    {
        std::ranges::sort(storage.getGameItems(), [&storage](const game_items::Item& itemA, const game_items::Item& itemB) {
            if (itemA.weaponID == itemB.weaponID && (itemA.isSkin() || itemA.isGloves()) && (itemB.isSkin() || itemB.isGloves()))
                return storage.getPaintKit(itemA).id < storage.getPaintKit(itemB).id;
            return itemA.weaponID < itemB.weaponID;
        });
        return storage;
    }

    StaticDataStorage storage;
    std::vector<std::reference_wrapper<const game_items::Item>> tournamentStickersSorted;
};
