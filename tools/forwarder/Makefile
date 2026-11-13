# kohjoh forwarder
DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
CC := $(DEVKITARM)/bin/arm-none-eabi-gcc
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
MAKEROM ?= makerom
BANNERTOOL ?= bannertool
EDITION ?= gold
ID_gold := 0xD7101
ID_silver := 0xD7102
CODE_gold := KJGH
CODE_silver := KJSH
OUT ?= ../../build/forwarder-$(EDITION)
all:
	mkdir -p $(OUT)
	$(CC) $(ARCH) -O2 -D__3DS__ -DEDITION=\"$(EDITION)\" -I$(DEVKITPRO)/libctru/include -specs=3dsx.specs -L$(DEVKITPRO)/libctru/lib main.c -lctru -lm -o $(OUT)/forwarder.elf
	$(BANNERTOOL) makebanner -i $(EDITION)-banner.png -a silence.wav -o $(OUT)/banner.bnr
	$(BANNERTOOL) makesmdh -s "kohjoh $(EDITION)" -l "kohjoh $(EDITION)" -p kohjoh -i $(EDITION)-icon.png -o $(OUT)/icon.smdh
	sed -e 's/0xD7101/$(ID_$(EDITION))/' -e 's/KJGH/$(CODE_$(EDITION))/' forwarder.rsf > $(OUT)/forwarder.rsf
	$(MAKEROM) -f cia -o $(OUT)/kohjoh-$(EDITION).cia -target t -exefslogo -rsf $(OUT)/forwarder.rsf -elf $(OUT)/forwarder.elf -banner $(OUT)/banner.bnr -icon $(OUT)/icon.smdh -major 0 -minor 0 -micro 1
.PHONY: all
