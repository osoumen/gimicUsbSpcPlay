.ifndef _DEFINES_I_
.define _DEFINES_I_

;-------------------------------------------------
; bank define
;-------------------------------------------------

.memorymap
defaultslot 0
slotsize $200
slot 0 $0000		;bss
slotsize $10000-$200;srcdir+driver+data
slot 1 $200
.endme


.rombankmap
bankstotal 2
banksize $200
banks 1
banksize $10000-$200
banks 1
.endro

.define NULL 0

;-------------------------------------------------
; Struct define
;-------------------------------------------------
.struct SPC_PORT
PORT0	db
PORT1	db
PORT2	db
PORT3	db
.endst

.struct CHANNEL
CHANNEL1	db
CHANNEL2	db
CHANNEL3	db
CHANNEL4	db
CHANNEL5	db
CHANNEL6	db
CHANNEL7	db
CHANNEL8	db
.endst

.struct CHANNEL_W
CHANNEL1	dw
CHANNEL2	dw
CHANNEL3	dw
CHANNEL4	dw
CHANNEL5	dw
CHANNEL6	dw
CHANNEL7	dw
CHANNEL8	dw
.endst

;-------------------------------------------------
; spc dsp define
;-------------------------------------------------
.define DSP_VOL		$00
.define DSP_P		$02
.define DSP_SRCN	$04
.define DSP_ADSR	$05
.define DSP_GAIN	$07
.define DSP_ENVX	$08	; *
.define DSP_OUTX	$09	; *
.define DSP_MVOLL	$0c
.define DSP_MVOLR	$1c
.define DSP_EVOLL	$2c
.define DSP_EVOLR	$3c
.define DSP_KON		$4c
.define DSP_KOF		$5c
.define DSP_FLG		$6c
.define DSP_ENDX	$7c	; *
.define DSP_EFB		$0d
.define DSP_PMON	$2d
.define DSP_NON		$3d
.define DSP_EON		$4d
.define DSP_DIR		$5d
.define DSP_ESA		$6d
.define DSP_EDL		$7d
.define DSP_FIR		$0F

;-------------------------------------------------
; spc register define
;-------------------------------------------------
.define SPC_TEST	$f0
.define SPC_CONTROL	$f1
.define SPC_REGADDR	$f2
.define SPC_REGDATA	$f3
.define SPC_PORT0	$f4
.define SPC_PORT1	$f5
.define SPC_PORT2	$f6
.define SPC_PORT3	$f7
.define SPC_TIMER0	$FA
.define SPC_TIMER1	$FB
.define SPC_TIMER2	$FC
.define SPC_COUNTER0	$FD
.define SPC_COUNTER1	$FE
.define SPC_COUNTER2	$FF

;-------------------------------------------------
; spc flug define
;-------------------------------------------------
.define FLG_ECEN		$20
.define FLG_MUTE		$40
.define FLG_RES			$80

;-------------------------------------------------
; spc control flug define
;-------------------------------------------------
.define CNT_TIMER0_ON	%00000001
.define CNT_TIMER1_ON	%00000010
.define CNT_TIMER2_ON	%00000100
.define CNT_CLRPORT2	%00010000	; clear PORT0&1
.define CNT_CLRPORT1	%00100000	; clear PORT2&3
.define CNT_IPL			%10000000

.endif