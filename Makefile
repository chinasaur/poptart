OBJS=poptart.o
BIN=poptart

LDFLAGS+=-lfreetype -lz

VGFONT=-L/opt/vc/src/hello_pi/libs/vgfont -lvgfont 
VGFONTBUNDLED=-Lvgfont -lvgfont
VGFONTCSLI=-L../vgfontcsli -lvgfontcsli

CFLAGS+=-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi

LDFLAGS+=-L$(SDKSTAGE)/opt/vc/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -L/opt/vc/src/hello_pi/libs/ilclient 

# Didn't bother to change vgfont include as header hasn't changed.
INCLUDES+=-I$(SDKSTAGE)/opt/vc/include/ -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads -I$(SDKSTAGE)/opt/vc/include/interface/vmcs_host/linux -I./ -I/opt/vc/src/hello_pi/libs/ilclient -I/opt/vc/src/hello_pi/libs/vgfont

all: $(BIN) $(LIB)

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

vgfont/libvgfont.a:
	$(MAKE) -C vgfont

poptart: vgfont/libvgfont.a $(OBJS)
	$(CC) -o $(BIN) -Wl,--whole-archive $(OBJS) $(LDFLAGS) $(VGFONTBUNDLED) -Wl,--no-whole-archive -rdynamic

csli: $(OBJS)
	$(CC) -o $(BIN) -Wl,--whole-archive $(OBJS) $(LDFLAGS) $(VGFONTCSLI) -Wl,--no-whole-archive -rdynamic

%.a: $(OBJS)
	$(AR) r $@ $^

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f $(BIN) $(LIB)
	@rm -f vgfont/libvgfont.a
