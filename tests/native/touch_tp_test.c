/* Virtual touch panel pen-up semantics (issue #28).
 *
 * Compiles the real src/compat/nitro/src/port_tp.c against minimal SDK
 * stubs, then feeds a pen-down / drag / pen-up / re-down trace through the
 * same calibration and validity filter HGSS uses (NitroSDK
 * TP_GetCalibratedPoint, then touchpad.c TouchOutput_CollectValid). A
 * release must contribute no drawable point and must break stroke
 * continuity; a valid (0,0) pen-up sample is the false diagonal from the
 * issue.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nitro.h>
#include <nitro_port.h>
#include <nitro/spi/common/type.h>

static unsigned s_replies;

void NitroPort_Arm7Reply(int tag, u32 data, BOOL err) {
    (void)tag;
    (void)data;
    (void)err;
    ++s_replies;
}

static OS_SystemWork s_work;

OS_SystemWork *OS_GetSystemWork(void) {
    return &s_work;
}

/* The firmware calibration NitroPort_BootFirmware installs: two points at
 * (32,24) and (224,168), raw = screen * 16. TP_CalcCalibrateParam turns that
 * into dotSize 4096 and scale 2^28 / 4096. */
#define CAL_DOT_SIZE 4096
#define CAL_SCALE (0x10000000u / CAL_DOT_SIZE)

typedef struct Point {
    int x;
    int y;
    unsigned touch;
    unsigned validity;
} Point;

static SPITpData shared_sample(void) {
    SPITpData sample;
    memcpy(sample.bytes, (const void *)s_work.touch_panel, sizeof(sample.bytes));
    return sample;
}

/* TP_GetCalibratedPoint: touch and validity pass through; x/y are zero for
 * a released panel even when the validity bits say the coordinates are
 * usable. */
static Point calibrated(SPITpData raw) {
    Point point;
    point.touch = raw.e.touch;
    point.validity = raw.e.validity;
    if (!raw.e.touch) {
        point.x = 0;
        point.y = 0;
        return point;
    }
    point.x = (int)(((long long)(raw.e.x << 2) * CAL_SCALE) >> 22);
    point.y = (int)(((long long)(raw.e.y << 2) * CAL_SCALE) >> 22);
    return point;
}

/* touchpad.c TouchOutput_CollectValid: only validity selects a sample. */
static int collect(const Point *samples, int count, Point *out) {
    int n = 0;
    for (int i = 0; i < count; ++i) {
        if (samples[i].validity == 0) {
            out[n++] = samples[i];
        }
    }
    return n;
}

typedef struct Stroke {
    int count;
    Point points[2];
    int has_last;
    Point last;
} Stroke;

/* One GF_TouchpadUpdate + DrawStrokes: newStroke holds the collected
 * samples, and penPos carries the last drawn point across frames. A frame
 * with no collected samples clears penPos, which is what breaks continuity
 * between strokes. */
static void draw_frame(const Point *samples, int count, Stroke *pen) {
    Point points[2];
    int n = collect(samples, count, points);
    int has_last = pen->count != 0;
    Point last = pen->last;

    for (int j = 0; j < n; ++j) {
        if (j != 0 || has_last) {
            assert(!(points[j].x == 0 && points[j].y == 0));
            assert(abs(points[j].x - last.x) + abs(points[j].y - last.y) <= 80);
        }
        last = points[j];
    }
    if (n != 0) {
        pen->count = n;
        pen->last = last;
    } else {
        pen->count = 0;
    }
}

/* The same trace checker, but returning whether it saw the false origin
 * point. Used to prove the check would fail on the pre-fix encoding. */
static int trace_has_false_origin(const Point *samples, int count, Stroke *pen) {
    Point points[2];
    int n = collect(samples, count, points);
    int has_last = pen->count != 0;
    Point last = pen->last;
    int bad = 0;

    for (int j = 0; j < n; ++j) {
        if (j != 0 || has_last) {
            if (points[j].x == 0 && points[j].y == 0) {
                bad = 1;
            }
            if (abs(points[j].x - last.x) + abs(points[j].y - last.y) > 80) {
                bad = 1;
            }
        }
        last = points[j];
    }
    if (n != 0) {
        pen->count = n;
        pen->last = last;
    } else {
        pen->count = 0;
    }
    return bad;
}

static Point publish(BOOL down, unsigned x, unsigned y) {
    NitroPort_SetTouch(down, (u16)x, (u16)y);
    return calibrated(shared_sample());
}

