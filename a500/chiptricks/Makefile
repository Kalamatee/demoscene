SOURCES_C := \
	chiptricks.c \
	ct-gui.c \
	ct-load.c \
	ct-scope.c

DATA := 'data/01. Chip Shop (4-Mat) - Elektro.ahx' \
	'data/02. Cyber Ride (Jester) - KAM_.ahx' \
	'data/02. Cyber Ride (Jester) - KAM_.txt' \
	'data/03. Delicate 0ooz! (Emax) - KAM_.ahx' \
	'data/03. Delicate 0ooz! (Emax) - KAM_.txt' \
	'data/04. Enigma (Firefox & Tip) - Jazzcat.ahx' \
	'data/05. Gratitude (Dime) - Varthall.ahx' \
	'data/05. Gratitude (Dime) - Varthall.txt' \
	'data/06. Lotus III Ingame 5 (Patrick Phelan) - KAM_.ahx' \
	'data/06. Lotus III Ingame 5 (Patrick Phelan) - KAM_.txt' \
	'data/07. Madness2 - Part B (4-Mat) - Mice.ahx' \
	'data/07. Madness2 - Part B (4-Mat) - Mice.txt' \
	'data/08. Phantasmagoria (4-Mat) - Jazzcat.ahx' \
	'data/09. Pinball Illusions Title (Blaizer) - KAM_.ahx' \
	'data/09. Pinball Illusions Title (Blaizer) - KAM_.txt' \
	'data/10. Ride (XTD) - KAM_.thx' \
	'data/11. Shattere (Hollywood & Reverse) - Mice.ahx' \
	'data/11. Shattere (Hollywood & Reverse) - Mice.txt' \
	'data/12. Switchback (Chromag) - KAM_.ahx' \
	'data/12. Switchback (Chromag) - KAM_.txt' \
	'data/ahxWaves' \
	'data/ahxWaves.Location' \
	'data/ct-6x9.font' \
	'data/ct-all.json' \
	'data/ct_font.ilbm' \
	'data/ct_panel.ilbm' \
	'data/ct_pointer.ilbm' \
	'data/ct_track_active.ilbm' \
	'data/ct_track_hover.ilbm' \
	'data/ct_track_inactive.ilbm'

LIBS = libahx

all: build-data chiptricks.exe

include build.mk

archive:
	7z a "chiptricks-$$(date +%F-%H%M).7z" $< $(DATA)

clean:: clean-data

# vim: ts=8 sw=8 expandtab
