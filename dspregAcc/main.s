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

	.db "SS"
main:
	mov a,#$00
	mov SPC_PORT3,#$77
loop:
	cmp a,SPC_PORT0		; 3
	beq loop			; 2
	mov a,SPC_PORT0		; 3
	bmi toram			; 2
	mov x,SPC_PORT1		; 3
	mov SPC_REGADDR,x	; 4
	mov SPC_REGDATA,SPC_PORT2
	mov SPC_PORT0,a		; 4
	; wait 64 - 32 cycle
	cmp x,#$4c	; 3
	beq wait	; 4
	cmp x,#$5c	; 3
	bne loop	; 4
wait:
	mov y,#5	; 2
-
	dbnz y,-	; 4/6
	nop			; 2
	bra loop	; 4
toram:
	mov x,a
	mov y,#0
	mov a,SPC_PORT3
	mov [SPC_PORT1]+y,a
	mov a,x
	mov SPC_PORT0,a
	bra loop
	.db "EE"

.ends

.bank 1 slot 1
.section "SAMPLEDATA"

wavearea:
	.db $00
.ends
