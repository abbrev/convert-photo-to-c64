CFLAGS += -O2 -Wall #-ggdb3
LDFLAGS += -O2 #-ggdb3
LDLIBS := -lnetpbm -lm #-lefence
LD := $(CC)

all: convert-photo-to-c64

convert-photo-to-c64: convert-photo-to-c64.o generate.o evaluate.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

#.o.c:

#%.o: %.c

.PHONY: clean
clean:
	rm -f *.o
	rm -f convert-photo-to-c64

IMAGES = \
	afghangirl.png \
	bar.png \
	birds.png \
	coffee.png \
	desert.png \
	dsotm.png \
	eits.png \
	escher.png \
	gc.png \
	holi2.png \
	lena.png \
	monarch.png \
	nd.png \
	peppers.png \
	raytrace.png \
	rr.png \
	starry-night.png \
	starwars2.png \
	starwars.png \
	webb.png \

#MULTICOLOR_IMAGES = $(IMAGES:.png=.multicolor.png)
MULTICOLOR_IMAGES = $(patsubst %.png,%.multicolor.png,$(IMAGES))
#HIRES_IMAGES = $(IMAGES:.png=.hires.png)
HIRES_IMAGES = $(patsubst %.png,%.hires.png,$(IMAGES))

c64images: $(MULTICOLOR_IMAGES) $(HIRES_IMAGES)

#.PHONY: $(MULTICOLOR_IMAGES) $(HIRES_IMAGES)

.PHONY: FORCE

%.hires.png: %.png FORCE
	pngtopnm <$<                        | ./convert-photo-to-c64 -m hires -p colodore                                  | pnmtopng >$@

%.multicolor.png: %.png FORCE
	pngtopnm <$< | pnmscale -xscale 0.5 | ./convert-photo-to-c64 -m multicolor -p colodore | pnmscale -xscale 2 -nomix | pnmtopng >$@
