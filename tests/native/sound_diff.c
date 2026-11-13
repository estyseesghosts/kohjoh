#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include SOURCE
static uint32_t rng = 42;
static uint32_t next(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
static uint64_t hash = 1469598103934665603ull;
static void digest(const void *p, size_t n) { const unsigned char *b=p; while(n--) hash=(hash ^ *b++)*1099511628211ull; }
int main(void) {
    uint8_t source[2052];
    for (unsigned test=0; test<100000; ++test) {
        NitroSoundVoice v = {0}; Channel c = {0};
        int32_t L[128]={0}, R[128]={0}, values[128]={0};
        for (unsigned i=0;i<sizeof(source);++i) source[i]=next();
        source[2] %= 89;
        c.v=&v; c.source=source; c.sourceAddr=0x1000; c.format=2;
        c.nSamples=4097; c.endByte=2052; c.loopAt=1+(next()%128);
        c.repeat=next()%3; c.interpolate=next()&1;
        c.volume=next()%128; c.pan=next()%128; c.shift=next()%5;
        c.step=((uint64_t)(next()%5)<<32)|next();
        if (test&1) c.step=(uint32_t)c.step;
        v.running=1; v.phase=next(); v.sampleIndex=0;
        adpcm_init(&v,source); v.adpcmClean=1; v.adpcmSource=c.sourceAddr;
        for (unsigned block=0;block<8;++block) {
            mix_block(&c,L,R,values,128);
            digest(L,sizeof(L)); digest(R,sizeof(R)); digest(values,sizeof(values)); digest(&v,sizeof(v));
        }
    }
    printf("%016llx\n",(unsigned long long)hash);
    return 0;
}
