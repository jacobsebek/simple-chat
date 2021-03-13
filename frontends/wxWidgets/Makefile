EXEC=./chat
VPATH=src

CXXFLAGS=-Wall -Wextra -pedantic -Iinclude -I../../client/include -I../../server/include -I$(WXW_PATH)/include -I$(WXW_PATH)/lib/gcc810_x64_dll/mswu -DwxDEBUG_LEVEL=0 -mwindows
LDFLAGS=-L../../client -L$(WXW_PATH)/lib/gcc810_x64_dll
LDLIBS= -lclient -lwxbase31u -lwxmsw31u_core

OBJECTS=$(patsubst %.cpp, %.o, $(notdir $(wildcard $(VPATH)/*.cpp)))
WINRES=rds.coff

$(EXEC) : $(OBJECTS) $(WINRES)
	${CXX} -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

%.o : include/*.h

%.coff : 
	windres -i $(patsubst %.coff, %.rc, $@) -o $@ -I$(WXW_PATH)/include
