/* Native tests for the field_scale parser (pixel-perfect.txt, Task 2).
 * Build: cc -I src tests/native/config_test.c src/app/config.c -o config_test
 * config.c's kj_log dependency is stubbed here so no 3DS service is needed. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app/config.h"

void kj_log(const char *fmt, ...)
{
    (void)fmt;
}

static void check(const char *text, size_t len, kj_config_status status,
                  kj_field_scale requested, kj_field_scale effective)
{
    kj_field_scale want = KJ_FIELD_SCALE_2X, use = KJ_FIELD_SCALE_2X;
    assert(kj_config_parse(text, len, &want, &use) == status);
    assert(want == requested);
    assert(use == effective);
}

#define CHECK(s, st, req, eff) check(s, strlen(s), st, req, eff)

int main(void)
{
    /* No file data, comments and whitespace only: safe legacy default. */
    CHECK("", KJ_CONFIG_UNRECOGNIZED, KJ_FIELD_SCALE_LEGACY, KJ_FIELD_SCALE_LEGACY);
    CHECK("# Overworld rendering resolution.\n# Valid values: legacy, 1x, 2x\n",
          KJ_CONFIG_UNRECOGNIZED, KJ_FIELD_SCALE_LEGACY, KJ_FIELD_SCALE_LEGACY);
    CHECK(" \t\r\n  \n", KJ_CONFIG_UNRECOGNIZED, KJ_FIELD_SCALE_LEGACY, KJ_FIELD_SCALE_LEGACY);
    CHECK("something_else = 1x\n", KJ_CONFIG_UNRECOGNIZED, KJ_FIELD_SCALE_LEGACY,
          KJ_FIELD_SCALE_LEGACY);

    /* The documented forms, with LF and CRLF. */
    CHECK("field_scale = 1x\n", KJ_CONFIG_OK, KJ_FIELD_SCALE_1X, KJ_FIELD_SCALE_1X);
    CHECK("field_scale = 2x\r\n", KJ_CONFIG_OK, KJ_FIELD_SCALE_2X, KJ_FIELD_SCALE_2X);
    CHECK("field_scale = legacy\n", KJ_CONFIG_OK, KJ_FIELD_SCALE_LEGACY,
          KJ_FIELD_SCALE_LEGACY);
    CHECK("field_scale=1x", KJ_CONFIG_OK, KJ_FIELD_SCALE_1X,
          KJ_FIELD_SCALE_1X); /* unterminated final line */

    /* Surrounding whitespace, inline comments and an unknown key. */
    CHECK("  field_scale\t=\t2x  # pixelated\n", KJ_CONFIG_OK, KJ_FIELD_SCALE_2X,
          KJ_FIELD_SCALE_2X);
    CHECK("other = nonsense\n  field_scale = 1x\n", KJ_CONFIG_OK, KJ_FIELD_SCALE_1X,
          KJ_FIELD_SCALE_1X);
    CHECK("field_scale = 1X\n", KJ_CONFIG_OK, KJ_FIELD_SCALE_1X, KJ_FIELD_SCALE_1X);

    /* Malformed/unrecognised values retain legacy without crashing, and do
     * not stop later valid lines from being read. */
    CHECK("field_scale = banana\n", KJ_CONFIG_UNRECOGNIZED, KJ_FIELD_SCALE_LEGACY,
          KJ_FIELD_SCALE_LEGACY);
    CHECK("field_scale\n", KJ_CONFIG_UNRECOGNIZED, KJ_FIELD_SCALE_LEGACY,
          KJ_FIELD_SCALE_LEGACY);
    CHECK("field_scale = \n", KJ_CONFIG_UNRECOGNIZED, KJ_FIELD_SCALE_LEGACY,
          KJ_FIELD_SCALE_LEGACY);
    CHECK("field_scale = nope\nfield_scale = 2x\n", KJ_CONFIG_OK, KJ_FIELD_SCALE_2X,
          KJ_FIELD_SCALE_2X);
    CHECK("field_scale = 2x\nfield_scale = bogus\n", KJ_CONFIG_OK, KJ_FIELD_SCALE_2X,
          KJ_FIELD_SCALE_2X);
    CHECK("field_scale = 2x\nfield_scale = legacy\n", KJ_CONFIG_OK, KJ_FIELD_SCALE_LEGACY,
          KJ_FIELD_SCALE_LEGACY);

    /* Length-delimited input: bytes after `length` must be ignored. */
    {
        const char raw[] = "field_scale = 1xGARBAGE";
        kj_field_scale want, use;
        assert(kj_config_parse(raw, 16, &want, &use) == KJ_CONFIG_OK);
        assert(want == KJ_FIELD_SCALE_1X && use == KJ_FIELD_SCALE_1X);
        assert(kj_config_parse(NULL, 0, &want, &use) == KJ_CONFIG_UNRECOGNIZED);
        assert(want == KJ_FIELD_SCALE_LEGACY && use == KJ_FIELD_SCALE_LEGACY);
    }

    /* Names and the global effective mode. */
    assert(strcmp(kj_config_scale_name(KJ_FIELD_SCALE_LEGACY), "legacy") == 0);
    assert(strcmp(kj_config_scale_name(KJ_FIELD_SCALE_1X), "1x") == 0);
    assert(strcmp(kj_config_scale_name(KJ_FIELD_SCALE_2X), "2x") == 0);
    {
        kj_field_scale want, use;
        assert(kj_config_load("no/such/config.txt", &want, &use) == KJ_CONFIG_MISSING);
        assert(want == KJ_FIELD_SCALE_LEGACY && use == KJ_FIELD_SCALE_LEGACY);
        assert(kj_config_field_scale() == KJ_FIELD_SCALE_LEGACY);
        assert(!kj_config_requires_offscreen());
    }

    /* text_renderer is an independent key with its own validation. */
    {
        kj_text_renderer want, use;

        assert(kj_config_parse_text("text_renderer = native\n", 22, &want, &use) ==
               KJ_CONFIG_OK);
        assert(want == KJ_TEXT_RENDERER_NATIVE && use == KJ_TEXT_RENDERER_NATIVE);
        assert(kj_config_parse_text("text_renderer = legacy\r\n", 23, &want, &use) ==
               KJ_CONFIG_OK);
        assert(want == KJ_TEXT_RENDERER_LEGACY && use == KJ_TEXT_RENDERER_LEGACY);
        assert(kj_config_parse_text("  text_renderer\t=\tNATIVE # opt in\n", 32, &want,
                                    &use) == KJ_CONFIG_OK);
        assert(use == KJ_TEXT_RENDERER_NATIVE);

        /* A malformed text_renderer does not invalidate field_scale... */
        {
            const char both[] = "text_renderer = banana\nfield_scale = 1x\n";
            kj_field_scale fw, fu;
            assert(kj_config_parse(both, sizeof(both) - 1, &fw, &fu) == KJ_CONFIG_OK);
            assert(fu == KJ_FIELD_SCALE_1X);
            assert(kj_config_parse_text(both, sizeof(both) - 1, &want, &use) ==
                   KJ_CONFIG_UNRECOGNIZED);
            assert(use == KJ_TEXT_RENDERER_LEGACY);
        }
        /* ...and an unknown key does not change it either. */
        assert(kj_config_parse_text("field_scale = 2x\n", 17, &want, &use) ==
               KJ_CONFIG_UNRECOGNIZED);
        assert(use == KJ_TEXT_RENDERER_LEGACY);
        /* Last valid entry wins, like field_scale. */
        {
            const char last[] = "text_renderer = native\ntext_renderer = legacy\n";
            assert(kj_config_parse_text(last, sizeof(last) - 1, &want, &use) == KJ_CONFIG_OK);
        }
        assert(use == KJ_TEXT_RENDERER_LEGACY);

        assert(strcmp(kj_config_text_name(KJ_TEXT_RENDERER_LEGACY), "legacy") == 0);
        assert(strcmp(kj_config_text_name(KJ_TEXT_RENDERER_NATIVE), "native") == 0);
        assert(kj_config_text_renderer() == KJ_TEXT_RENDERER_LEGACY);
        assert(!kj_config_native_text_requested());
    }

    puts("config_test: PASS");
    return 0;
}
