APP = simple-scene

include $(SDK_DIR)/Makefile.defs

SCENE = ../../scene
SCENE_H = $(SCENE)/headers
SCENE_C = $(SCENE)/src

CFLAGS += -I$(SCENE_H) -DSCENE_MAX_SIZE=256

OBJS = main.o $(ASSETS).gen.o $(SCENE_C)/scene.o

include $(SDK_DIR)/Makefile.rules
