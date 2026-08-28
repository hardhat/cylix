; Export the symbols

  .module img
  .area _TEXT

	.globl _ship_palette
    .globl _ship_palette_len
	.globl _ship_tiles
    .globl _ship_tiles_len

_ship_palette:
    .incbin "img/ship.ztp"
_ship_palette_len:
    .dw .-_ship_palette
_ship_tiles:
    .dw _ship_tiles_data
_ship_tiles_data:
    .incbin "img/ship.zts"
_ship_tiles_len:
    .dw .-_ship_tiles_data
