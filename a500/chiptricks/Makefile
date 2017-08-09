SOURCES_C := \
	chiptricks.c \
	ct-gui.c \
	ct-load.c \
	ct-scope.c

DATA := data/01-chipshop.ahx \
        data/02-cyberride.ahx \
        data/03-delicate.ahx \
        data/04-enigma.ahx \
        data/05-gratitude.ahx \
        data/06-lotus3.ahx \
        data/07-madness2.ahx \
        data/08-phantasmagoria.ahx \
        data/09-pinball.ahx \
        data/10-ride.thx \
        data/11-shattere.ahx \
        data/12-switchback.ahx \
        data/ahxWaves \
        data/ahxWaves.Location \
        data/ct-big.font \
        data/ct-modules.dat \
        data/ct-small.font \
        data/ct_big_font.ilbm \
        data/ct_panel.ilbm \
        data/ct_pointer.ilbm \
        data/ct_small_font.ilbm \
        data/ct_spans.ilbm \
        data/ct_track_active.ilbm \
        data/ct_track_hover.ilbm \
        data/ct_track_inactive.ilbm \
        data/ct_track_pause.ilbm \
        data/ct_track_pause_hover.ilbm \
        data/ct_track_play.ilbm \
        data/ct_track_play_hover.ilbm

LIBS = libahx

all: build-data chiptricks.exe

include build.mk

archive:
	7z a "chiptricks-$$(date +%F-%H%M).7z" chiptricks.exe $(DATA)

clean:: clean-data

# vim: ts=8 sw=8 expandtab