int main(void) {
    SPITpData raw;
    Point sample;
    Stroke pen;

    memset(&s_work, 0, sizeof(s_work));
    NitroPort_ResetTouchState();

    /* Initial state: released and not usable as a coordinate. */
    raw = shared_sample();
    assert(raw.e.touch == SPI_TP_TOUCH_OFF);
    assert(raw.e.validity == SPI_TP_VALIDITY_INVALID_XY);
    assert(raw.e.x == 0 && raw.e.y == 0);
    printf("PASS release: touch=off validity=invalid x=0 y=0\n");

    /* Pen-down: valid coordinates at the raw scale, calibration restores the
     * screen position. */
    sample = publish(TRUE, 10, 20);
    assert(sample.touch == SPI_TP_TOUCH_ON);
    assert(sample.validity == SPI_TP_VALIDITY_VALID);
    assert(sample.x == 10 && sample.y == 20);
    raw = shared_sample();
    assert(raw.e.x == 10 * NITRO_PORT_TP_RAW_SCALE);
    assert(raw.e.y == 20 * NITRO_PORT_TP_RAW_SCALE);
    printf("PASS press: raw=%u,%u calibrated=%d,%d validity=valid\n", raw.e.x, raw.e.y, sample.x,
           sample.y);

    /* Release reports (0,0) with INVALID_XY, the ARM7's no-touch sample,
     * not (0,0) with VALID. */
    sample = publish(FALSE, 200, 150);
    assert(sample.touch == SPI_TP_TOUCH_OFF);
    assert(sample.validity == SPI_TP_VALIDITY_INVALID_XY);
    assert(sample.x == 0 && sample.y == 0);
    printf("PASS pen-up: touch=off validity=invalid, no drawable point\n");

    /* Clamping is unchanged. */
    sample = publish(TRUE, 300, 250);
    assert(sample.x == 255 && sample.y == 191);
    printf("PASS clamp: %d,%d\n", sample.x, sample.y);

    /* Full trace, two samples per game frame (GF_TouchpadStartSampling(1)
     * asks for two). The pen-up frames are published by the real driver. */
    {
        Point frames[5][2];
        Point a = {40, 60, 1, 0};
        Point a2 = {48, 60, 1, 0};
        Point b = {80, 60, 1, 0};
        Point c = {120, 60, 1, 0};
        Point c2 = {128, 60, 1, 0};

        frames[0][0] = publish(FALSE, 0, 0);
        frames[0][1] = publish(FALSE, 0, 0);
        frames[1][0] = a;
        frames[1][1] = a;
        frames[2][0] = a2;
        frames[2][1] = b;
        frames[3][0] = publish(FALSE, 0, 0);
        frames[3][1] = publish(FALSE, 0, 0);
        frames[4][0] = c;
        frames[4][1] = c2;

        memset(&pen, 0, sizeof(pen));
        for (int i = 0; i < 5; ++i) {
            draw_frame(frames[i], 2, &pen);
            if (i == 3) {
                assert(pen.count == 0); /* the release broke the stroke */
            }
        }
        assert(pen.count == 2);
        printf("PASS trace: pen-up frames collect 0 points; re-down does not join\n");
    }

    /* Mutation check: the old pen-up encoding (valid (0,0)) must fail the
     * same trace, so this test would catch a regression. */
    {
        Point frames[2][2];
        Point b = {80, 60, 1, 0};
        Point origin = {0, 0, 0, SPI_TP_VALIDITY_VALID};

        frames[0][0] = b;
        frames[0][1] = b;
        frames[1][0] = origin;
        frames[1][1] = origin;

        memset(&pen, 0, sizeof(pen));
        assert(trace_has_false_origin(frames[0], 2, &pen) == 0);
        assert(trace_has_false_origin(frames[1], 2, &pen) == 1);
        printf("PASS mutation: valid (0,0) pen-up is rejected by the trace check\n");
    }

    /* The auto-sampling path still publishes one packet per SetTouch. */
    {
        unsigned before = s_replies;
        NitroPort_HandleTp(
            0, SPI_PXI_START_BIT | SPI_PXI_END_BIT | (SPI_PXI_COMMAND_TP_AUTO_ON << 8), FALSE);
        assert(s_replies == before + 1);
        NitroPort_SetTouch(TRUE, 1, 2);
        assert(s_replies == before + 2);
        printf("PASS auto-sampling: one reply per published sample\n");
    }

    return 0;
}
