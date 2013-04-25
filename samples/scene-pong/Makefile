APP = scene-pong

include $(SDK_DIR)/Makefile.defs

SCENE = ../../scene
SCENE_H = $(SCENE)/headers
SCENE_C = $(SCENE)/src

SHARED = ../../shared
SHARED_H = $(SHARED)/headers
SHARED_C = $(SHARED)/src

CFLAGS += -I$(SCENE_H) -I$(SHARED_H) -DSCENE_MAX_SIZE=256

OBJS = $(ASSETS).gen.o main.o $(SCENE_C)/scene.o $(SHARED_C)/propfont.o $(SHARED_C)/fontdata.o BallObj.o BatObj.o GameLogic.o public.o

include $(SDK_DIR)/Makefile.rules
