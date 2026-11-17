/*
 * kj_screen.c: which application the game is showing (issue #15).
 *
 * Every HGSS screen runs as an OverlayManager application (Init / Main /
 * Exit). The patched OverlayManager_Run (patch 2009) reports each run; a
 * child application runs inside its parent's Main, so the deepest run of a
 * frame is the screen on display. When that changes, the screen gets a new
 * stable ID and a new layout generation. Applications are recognised by
 * their template's init function, compared against the game's own
 * templates and init functions (weak: a symbol the build lacks never
 * matches). Reference: Unown's soulsilver3ds/platform/ss3ds_widescreens.c,
 * revalidated against this pret/pokeheartgold revision's names.
 *
 * Display only: nothing here touches game state or the save. A reset
 * restarts the program, which clears this state.
 */

#include <nitro.h>
#include <nitro_port.h>
#include <nitro_port_app.h>
#include <stddef.h>
#include <stdint.h>

#include "kj_bridge.h"
#include "kj_layouts.h"
#include "kj_screen.h"

/* OverlayManagerTemplate (include/overlay_manager.h): init, main, exit,
 * overlay ID. Only the init word is read. */
typedef struct app_template {
    const void *init, *main, *exit;
    uint32_t ovy;
} app_template;

typedef int (*app_fn)(void *, int *);

#define TEMPLATES(X)                                        \
    X(gOverlayTemplate_FieldMap, KJ_SCREEN_FIELD)           \
    X(gOverlayTemplate_Battle, KJ_SCREEN_BATTLE)            \
    X(gApplication_TitleScreen, KJ_SCREEN_TITLE)            \
    X(gApplication_IntroMovie, KJ_SCREEN_INTRO)             \
    X(gApp_MainMenu, KJ_SCREEN_MAIN_MENU)                   \
    X(gApplication_OakSpeech, KJ_SCREEN_OAK_SPEECH)         \
    X(gOverlayTemplate_PartyMenu, KJ_SCREEN_PARTY)          \
    X(gOverlayTemplate_PokemonSummary, KJ_SCREEN_SUMMARY)

#define INITS(X)                                            \
    X(Bag_Init, KJ_SCREEN_BAG)                              \
    X(PCBox_Init, KJ_SCREEN_PC_BOX)                         \
    X(Pokedex_Init, KJ_SCREEN_POKEDEX)                      \
    X(Pokegear_Init, KJ_SCREEN_POKEGEAR)                    \
    X(PokegearMap_Init, KJ_SCREEN_GEAR_MAP)                 \
    X(PokegearPhone_Init, KJ_SCREEN_GEAR_PHONE)             \
    X(PokegearRadio_Init, KJ_SCREEN_GEAR_RADIO)             \
    X(PokegearConfigure_Init, KJ_SCREEN_GEAR_CONFIG)        \
    X(TrainerCardMainApp_Init, KJ_SCREEN_TRAINER_CARD)      \
    X(TrainerCardSignature_Init, KJ_SCREEN_TC_SIGNATURE)    \
    X(OptionsMenu_Init, KJ_SCREEN_OPTIONS)                  \
    X(FlyMap_Init, KJ_SCREEN_FLY_MAP)                       \
    X(ChooseStarter_Init, KJ_SCREEN_STARTER)                \
    X(Credits_Init, KJ_SCREEN_CREDITS)                      \
    X(EasyChat_Init, KJ_SCREEN_EASY_CHAT)                   \
    X(NamingScreenApp_Init, KJ_SCREEN_NAMING)

#define X(sym, id) extern const app_template sym __attribute__((weak));
TEMPLATES(X)
#undef X
#define X(sym, id) extern int sym(void *, int *) __attribute__((weak));
INITS(X)
#undef X

static const char *const NAMES[KJ_SCREEN_COUNT] = KJ_SCREEN_NAMES_INIT;

static uint32_t classify(const void *init)
{
    if (init == NULL) {
        return KJ_SCREEN_UNKNOWN;
    }
#define X(sym, id)                                          \
    if (&sym != NULL && sym.init == init) {                 \
        return id;                                          \
    }
    TEMPLATES(X)
#undef X
#define X(sym, id)                                          \
    if (sym != NULL && (const void *)sym == init) {         \
        return id;                                          \
    }
    INITS(X)
#undef X
    return KJ_SCREEN_OTHER;
}

static int s_depth;
static int s_deepest_at = -1;
static const void *s_deepest;
static const void *s_shown;
static kj_screen_state s_state;

void NitroPort_AppEnter(const void *init)
{
    if (s_depth == 0) {
        s_deepest = NULL;
        s_deepest_at = -1;
    }
    if (s_depth > s_deepest_at) {
        s_deepest_at = s_depth;
        s_deepest = init;
    }
    ++s_depth;
}

void NitroPort_AppLeave(void)
{
    if (s_depth > 0) {
        --s_depth;
    }
    if (s_depth != 0 || s_deepest == s_shown) {
        return;
    }
    s_shown = s_deepest;
    const uint32_t id = classify(s_shown);
    s_state.screen_id = id;
    s_state.depth = (uint32_t)(s_deepest_at < 0 ? 0 : s_deepest_at);
    kj_layouts_for(id, &s_state);
    ++s_state.generation;
    NitroPort_Log("[screen] %s (depth %lu, init %p, generation %lu, layout rows A %lu B %lu)",
                  NAMES[id], (unsigned long)s_state.depth, s_shown,
                  (unsigned long)s_state.generation, (unsigned long)s_state.row_count[0],
                  (unsigned long)s_state.row_count[1]);
}

void kj_screen_snapshot(kj_screen_state *out)
{
    *out = s_state;
}
