CFLAGS += -O2 -Wall -Wextra -Wpedantic -std=gnu11 #-ggdb3
CXXFLAGS += -O2 -Wall -Wextra -Wpedantic -std=gnu++11 #-ggdb3
LDFLAGS += -O2 #-ggdb3
LDLIBS := -lnetpbm -lm #-lefence
LD := $(CXX)

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
	examples/afghangirl.png \
	examples/bar.png \
	examples/birds.png \
	examples/coffee.png \
	examples/desert.png \
	examples/dsotm.png \
	examples/eits.png \
	examples/escher.png \
	examples/gc.png \
	examples/holi2.png \
	examples/lena.png \
	examples/monarch.png \
	examples/nd.png \
	examples/peppers.png \
	examples/raytrace.png \
	examples/rr.png \
	examples/starry-night.png \
	examples/starwars2.png \
	examples/starwars.png \
	examples/webb.png \

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
