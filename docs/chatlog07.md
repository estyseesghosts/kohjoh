## The hardware logs narrow this down considerably

The strongest finding is that the severe Goldenrod slowdown is CPU-side, not a PICA200 triangle-throughput problem.

In the new Old 3DS logs, the worst sustained windows show:

| Metric                           | O3DS debug build | O3DS normal build |
| -------------------------------- | ---------------- | ----------------- |
| Game logic, median               | 121.9 ms         | 143.9 ms          |
| 3D draw preparation, median      | 5.5 ms           | 5.1 ms            |
| 2D rendering, median             | 1.2 ms           | 1.2 ms            |
| GPU wait, median                 | 0.007 ms         | 0.007 ms          |
| Displayed-frame interval, median | 130 ms           | 154 ms            |

These measurements are from different passes through the scene, not perfectly matched A/B runs.

Crucially, the renderer reports GPU drawing averages around 3–4 ms per picture during the worst geometry-heavy sections. That leaves an enormous discrepancy: the GPU finishes its work quickly, while the game thread spends roughly 120–145 ms producing the next frame.

The slowdown also reproduces on a New 3DS held at O3DS clock speeds. This is not an isolated fault with one physical console.

We should stop prioritizing GPU throughput optimizations until this CPU-side cost is explained.

## The cache is not the obvious regression culprit

The logs contain two builds:

- `5026d7b-dirty` — ordinary build
- `8a92e93-dirty-prof` — later profiled build

Both report millions of display-list cache hits. So these are not cache-disabled versus cache-enabled runs, despite the different commit dates.

In the sustained 51,310-FIFO-word scene, the later O3DS build reports approximately 122 ms median logic time versus 144 ms in the earlier build. The runs aren't strictly controlled, but there's no evidence here that the cache is causing that catastrophic slowdown.

More importantly, even the lighter field sections are close to the deadline: one O3DS window at roughly 20,700 FIFO words per game frame has a 24.9 ms median logic stage, before 2D work, 3D submission and interruptions from audio.

That explains why a few extra milliseconds can now destabilize scenes that used to hold 30 FPS. We need to measure where those milliseconds went, rather than treating the entire renderer as equally suspect.

## Audio is less suspicious than it first appeared

The new `sndprof` counters let us compare mixer work directly.

At approximately 39 seconds into the O3DS runs:

- Earlier mixer: about 2.26 billion mixing ticks
- Optimized mixer: about 1.63 billion mixing ticks

The number of active channels is slightly different, but normalized cost also improves, from roughly 169 to 130 ticks per channel-sample.

That's a substantial hardware improvement. Audio still competes with the game on core 0, but the revised mixer is not the leading explanation for the new slowdown.

This leaves one disproportionately large, insufficiently measured stage: the DS geometry command processing that happens inside the `logic` timer, before Kohjoh enters its separately timed PICA200 draw path.

## What the remaining counters tell us

The sustained Goldenrod workload contains approximately:

| Per game frame or presented picture        | Measurement |
| ------------------------------------------ | ----------- |
| DS geometry FIFO calls/game frame          | 979         |
| FIFO words/game frame                      | 51,310      |
| DS geometry commands/game frame            | 6,885       |
| Triangles/presented picture                | 8,370       |
| Display-list words replayed from cache     | \~74%       |
| Words bypassing cache                      | \~20%       |
| Words needing re-decoding after key misses | \~5%        |

That is a substantial amount of geometry processing on the ARM11, even though the cached lists no longer require full decoding.

The source explains why: `dl_replay()` still executes state events, computes matrix references and copies vertices and indices into the current frame. Cache hits aren't free.

There's another useful negative result: during the worst O3DS windows, the renderer reports zero rebuilt 2D layers, with median 2D time around 1.2 ms. So Goldenrod's sustained slowdown is not the 2D retention problem we previously suspected.

## The two measurements we are missing

I would add only two coarse timing groups before doing any further optimization.

1\. Game-side geometry production

Measure the complete NitroSystem model/draw calls, excluding the time they spend inside `NitroPort_GxFifoWords`.

This determines whether Pokémon's own model traversal, animation, visibility handling and geometry generation are expensive.

2\. Kohjoh's geometry translation

Measure total time inside `NitroPort_GxFifoWords`, broken into cache lookup/comparison, replayed state commands, replayed vertex/index copying, and live decoding.

That gives us a decision tree:

| Result                              | Next optimization                                                        |
| ----------------------------------- | ------------------------------------------------------------------------ |
| Game-side model traversal dominates | Cache model traversal, transforms and geometry generation                |
| Live decoding dominates             | Improve DMA batching and eliminate uncached short/split lists            |
| Replay state commands dominate      | Retain model-level state and avoid executing redundant commands          |
| Vertex/index copying dominates      | Retain immutable GPU geometry                                            |
| Neither dominates                   | Profile game functions outside rendering, especially Pokémon data access |

Use sampled or aggregated timing, not one clock read per opcode. The `-prof` build already demonstrates millions of timed opcode calls and is unsuitable as the performance baseline.

## My revised priority

1. Do one clean O3DS cache-on/cache-off A/B, using the same commit and location. Both supplied builds had caching active, so this comparison is still missing.
2. Separate game-side geometry generation from Kohjoh's FIFO translation. This is the single most informative new measurement.
3. Investigate the approximately 20% uncached FIFO traffic, especially command streams split across calls.
4. Target whichever CPU stage actually dominates. Don't implement another GPU culling pass before measuring whether it can eliminate meaningful upstream CPU work.

One final distinction: the logs confirm a severe performance problem, but do not yet prove a specific recent code regression. The older stable field windows processed around 17,300 FIFO words per game frame; a newer problematic window processed around 20,700. Those aren't identical workloads.

The diagnosis is now much narrower: the PICA200 and retained 2D renderer appear to have sufficient headroom. Audio has improved. The remaining question is how much of the expensive DS geometry work occurs in the game itself versus Kohjoh's decoder and cache. One carefully instrumented O3DS run should answer that.
