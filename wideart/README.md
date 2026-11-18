# wideart

Hand-drawn side bands for the 400-px top screen (issue #14). Each picture is
`<key>_<hofs>_<vofs>.png`, 320x192 RGBA: x 0..31 and 288..319 are the bands,
the middle is the DS picture (alpha 64 = "the game's own", left transparent).

To find a layer's name, add `debug: 0x80` to the card's `script.txt`; the log
then prints `[wide art] engine E bg B key <key>_<hofs>_<vofs>` for every
top-screen text BG as it is rebuilt.

The three pictures here are test placeholders for the opening intro shot
(frame ~1700), made by mirroring the DS picture's outer 32 columns of a LOTUS
capture. They prove the pipeline; they are not finished art.

Limits: a layer whose key changes every frame (palette or tile animation,
e.g. the title's sunburst) cannot be matched, and frames with per-line
register changes draw no bands.
