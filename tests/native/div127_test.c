/* div127 (port_sound_chip.c) against C division over its whole stated
 * range, |x| < 38,000,000; run by tests/native/sound_arm.py. */
#include <stdint.h>
#include <stdio.h>

static int32_t div127(int32_t x)
{
    return (int32_t)(((int64_t)x * 33818641) >> 32) - (x >> 31);
}

int main(void)
{
    int32_t x;

    for (x = -37999999; x < 38000000; ++x) {
        if (div127(x) != x / 127) {
            printf("FAIL div127(%ld) = %ld, expected %ld\n", (long)x, (long)div127(x),
                   (long)(x / 127));
            return 1;
        }
    }
    puts("PASS div127: |x| < 38,000,000");
    return 0;
}
