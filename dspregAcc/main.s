.include "defines.i"

.section "BANKHEADER" size 256

.db "SNES-SPC700 Sound File Data v0.30"
.db $26, $26
.db $26		; 26 = header contains ID666 information, 27 = header contains no ID666 tag
.db 30		; Version minor (i.e. 30)
.dw main	; PC
.db 0		; A
.db 0		; X
.db 0		; Y
.db $02		; PSW
.db $ef		; SP
.db 0,0		; reserved

.db "Song title                      "
.db "Game title                      "
.db "Name of dumper  "
.db "Comments                        "
.db "2014/05/28",0		; Date SPC was dumped (YYYY/MM/DD)
.db "120"				; Number of seconds to play song before fading out
.db "20000"				; Length of fade in milliseconds
.db "Artist of song                  "
.db 0					; Default channel disables (0 = enable, 1 = disable)
.db 0					; Emulator used to dump SPC: 0 = unknown, 1 = ZSNES, 2 = Snes9x
.dsb 45 0

.ends


.bank 1 slot 1
.section "SRCDIR" align 256

srcdir:
	.dw wavearea		;brrdata
	.dw wavearea+18		;looppoint
.ends

.bank 1 slot 1
.section "CODE"

	.db $cc
	.db $cc
main:
	mov SPC_PORT3,#$77
	mov a,#$00
loop:
	cmp a,SPC_PORT0
	beq +
	mov a,SPC_PORT0
	mov SPC_REGADDR,SPC_PORT1
	mov SPC_REGDATA,SPC_PORT2
	mov SPC_PORT0,a
+
	bra loop

	.db $ee
	.db $ee

.ends

.bank 1 slot 1
.section "SAMPLEDATA"

wavearea:
	.db $00
.ends
