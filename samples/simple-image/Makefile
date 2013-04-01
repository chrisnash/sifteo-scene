APP = simple-scene

include $(SDK_DIR)/Makefile.defs

SCENE = ../../scene
SCENE_H = $(SCENE)/headers
SCENE_C = $(SCENE)/src

CFLAGS += -I$(SCENE_H) -DSCENE_MAX_SIZE=256

OBJS = $(ASSETS).gen.o main.o $(SCENE_C)/scene.o

include $(SDK_DIR)/Makefile.rules
