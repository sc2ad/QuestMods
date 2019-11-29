#include <android/log.h>
#include <stdlib.h>
#include <time.h>
#include <stdalign.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <array>
#include <vector>
#include <limits>
#include <map>
#include <dlfcn.h>
#include "../pistol-whip-hook/shared/utils/logging.h"
#include "../pistol-whip-hook/shared/inline-hook/inlineHook.h"
#include "../pistol-whip-hook/shared/utils/utils.h"
#include "../pistol-whip-hook/shared/utils/utils-functions.hpp"
#include "../pistol-whip-hook/shared/utils/il2cpp-functions.hpp"
#include "../pistol-whip-hook/shared/utils/il2cpp-utils.hpp"
#include "../pistol-whip-hook/shared/utils/typedefs.h"

static Il2CppObject *GameData;

MAKE_HOOK_OFFSETLESS(PlayerActionManagerGameStart, void, void *self, void *e)
{
    log(INFO, "Called PlayerActionManager GameStart Hook!");
    PlayerActionManagerGameStart(self, e);
    GameData = nullptr;
    GameData = il2cpp_utils::GetFieldValue(reinterpret_cast<Il2CppObject *>(self), "playerData");
    log(INFO, "GameData: %p", GameData);
    log(INFO, "Finished PlayerActionManager GameStart Hook!");
}

static float lastAcc = -1;   // To ensure it updates immediately
static int lastBullets = -1; // To ensure it updates immediately

MAKE_HOOK_OFFSETLESS(GunAmmoDisplayUpdate, void, Il2CppObject *self)
{
    GunAmmoDisplayUpdate(self);

    static auto tmp_text_class = il2cpp_utils::GetClassFromName("TMPro", "TMP_Text");
    static auto get_text_method = il2cpp_utils::GetMethod(tmp_text_class, "get_text", 0);
    static auto set_text_method = il2cpp_utils::GetMethod(tmp_text_class, "set_text", 1);
    static auto set_richText_method = il2cpp_utils::GetMethod(tmp_text_class, "set_richText", 1);
    if (GameData == nullptr)
    {
        return;
    }
    float accuracy;
    il2cpp_utils::GetFieldValue(&accuracy, GameData, "accuracy");
    if (lastAcc == accuracy)
        return; // No accuracy change

    int bulletCount;
    il2cpp_utils::GetFieldValue(&bulletCount, reinterpret_cast<Il2CppObject *>(self), "currentBulletCount");

    Il2CppObject *displayTextObj = il2cpp_utils::GetFieldValue(self, "displayText");

    log(INFO, "displayTextObj: %p", displayTextObj);
    Il2CppString *displayText;
    il2cpp_utils::RunMethod(&displayText, displayTextObj, get_text_method);
    log(INFO, "displayText: %p", displayText);
    std::string text = to_utf8(csstrtostr(displayText));
    log(INFO, "displayText text: %s", text.data());
    log(INFO, "Accuracy: %.2f", accuracy);
    char buffer[10];
    sprintf(buffer, "(%.2f%%)", accuracy * 100);
    text = std::to_string(bulletCount) + "\n<size=75%>" + std::string(buffer);
    log(INFO, "Updated text: %s", text.data());
    bool richTextValue = true;
    il2cpp_utils::RunMethod(displayTextObj, set_richText_method, &richTextValue);
    il2cpp_utils::RunMethod(displayTextObj, set_text_method, il2cpp_functions::string_new(text.data()));

    lastAcc = accuracy;
}

// THESE HOOKS JUST MAKE SURE WE UPDATE THE DISPLAY WHEN WE DO THINGS

MAKE_HOOK_OFFSETLESS(GunReload, void, void *self, bool ignoreBulletWaste)
{
    lastAcc = -1;
    GunReload(self, ignoreBulletWaste);
}

MAKE_HOOK_OFFSETLESS(GunFire, void, void *self)
{
    lastAcc = -1;
    GunFire(self);
}

MAKE_HOOK_OFFSETLESS(GameDataAddScore, void, Il2CppObject *self, Il2CppObject *ScoreItem)
{
    log(INFO, "GameData AddScore hook called.");
    GameDataAddScore(self, ScoreItem);
    // ScoreItem is actually a struct not a class!
    log(INFO, "ScoreItem: %p", ScoreItem);
    // Hack for struct field fix? Might need to double check to see how this behaves with structs, since it seemingly does not.
    auto tmp = reinterpret_cast<int*>(ScoreItem);
    // Field of interest: 0x14
    log(INFO, "onBeatValue of score being added: %i", tmp[5]);
}

extern "C" void load() {
    log(INFO, "Loaded AccCounter!");
    log(INFO, "Installing AccCounter Hooks!");
    INSTALL_HOOK_OFFSETLESS(PlayerActionManagerGameStart, il2cpp_utils::GetMethod("", "PlayerActionManager", "OnGameStart", 1));
    INSTALL_HOOK_OFFSETLESS(GunAmmoDisplayUpdate, il2cpp_utils::GetMethod("", "GunAmmoDisplay", "Update", 0));
    INSTALL_HOOK_OFFSETLESS(GunReload, il2cpp_utils::GetMethod("", "Gun", "Reload", 1));
    INSTALL_HOOK_OFFSETLESS(GunFire, il2cpp_utils::GetMethod("", "Gun", "Fire", 0));
    INSTALL_HOOK_OFFSETLESS(GameDataAddScore, il2cpp_utils::GetMethod("", "GameData", "AddScore", 1));
    log(INFO, "Installed AccCounter Hooks!");
}

__attribute__((constructor)) void lib_main()
{
    log(INFO, "Constructed AccCounter!");
}