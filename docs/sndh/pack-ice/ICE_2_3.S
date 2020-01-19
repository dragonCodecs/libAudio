;**************************************************************************;
;	  Pack-ice, a great packer written by Axe of Delight!	     ;
;**************************************************************************;

; PACK-ICE is simply the best packer that's available on the ST.
; I used especially much time to get the depacking-routine as small and
; fast (!!!) as possible.
; Assemble this source with Devpac 2 and set TAB-position to 10.
; An update of this version is planned and will be finished at the
; beginning of march.

	opt	o+,ow-,a+
begin:	bra	do_begin
	dc.b	'Pack-Ice: The best packer on the ST. '
	dc.b	'It is Public Domain. You may use it for packing '
	dc.b	'anything you like. I don''t want any donations. '
	dc.b	'Greetings go to all people who use this packer. '
	dc.b	0
	even
do_begin: movea.l	sp,a5		; adressiere basepage
	movea.l	4(a5),a5		;
	move.l	12(a5),d0 	; bestimme zu reservierende
	add.l	20(a5),d0 	; programmlaenge
	add.l	28(a5),d0 	;
	addi.l	#$0200,d0 	; + basepagelaenge
	move.l	d0,-(sp)		; reserviere speicher ab der basepage
	move.l	a5,-(sp)		;
	clr.w	-(sp)		;
	move.w	#$4A,-(sp)	;
	trap	#1		; setblock
	adda.w	#$0C,sp		;
	move.w	do_reloc1(pc),save1 ; installiere orginalcode
	move.w	do_reloc2(pc),save2 ;
	bsr	init_aes
	bsr	maus_aus
	bsr	test_alternate
	bsr	main
	bsr	maus_an_pfeil
	bsr	exit_aes
	bra.s	_pterm

	include	d:\sources\aes_equ.s
	include	d:\sources\aes_text.s



test_alternate:
	moveq	#0,d4
	bsr	check_alt
	bne.s	l000b
	moveq	#1,d4
l000b:	move.w	d4,mausflag
	rts


; Hier beginnt das Hauptprogramm

main:	lea	copyright(pc),a0
	bsr	print
data_exec:
	tst.w	mausflag
	beq.s	nomaus1
	lea	aes_askexec(pc),a0
	bsr	alertbox
	addi.w	#$30,d0
	bra.s	onmaus1
nomaus1:	lea	askexec(pc),a0
	bsr	print
waitkey:	bsr	conin
	cmpi.b	#$33,d0
	bne.s	no_setup
	lea	crlf(pc),a0
	bsr	print
	bsr	setup
	lea	crlf(pc),a0
	bsr	print
	bra.s	data_exec
onmaus1:	cmpi.b	#$33,d0
	bne.s	no_setup
	bsr	setup
	bra.s	data_exec
no_setup: move.b	#1,dataflag
	cmpi.b	#'1',d0
	beq.s	picture
	cmpi.b	#$0d,d0
	beq.s	picture
	cmpi.b	#'2',d0
	bne.s	waitkey
	clr.b	dataflag
relocate: tst.w	mausflag
	beq.s	nomaus2
	lea	aes_askreloc(pc),a0
	bsr	alertbox
	addi.w	#$6c,d0
	cmp.w	#'n',d0
	beq.s	reloc_inst
	moveq	#'y',d0
	bra.s	reloc_inst
nomaus2:	lea	ask_relocation(pc),a0
	bsr	print
	bsr	conin
reloc_inst:
	move.w	save1(pc),do_reloc1
	move.w	save2(pc),do_reloc2
	cmpi.b	#$0d,d0
	beq.s	set_verify
	ori.b	#$20,d0
	cmpi.b	#'y',d0
	beq.s	set_verify
	cmpi.b	#'n',d0
	bne.s	relocate
	move.w	#$6000+norel1b-norel1a-2,do_reloc1
	move.w	#$6000+norel2b-norel2a-2,do_reloc2
	bra.s	set_verify
picture:	clr.w	picturerequest
	tst.w	mausflag
	beq.s	nomaus3
	lea	aes_ask_picture(pc),a0
	bsr	alertbox
	cmp.w	#1,d0
	beq.s	set_verify
	bra.s	picture_yes
nomaus3:	lea	ask_picture(pc),a0
	bsr	print
	bsr	conin
	cmp.b	#'y',d0
	bne.s	set_verify
picture_yes:
	move.w	#1,picturerequest
set_verify:
	moveq	#-1,d3
	move.w	d3,verify
	moveq	#0,d3
	move.w	d0,notab_request
	tst.w	mausflag
	beq	nomaus4
	move.w	#$19,-(sp)
	trap	#1
	addq.l	#2,sp
	lea	fselect_path(pc),a4
	move.b	#'A',(a4)
	add.b	d0,(a4)+
	move.b	#':',(a4)+
	clr.w	-(sp)
	pea	(a4)
	move.w	#$47,-(sp)
	trap	#1
	addq.l	#8,sp
end_of_path:
	tst.b	(a4)+
	bne.s	end_of_path
	move.b	#'\',-1(a4)
	move.b	#'*',(a4)+
	move.b	#'.',(a4)+
	move.b	#'*',(a4)+
	clr.b	(a4)+
	bsr	maus_an_pfeil
	lea	addr_in(pc),a4
	move.l	#fsel_input,d0
	move.l	#fselect_path,(a4)+
	move.l	#fselect_name,(a4)+
	bsr	_aes
	tst.w	d0
	beq	fselect_abbruch
	move.l	int_out(pc),d4
	bsr	maus_aus
	lea	copyright(pc),a0
	bsr	print
	tst.w	d4
	beq	fselect_abbruch
	lea	fselect_path(pc),a2
path_end: tst.b	(a2)+
	bne.s	path_end
cut_mask: cmpi.b	#'\',-(a2)
	beq.s	path_done
	cmpi.b	#':',(a2)
	bne.s	cut_mask
	addq.l	#1,a2
	move.b	#'\',(a2)
path_done:clr.b	1(a2)
	lea	fselect_path(pc),a4
	cmpi.b	#':',1(a4)
	bne.s	no_setdrive
	moveq	#$0e,d0
	swap	d0
	move.b	(a4),d0
	ori.b	#$20,d0
	subi.b	#'a',d0
	move.l	d0,-(sp)
	trap	#1
	addq.l	#4,sp
	addq.l	#2,a4
no_setdrive:
	pea	(a4)
	move.w	#$3b,-(sp)
	trap	#1
	addq.l	#6,sp
	tst.l	d0
	bmi	fselect_abbruch
	lea	fselect_name(pc),a2
	tst.b	(a2)
	bne.s	name_found
	move.l	#$2a2e2a00,(a2)	; "*.*"
	bra.s	name_found
nomaus4:	lea	ask_sourcefile(pc),a0
	bsr	print
	pea	filename_input(pc)
	move.w	#$0a,-(sp)
	trap	#1
	addq.l	#6,sp
	lea	fselect_name(pc),a1
	adda.w	d0,a1
	clr.b	(a1)
	lea	crlf(pc),a0
	bsr	print
name_found:
	pea	dta_puffer(pc)
	move.w	#$1a,-(sp)
	trap	#1
	addq.l	#6,sp
	move.w	#0,-(sp)
	pea	fselect_name(pc)
	move.w	#$4e,-(sp)
	trap	#1
	addq.l	#8,sp
	tst.l	d0
	bmi	fselect_abbruch
fsnext:	
	bsr	reserve_table
	move.l	filesize(pc),d3
	addi.l	#$8000+1200+20,d3
	andi.w	#-2,d3
	move.l	d3,-(sp)
	move.w	#$48,-(sp)
	trap	#1
	addq.l	#6,sp
	tst.l	d0
	bpl.s	good_memory
	bsr	release_table
	move.l	filesize(pc),d0
	addi.l	#1200+20,d3
	andi.w	#-2,d3
	move.l	d3,-(sp)
	move.w	#$48,-(sp)
	trap	#1
	addq.l	#6,sp
	tst.l	d0
	bmi	nomemory
good_memory:
	addq.l	#1,d0
	andi.w	#-2,d0
	move.l	d0,packed_data
	move.l	d0,a0
	addi.l	#1200,d0
	move.l	d0,src_start
	move.l	#$affeaffe,d0	; Speicher mit Kennung fÅllen
	lsr.l	#3,d3
prepmem:	move.l	d0,(a0)+
	move.l	d0,(a0)+
	subq.l	#1,d3
	bne.s	prepmem
	bsr	fopen
	bmi	main_end
	move.l	src_start(pc),a0
	move.l	filesize(pc),d0
	bsr	fread
	bmi	main_end
	bsr	fclose
	move.l	filesize(pc),d1
	add.l	src_start(pc),d1
	move.l	d1,src_ende
	lea	working(pc),a0
	bsr	print
	tst.w	picturerequest
	beq.s	nopic
	lea	picfound_text(pc),a0
	bsr	print
nopic:	lea	filename(pc),a0
	bsr	print
	clr.w	interrupt
	bsr	gettime
	bsr	merge_planes
	bsr	crunch		; Packen
	movea.l	d0,a3		; neue LÑnge
	tst.l	d0
	bmi	main_end
	bsr	printtime
	tst.w	interrupt
	bne	interrupted
	tst.w	error
	beq.s	no_error
	lea	pack_error(pc),a0
	bsr	print
	bra	done
no_error: lea	(a3),a1
	tst.b	dataflag
	bne.s	nurdata
	adda.w	#ende_header-header,a1
nurdata:	move.l	a1,newlength
	movea.l	filesize(pc),a2
	cmpa.l	a2,a1
	bge	toolarge
	tst.w	verify
	beq	no_verify

	lea	verifytext(pc),a0	; Verify durchfÅhren
	bsr	print
	move.l	filesize(pc),d1	; OriginallÑnge
	addi.l	#11000,d1		; und 10000 Bytes Readpuffer
	move.l	d1,-(sp)		; reservieren
	move.w	#$48,-(sp)
	trap	#1
	addq.l	#6,sp
	tst.l	d0
	bpl.s	memo_ok
	lea	nomem_verify(pc),a0
	bsr	print
	bra	no_verify
memo_ok:	movea.l	d0,a4
	movea.l	d0,a2
	move.l	d0,mallocadr
	move.l	a3,d1
	subq.l	#1,d1
	movea.l	packed_data(pc),a1
copyblk:	move.b	(a1)+,(a2)+
	dbra	d1,copyblk
	subi.l	#$010000,d1
	bpl.s	copyblk
	movea.l	a4,a0
	adda.w	#120,a4
	move.l	a4,a1
	bsr	decrunch
	lea	100(a4),a2
	adda.l	filesize(pc),a2
	move.l	a2,d0
	andi.w	#-2,d0
	move.l	d0,readpuffer
	bsr	fopen
readon:	movea.l	readpuffer(pc),a0
	move.l	#10000,d0
	bsr	fread
	move.l	d0,d4
	beq.s	doclose
	cmpi.l	#10000,d0
	bcc.s	noclose
doclose:	bsr	fclose
	move.l	d4,d0
noclose:	movea.l	readpuffer(pc),a2
	subq.w	#1,d0
	bmi.s	copyok
compare:	move.b	(a2)+,d1
	move.b	(a4)+,d2
	cmp.b	d1,d2
	bne.s	packerror
	dbra	d0,compare
copyok:	cmpi.l	#10000,d4
	beq.s	readon
	lea	verify_ok(pc),a0
	bsr	print
	bsr	memfree
	bra.s	no_verify
packerror:
	lea	verify_error(pc),a0
	bsr	print
	bsr	memfree
	bra	done
memfree:	move.l	mallocadr(pc),-(sp)
	move.w	#$49,-(sp)
	trap	#1
	addq.l	#6,sp
	rts
no_verify:
	bsr	calc_packingrate
	cmpi.b	#1,dataflag
	beq.s	onlydata
	movea.w	#ende_header-header,a4
	movea.l	a3,a5
	lea	-$1c(a5,a4.w),a5
	move.l	a5,d0
	addq.l	#1,d0
	andi.w	#-2,d0
	move.l	d0,textlength
	move.l	a3,d0		; gepackte LÑnge
	moveq	#120,d1		; - 120
	sub.l	d1,d0
	move.l	d0,reg1+2
	move.l	filesize,reg2+2
	bsr	fcreate
	bmi	main_end
	lea	header(pc),a5
	movea.w	#ende_header-header,a6
	bsr	fwrite
	bmi	main_end
	movea.l	packed_data(pc),a5
	move.l	a3,d0
	addq.l	#1,d0
	andi.w	#-2,d0
	movea.l	d0,a6
	bsr	fwrite
	bmi	main_end
	lea	nullbytes(pc),a5
	clr.l	(a5)
	movea.w	#4,a6
	bsr	fwrite
	bmi	main_end
	bsr	fclose
	bmi	main_end
	bra.s	done
onlydata: bsr	fcreate
	movea.l	packed_data(pc),a5
	movea.l	a3,a6
	bsr	fwrite
	bmi	main_end
	bsr	fclose
	bra.s	done
interrupted:
	lea	alt_pressed(pc),a0
	bsr	print
	bra.s	done
toolarge: lea	uneffectiv(pc),a0
	bsr	print
done:	move.l	packed_data(pc),-(sp)
	move.w	#$49,-(sp)
	trap	#1
	addq.l	#6,sp
	tst.l	d0
	bne.s	main_end
	bsr	release_table
	move.w	#$4f,-(sp)
	trap	#1
	addq.l	#2,sp
	tst.l	d0
	bpl	fsnext
fselect_abbruch:
	tst.w	mausflag
	beq.s	nomaus5
	lea	aes_askanother(pc),a0
	bsr	alertbox
	cmp.w	#1,d0
	beq	main
	cmp.w	#2,d0
	beq.s	main_end
nomaus5:	lea	crlf(pc),a0
	bsr	print
	lea	crunch_another(pc),a0
	bsr	print
	bsr	conin
	ori.b	#$20,d0
	cmpi.b	#'n',d0
	beq.s	main_end
	bra	main
nomemory: lea	nomem_text(pc),a0
	bsr	print
	bsr	cconin
main_end: rts

fopen:	clr.w	-(sp)		; nur lesen
	pea	filename(pc)
	move.w	#$3d,-(sp)
	trap	#1
	addq.l	#8,sp
	move.w	d0,handle
	tst.l	d0
	rts
fread:	pea	(a0)
	move.l	d0,-(sp)
	move.w	handle(pc),-(sp)
	move.w	#$3f,-(sp)
	trap	#1
	adda.w	#$c,sp
	tst.l	d0
	rts
fcreate:	move.w	#0,-(sp)
	pea	filename(pc)
	move.w	#$3c,-(sp)
	trap	#1
	addq.l	#8,sp
	move.w	d0,handle
	tst.l	d0
	rts
fwrite:	move.l	a5,-(sp)
	move.l	a6,-(sp)
	move.w	handle(pc),-(sp)
	move.w	#$40,-(sp)
	trap	#1
	lea	12(sp),sp
	tst.l	d0
	rts
fclose:	move.w	handle(pc),-(sp)
	move.w	#$3e,-(sp)
	trap	#1
	addq.l	#4,sp
	tst.l	d0
	rts
conin:	move.w	#1,-(sp)
	trap	#1
	addq.l	#2,sp
	rts
cconin:	move.w	#7,-(sp)
	trap	#1
	addq.l	#2,sp
	rts
check_alt:move.l	#$0000bffff,-(sp)
	trap	#$d
	addq.l	#4,sp
	btst	#3,d0
	rts
print:	move.l	a0,-(sp)
	move.w	#9,-(sp)
	trap	#1
	addq.l	#6,sp
	rts
gettime:	pea	read_4ba(pc)
	move.w	#$26,-(sp)
	trap	#14
	addq.l	#6,sp
	rts
read_4ba: move.l	$4ba.w,d0
	sub.l	oldtime(pc),d0
	move.l	d0,newtime
	move.l	$4ba.w,oldtime
	rts
printtime:movem.l	d0-a6,-(sp)
	lea	timetext(pc),a0
	bsr.s	print
	bsr.s	gettime
	move.l	newtime(pc),d0
	divu	#200,d0		; weil 200 Hz Timer
	swap	d0
	clr.w	d0
	swap	d0
	divu	#60,d0		; 1 Minute: 60 Sekunden
	moveq	#0,d1
	move.w	d0,d1
	swap	d0
	moveq	#0,d2
	move.w	d0,d2
	lea	ascii_time(pc),a0
	clr.l	(a0)
	clr.l	2(a0)
l0042:	divu	#10,d1
	swap	d1
	addi.w	#'0',d1
	move.w	d1,(a0)+
	clr.w	d1
	swap	d1
	tst.w	d1
	bne.s	l0042
	move.l	#$200020,(a0)+
	lea	l0044(pc),a0
	move.w	-(a0),d0
	bsr	conout
	move.w	-(a0),d0
	bsr	conout
	move.w	-(a0),d0
	bsr	conout
	moveq	#':',d0
	bsr	conout
	move.l	d2,d0
	divu	#10,d0
	addi.w	#'0',d0
	bsr	conout
	swap	d0
	addi.w	#'0',d0
	bsr	conout
	movem.l	(sp)+,d0-a6
	rts
ascii_time:	ds.w 3
l0044:		ds.w 5
setup:	lea	ascii_offset(pc),a1
	move.l	optimize(pc),d0
	moveq	#3,d2
l0046:	moveq	#$0f,d1
	and.l	d0,d1
	cmp.w	#9,d1
	ble.s	l0047
	addi.w	#$27,d1
l0047:	addi.w	#$30,d1
	move.b	d1,-(a1)
	lsr.l	#4,d0
	dbra	d2,l0046
	lea	ask_offset(pc),a0
	bsr	print
	lea	l00d5(pc),a3
	move.b	#4,(a3)
	pea	(a3)
	move.w	#10,-(sp)
	trap	#1
	addq.l	#6,sp
	moveq	#0,d2
	move.b	1(a3),d2
	beq.s	l004d
	adda.l	d2,a3
	addq.l	#2,a3
	moveq	#0,d4
	moveq	#0,d3
	subq.w	#1,d2
	bmi.s	setup
l0048:	moveq	#0,d0
	move.b	-(a3),d0
	subi.b	#'0',d0
	bmi.s	l0049
	cmpi.b	#9,d0
	ble.s	l004a
	addi.b	#$30,d0
	ori.b	#$20,d0
	subi.b	#$57,d0
	cmpi.b	#9,d0
	ble.s	l0049
	cmpi.b	#$0f,d0
	ble.s	l004a
l0049:	moveq	#0,d0
l004a:	lsl.w	d3,d0
	add.l	d0,d4
	addq.w	#4,d3
	dbra	d2,l0048
	move.l	#$1528,d0		; hîchster Offset
	cmp.l	d0,d4
	bcs.s	l004b
	move.l	d0,d4
l004b:	cmp.w	#5,d4		; kleinster Offset
	bcc.s	l004c
	moveq	#5,d4
l004c:	move.l	d4,optimize
l004d:	rts

countdown_init:
	move.l	d0,-(sp)
	move.l	filesize(pc),d0		; gesamte FilelÑnge
	lsr.l	#7,d0			; in 128 StÅcke teilen
	move.l	d0,count_differenz		; Bei jedem StÅck werden
	move.l	src_start(pc),count_start 	; die Prozent angezeigt!
	move.w	#-1,oldcountdown
	move.l	(sp)+,d0
	rts

countdown:
	cmpa.l	count_start(pc),a0
	bcs	no_countdown
	movem.l	d0-a6,-(sp)
	move.l	count_differenz(pc),d0
	add.l	d0,count_start
	move.l	a0,d1
	move.l	src_ende(pc),d0
	move.l	src_start(pc),d2
	sub.l	d2,d1		; d1 = Offset in ungepackten Daten
	sub.l	d2,d0		; d0 = LÑnge der ungepackten Daten
	bsr	adjust_d0_d1
	mulu	#100,d1
	bsr	adjust_d0_d1
	tst.w	d0
	beq.s	endcountdown
	divu	d0,d1
	swap	d1
	clr.w	d1
	swap	d1
	cmp.w	oldcountdown(pc),d1
	beq.s	endcountdown
	move.w	d1,oldcountdown
	divu	#10,d1
	move.l	#$390039,d0
	sub.l	d1,d0
	lea	ascii_prozent(pc),a0
	move.b	d0,(a0)+
	swap	d0
	move.b	d0,(a0)+
	lea	remaining(pc),a0
	bsr	print
	bsr	check_alt
	beq.s	endcountdown
release:	bsr	check_alt
	bne.s	release
	move.w	#-1,interrupt
	movem.l	(sp)+,d0-a6
	move.l	a0,src_ende
	bra.s	no_countdown
endcountdown:
	movem.l	(sp)+,d0-a6
no_countdown:
	rts

count_start:	ds.l 1
count_differenz:	ds.l 1
oldcountdown:	ds.w 1



calc_packingrate:
	lea	packingrate(pc),a0
	bsr	print
	move.l	filesize(pc),d0
	move.l	newlength(pc),d1
	bsr	adjust_d0_d1
	mulu	#100,d1
	bsr	adjust_d0_d1
	divu	d0,d1
	move.l	d1,d0
	swap	d0
	clr.w	d0
	swap	d0
	divu	#10,d0
	addi.l	#$300030,d0
	bsr	conout
	swap	d0
	bsr	conout
	lea	saving(pc),a0
	bsr	print
	rts

adjust_d0_d1:
	swap	d0
	swap	d1
	tst.w	d0
	bne.s	rshift
	tst.w	d1
	bne.s	rshift
	swap	d0
	swap	d1
	rts

rshift:	swap	d0
	swap	d1
	lsr.l	#1,d0
	lsr.l	#1,d1
	bra.s	adjust_d0_d1

conout:	movem.l	d0-d3/a0-a3,-(sp)
	move.w	d0,-(sp)
	move.w	#2,-(sp)
	trap	#1
	addq.l	#4,sp
	movem.l	(sp)+,d0-d3/a0-a3
	rts

alertbox: bsr	maus_an_pfeil
	move.l	#form_alert,d0
	move.w	#1,int_in
	move.l	a0,addr_in
	bsr	_aes
	move.w	int_out(pc),-(sp)
	bsr	maus_aus
	moveq	#0,d0
	move.w	(sp)+,d0
	rts

maus_an_pfeil:
	move.l	#graf_mouse,d0
	move.w	#$0101,int_in
	bsr	_aes
	move.l	#graf_mouse,d0
	move.w	#0,int_in
	bsr	_aes
	rts

maus_aus: move.l	#graf_mouse,d0
	move.w	#$0100,int_in
	bsr	_aes
	rts

merge_planes:
	clr.w	pictureflag
	tst.w	picturerequest
	beq.s	no_merge_planes
	movea.l	src_start(pc),a1
	move.l	filesize(pc),d1
	move.l	#$7d00,d0
	sub.l	d0,d1
	bmi.s	l0062
	cmp.l	#$0104,d1
	bcc.s	l0062
	adda.l	d1,a1
	move.w	#$0f9f,d7
l005e:	movem.w	(a1),d0-d3
	moveq	#3,d6
	addq.w	#8,a1
l005f:	moveq	#3,d5
l0060:	add.w	d0,d0
	addx.w	d4,d4
	add.w	d1,d1
	addx.w	d4,d4
	add.w	d2,d2
	addx.w	d4,d4
	add.w	d3,d3
	addx.w	d4,d4
	dbra	d5,l0060
	move.w	d4,-(a1)
	dbra	d6,l005f
	addq.w	#8,a1
	dbra	d7,l005e
	move.w	#1,pictureflag
no_merge_planes:
	rts
l0062:	lea	nopicfound_text(pc),a0
	bsr	print
	bra.s	no_merge_planes
optimize: dc.l $1580

reserve_table:
	tst.w	notab_request
	bne.s	notable
	pea	$30010.l		; LÑnge der Tabelle
	move.w	#$48,-(sp)
	trap	#1
	addq.l	#6,sp
	tst.l	d0
	bgt.s	table_ok		; Reservieren hat funktioniert
	lea	slow(pc),a0	; Textausgabe wenn nicht
	bsr	print
notable:	moveq	#-1,d0
table_ok:	move.l	d0,table_start
	rts


release_table:
	move.l	table_start,d0
	move.w	#-1,table_start
	tst.l	d0
	bmi.s	table_ok		; keine Tabelle vorhanden
	move.l	d0,-(sp)
	move.w	#$49,-(sp)
	trap	#1
	addq.l	#6,sp
	tst.l	d0
	rts



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

crunch:	clr.w	error		; kein Fehler aufgetreten
	clr.l	tabptr		; Tabelle noch nicht initialisiert
	move.l	#$8000,d0		; Tabellenoffset: $8000
	sub.l	optimize,d0	; mein Offset abziehen
	move.l	d0,faraway	; Dies merken
	bsr	countdown_init	; Countdown installieren
	movea.l	src_start(pc),a0	; a0 = Anfang der zu packenden Daten
	move.l	filesize(pc),d0	; d0 = LÑnge der Daten
	movea.l	packed_data(pc),a1	; a1 = Zielpuffer fuer gepackte Daten
	lea.l	0(a0,d0.l),a2	; Ende der Daten
	move.l	a2,src_ende
	move.l	#'Ice!',d7
	bsr	langwort_ablegen
	addq.l	#4,a1		; Platz lassen fÅr gepackte LÑnge
	move.l	filesize(pc),d7
	bsr	langwort_ablegen
	moveq	#0,d5		; d5 = abgelegte Bytes
	moveq	#7,d6		; d6 = ZÑhler fÅr abgelegte Bits
	moveq	#0,d7		; d7 = Informationslangwort
	moveq	#0,d4		; 1 Bit ablegen (Pictureflag)
	move.w	pictureflag(pc),d1
	bsr	put_bits


mainloop:
	bsr	countdown 	; Wie lange denn noch?
; 1. Folge von gleichen Bytes suchen aus auszÑhlen
	lea	$409(a0),a4	; a4 = Ende des Suchbereichs
	cmpa.l	src_ende,a4	; Dateiende Åberschritten?
	ble.s	gleich_ok		; nein!
	movea.l	src_ende,a4	; Dateiende
gleich_ok:
	move.l	a0,a3		; a3 = Anfang des Suchbereichs
	move.b	(a3)+,d0		; aktuelles Byte
gleich_compare:
	cmp.b	(a3)+,d0		; Byte mit akt. Byte vergleichen
	bne.s	gleich_ende	; nicht mehr gleich
	cmpa.l	a4,a3		; Ende des Suchbereichs erreicht?
	blt.s	gleich_compare	; nein, suche noch weiter
gleich_ende:
	move.l	a3,d1		; a1 = Ende des Strings
	sub.l	a0,d1		; a0 = Anfang
	subq.l	#2,d1		; dem Byte folgen d1 gleiche Kopien
	move.l	d1,maxgleich	; bisheriger lÑngste Bytefolge
	cmp.l	#$409,d1		; lÑnger als lÑngster String?
	beq	gleich_ablegen	; ja: String gar nicht erst testen

; 2. Strings mit mîglichst groûer LÑnge und kleinem Offset suchen
	move.l	a0,a3		; a3 = Anfang des Suchbereichs ...
	adda.l	optimize(pc),a3 	; + LÑnge = Ende des Suchbereichs
	cmpa.l	src_ende,a3	; Dateiende Åberschritten?
	ble.s	offset_ende	; nein!
	movea.l	src_ende,a3	; Dateiende
offset_ende:
	moveq	#1,d4		; d4 = maximale StringlÑnge
	lea	2(a0),a4		; a4 = a0 + 1

	tst.w	table_start	; existiert die Tabelle?
	bmi	no_table		; nein! Schade!

	move.l	a3,-(sp)
	move.l	tabptr,a6		;Tabelle beginnt hier
	move.l	a0,d0		;aktueller Pointer in orig. Daten
	sub.l	a6,d0		;Pointer der Tabelle
	cmp.l	faraway,d0 	;Ist die Tabelle noch aktuell?
	blt	ok1		;ja!
	move.l	a0,a6		;Tabelle auf aktuellen Pointer
	move.l	a0,tabptr		;fÅr nÑchstes Mal merken
	move.l	table_start,a2	;
	bsr	cr_tab		;Tabelle erstellen
	movea.l	a2,a4		;
	adda.l	#$020002,a4	;a4: 2. Teil der Tabelle
	lea	1(a6),a5		;
	moveq	#0,d0		;
	moveq	#0,d1		;
	moveq	#0,d2		;
cr_tab3:	movep.w	-1(a5),d1		;2 Bytes auslesen
	move.b	(a5)+,d1		;
	movea.l	d1,a3		;
	adda.l	a3,a3		;
	adda.l	a2,a3		;
	move.w	(a3),d2		;Stelle in Positionstabelle holen
	addq.w	#2,(a3)		;NÑchstes Mal in nachfolgende Zelle
	move.w	d0,0(a4,d2.l)	;Positionsnummer eintragen (0-$7fff)
	addq.w	#1,d0		;
	bpl.s	cr_tab3		;
	bsr	cr_tab		;verÑnderte Tabelle neu generieren

ok1:	move.l	a0,d0		;momentaner Pointer im File
	sub.l	a6,d0		;Offset zu Tab-Start berechnen
	move.l	table_start,a3	;
	moveq	#0,d3		;Stelle in Tab berechnen, ab der
	movep.w	0(a0),d3		; die Adressen stehen, in denen
	move.b	1(a0),d3		; das gesuchte 'Wort' vorkommt
	add.l	d3,d3		;
	movea.l	a3,a2		;
	adda.l	#$020002,a2	;
	movea.l	a2,a4		;
	moveq	#0,d1		;
	move.w	0(a3,d3.l),d1	;
	adda.l	d1,a2		;

find_identical:
	moveq	#0,d0		;
	move.w	(a2)+,d0		;d0 zeigt auf Stelle, wo der gleiche
	add.l	a6,d0		;
	cmp.l	a0,d0		;In Tabelle nach dem identischen
	bne.s	find_identical	; Byte suchen.

	move.l	a2,d0		;Vorherige Adressen ausschliessen
	sub.l	a4,d0		;DafÅr neuen Offset-Wert
	move.w	d0,0(a3,d3.l)	; zurÅckschreiben
	move.l	(sp)+,a3

weiter_mit_string:
	tst.w	table_start
	bmi.s	no_table

nextword:	move.l	tabptr,a4
	add.w	(a2)+,a4
	cmp.l	a4,a3		; Ist Offset Åberschritten?
	ble	string_suche_fertig	; ja!
	move.b	(a4)+,d0		; 1. Byte lesen
	cmp.b	(a0),d0		; Stimmt erstes Byte?
	bne	string_suche_fertig	; nein!
	move.b	(a4),d1		; 2. Byte lesen
	cmp.b	1(a0),d1		; Stimmt zweites Byte?
	bne	string_suche_fertig	; nein!
	bra.s	string_start_found	; String untersuchen.

no_table:
	move.b	(a0),d0		; current nach d0
	move.b	1(a0),d1		; current + 1 nach d1
search_string:
	cmp.b	(a4)+,d0		; nach gleichem Byte suchen
	beq.s	ein_byte_stimmt
no_string:
	cmpa.l	a4,a3		; sucht stringanfang innerhalb
	bgt.s	search_string	; current + offset
	bra.s	string_suche_fertig

ein_byte_stimmt:
	cmp.b	(a4),d1		; current + n+1 mit current + 1
	bne.s	no_string 	; string stimmt nicht mehr
string_start_found:
	move.l	a4,a6		; a6: String an dieser Adresse
	subq.w	#1,a6		;     genauer untersuchen
	move.l	a6,d0		; d0 = auch Start des Strings
	movea.l	a0,a5		; a5 = current Pointer
string_compare:
	cmp.b	(a5)+,(a6)+	; vergleiche soweit es geht
	bne.s	string_zu_ende	; nicht mehr gleich
	cmp.l	d0,a5		; sich selbst gefunden?
	bhi.s	string_zu_ende	; ja: dann Ende des Strings
	cmpa.l	a6,a3		; Ende des Suchbereichs erreicht?
	bgt.s	string_compare	; nein, suche noch weiter
string_zu_ende:
	move.l	a5,d1		; a5 = Ende des Strings
	sub.l	a0,d1		; a0 = Anfang
	subq.l	#1,d1		; => d1 = LÑnge
	move.l	a6,d2		; Ende der Kopie
	sub.l	a5,d2		; Ende Kopie minus Ende Original
	sub.l	d1,d2		; => d2 = Bytes zwischen Strings
	addq.l	#1,d2		; 0 Byte bei gleichen Bytes

	move.l	#$409,d0		; d0: maximale StringlÑnge
	cmp.l	d0,d4		; maximale LÑnge erreicht?
	beq.s	string_suche_fertig ; ja: aufhîren
	cmp.l	d0,d1		; maximale LÑnge Åberschritten?
	bls.s	not_too_large	; nein!
	sub.l	d0,d1		; um wieviel zu groû?
	add.l	d1,d2		; Das wird zum Offset addiert
	move.l	d0,d1		; und LÑnge auf Maximalwert
not_too_large:

	cmp.l	d1,d4		; lÑnger als bisheriger lÑngster
	bge.s	string_lohnt_nicht	; nein, dann lohnt es noch nicht

	cmp.w	#2,d1		; Wenn der String lÑnger ...
	bls.s	shortstring	; als 2 Bytes ist, und
	cmp.w	#$111f,d2 	; d2 grîûer als $111f ist, soll
	bhi.s	string_lohnt_nicht	; nicht gepackt werden!
	
shortstring:
	move.l	d1,d4		; neue maximale StringlÑnge ...
	move.w	d1,maxlength	; in d4 und maxlength und ...
	move.w	d2,maxoffset	; dann zugehîrigen Offset merken
string_lohnt_nicht:
	cmpa.l	a4,a3		; Schon Åber die Grenze hinaus?
	bgt	weiter_mit_string	; nein!

string_suche_fertig:
	move.l	maxgleich,d0
	cmp.w	#1,d0		; nur ein gleiches Byte?
	ble.s	nogleich		; ja: nichts machen
	cmp.l	d4,d0		; vergleiche Bytes mit Strings?
	bge.s	gleich_ablegen	; mehr gleiche Bytes
nogleich:	move.w	maxoffset(pc),d0	; Parameter des lÑngsten
	move.w	d4,d1		; Strings nach d0 und d1
	cmpi.w	#1,d1		; Ist String ein Byte lang?
	beq.s	ein_byte_ablegen	; ja
	cmpi.w	#2,d1		; Ist String zwei Bytes lang?
	beq.s	zwei_byte_ablegen	; ja
	bra.s	mehr_bytes_ablegen	; nein: Dann ist er lÑnger!

;**************************************************************************

ein_byte_ablegen:
	move.b	(a0)+,(a1)+	; ein Byte unverÑndert ablegen
	addq.l	#1,d5		; d5 = BytezÑhler
	bra.s	kein_byte_ablegen	; Byte wurde abgelegt

gleich_ablegen:
	move.l	maxgleich,d1	; Anzahl gleicher Bytes
	move.w	d1,maxlength
	moveq	#0,d0		; Offset ist immmer 0
	subq.l	#2,d1		; Anzahl gleicher Bytes = 2?
	bne.s	mehr_bytes_ablegen	; nein, also mehr

zwei_byte_ablegen:
	cmpi.w	#$23f,d0		; liegen Strings weit auseinander?
	bhi.s	ein_byte_ablegen	; ja
	bsr	make_normal_bytes	; Information der vorherigen Bytes
	bsr	make_offset_2	; Offset zu String in Bits ablegen
	bra.s	lÑnge_ablegen	; LÑnge des Strings ablegen

mehr_bytes_ablegen:
	bsr	make_normal_bytes	; Information der vorherigen Bytes
	bsr	make_offset_mehr	; Offset fÅr lange Strings
lÑnge_ablegen:
	bsr	make_stringlength	; String-LÑnge in Bits ablegen
	moveq	#0,d5		; ab jetzt wieder 0 Bytes abgelegt
	move.w	maxlength(pc),d0	; d0 Bits wurden eben gepackt
	add.w	d0,a0		; und werden Åbersprungen

kein_byte_ablegen:
	move.l	src_ende,d0 	; Kann noch gepackt werden?
	sub.l	a0,d0		; Ist Ende erreicht?
	subq.l	#3,d0		; 3 Bytes vor Ende ist Schluû
	bpl	mainloop		; nein, noch davor

still_packing:
	cmp.l	src_ende,a0	; Wurden alle Bytes gepackt?
	bge.s	all_packed	; ja!
	move.b	(a0)+,(a1)+	; nur noch ein Byte ablegen
	addq.l	#1,d5		; BytezÑhler erhîhen
	bra.s	still_packing
all_packed:
	bsr	make_normal_bytes	; Informationsbyte ablegen
	bset	d6,d7		; letztes Info-Byte erzeugen
	move.b	d7,(a1)+		; und ablegen
	sub.l	packed_data(pc),a1	; minus Anfang des Puffers
	move.l	a1,d0		; nach d0
	move.l	d0,d7
	move.l	packed_data,a1
	addq.l	#4,a1
	bsr	langwort_ablegen
	rts	

make_normal_bytes:
	cmp.l	#$810d,d5 	; zu viele einzelne Bytes abgelegt?
	bls.s	noerror		; ja: Das schaff ich nicht!!!
	move.w	#-1,error 	; error melden
	move.l	a0,src_ende	; und abbrechen
noerror:	lea.l	table1(pc),a3	; a3 = Byte-Tabelle
	moveq	#6,d3		; d3 = Pointer in Tabelle
kleiner:	move.w	-(a3),d4
	cmp.w	d4,d5		; Anzahl der bereits abgelegten
	dbge	d3,kleiner	; Bytes in Tabelle suchen
	sub.w	d4,d5		; d5 = öberschuû von Tabelle
	add.w	d3,d3		; d3 = jetzt Pointer auf Words
	lea	table1(pc),a3
	adda.w	d3,a3
	move.b	(a3)+,d2

	ext.w	d2		; als Wort behandeln
	moveq	#-1,d1		; alle Bits setzen
	lsl.l	d2,d1		; Rechts Nullen nachschieben
	or.w	d5,d1		; in d1 steht jetzt: 
				; %11111..1110 d5 (als Bits)
	moveq	#0,d5		; alle Bytes abgelegt: neu zÑhlen!
	move.b	(a3)+,d4		; in d4 jetzt das Bit_Menge-Byte
	ext.w	d4		; holen und zu Wort erweitern
	subq.w	#1,d4
	bra	put_bits		; d4+1 Bits ins Informationsbyte


;----------------------------------------------------------------------
;	Anzahl Bytes	abgelegt		shiften Åbertragen
;----------------------------------------------------------------------
;	0		%0		  1	  1
;	1		%10		  1	  2
;	2		%1100		  2	  4
;	3		%1101
;	4		%1110
;	5		%111100		  2	  6
;	6		%111101
;	7		%111110
;	8		%111111000	  3	  9
;	9		%111111001
;	10		%111111010
;	11		%111111011
;	12		%111111100
;	13		%111111101
;	14		%111111110
;	15		%11111111100000000	  8	 17
;	16		%11111111100000001
;	  ..		      ..
;	269   (=$10d)	%11111111111111110
;	270   (=$10e)	%11111111111111111000000000000000
;	271   (=$110)	%11111111111111111000000000000001
;	  ..		      ..
;	33037 (=$810d)	%11111111111111111111111111111111
;----------------------------------------------------------------------

	dc.w     0,    1,   2,    5,    8,   15,  270
table1:	dc.w $0101,$0102,$0204,$0206,$0309,$0811,$0f20
; 1. Liste aller Standardoffsets
; 2. Anzahl Bits shiften, Anzahl Bits zu Åbertragen


make_offset_mehr:
	lea.l	table3(pc),a3	; a3 = Zeiger auf Tabelle
	moveq	#2,d3		; d3 = Zeiger in Tabelle
look_on:	add.w	d3,d3		; d3 verdoppeln (mîglich: 4,2,0)
	move.w	0(a3,d3.w),d4	; d4 = Wert aus Tabelle
	lsr.w	#1,d3		; d3 zurÅcksetzen
	cmp.w	d4,d0		; mit gesuchtem Offset vergleichen
	dbge	d3,look_on	; nur wenn kleiner: nÑchsten Wert
	sub.w	d4,d0		; d0: Um wieviel war Offset zu groû
	add.w	d3,d3		; d3 wieder auf Wort positionieren
	move.w	6(a3,d3.w),d3	; aber diesmal in der zweiten Tab.
	move.w	d3,d4		; d4 = Anzahl zu Åbertragene Bits
	lsr.w	#8,d3		; d3 = Shift-Register
	moveq	#-1,d1		; d1 = alle Bits gesetzt
	lsl.w	d3,d1		; d1 nach links shiften
	or.w	d0,d1		; und d0 in LÅcke setzen
	andi.w	#$f,d4		; hîchstens 15 Bits Åbertragen
	bra	put_bits		; Bits als Information speichern

;----------------------------------------------------------------------
;	Offset		abgelegt		shiften   Åbertragen
;----------------------------------------------------------------------
;	0		%1000000		6	7
;	1		%1000001		6	7
;	..		..		..	..
;	$1f		%1011111		6	7
;	$20		%000000000	9	9
;	$21		%000000001	9	9
;	$22		%000000010	9	9
;	..		..		..	..
;	$11f		%011111111	9	9
;	$120		%11000000000000 	12	14
;	$121		%11000000000001 	12	14
;	$122		%11000000000010 	12	14
;	..		..		..	..
;	$111f		%11111111111111 	12	14
;----------------------------------------------------------------------

table3:	dc.w $0000,$0020,$0120
	dc.b $06,$06,$09,$08,$0C,$0D


make_offset_2:
	cmpi.w	#$23f,d0		; Abstand zwischen Strings zu groû?
	bhi	ein_byte_ablegen	; ja: nichts packen
	move.w	d0,d1		; d0/d1 = Abstand zwischen Strings
	cmpi.w	#$3f,d1		; Abstand kleiner oder gleich $3f
	ble.s	offs_3f		; ja
	subi.w	#$40,d1		; Offset: zwischen 0 und $1ff
	moveq	#9,d4		; 10 Bits als Information ablegen
	bset	d4,d1		; d1: zwischen $200 und $3ff
	bra	put_bits		; Information ablegen
offs_3f:	moveq	#6,d4		; 7 Bits Information ablegen
	bra.s	put_bits		; d1: zwischen 0 und $3f

;----------------------------------------------------------------------
;	Offset		abgelegt		Åbertragen
;----------------------------------------------------------------------
;	0		%0000000		7
;	1		%0000001		7
;	2		%0000010		7
;	3		%0000011		7
;	..		..		..
;	$3f		%0111111		7
;	$40		%1000000000	10
;	$41		%1000000001	10
;	$42		%1000000010	10
;	..		..		..
;	$23e		%1111111110	10
;	$23f		%1111111111	10
;----------------------------------------------------------------------


make_stringlength:
	lea.l	table2(pc),a3	; a3 = Zeiger auf Tabelle
	move.w	maxlength(pc),d0	; d0 = LÑnge des Strings
	moveq	#4,d5		; d5 = Pointer in table2
search:	move.b	0(a3,d5.w),d4	; d4 = gelesenes Byte aus Tabelle
	ext.w	d4		; als Wort behandeln
	cmp.w	d4,d0		; Ist der String grîûer oder gleich?
	dbge	d5,search 	; nein: In Tabelle weitersuchen
	sub.w	d4,d0		; d0 = um wieviel lagen wir daneben
	move.b	5(a3,d5.w),d4	; Shift Byte auslesen
	ext.w	d4		; als Wort behandeln
	moveq	#-1,d1		; d1: alle Bits auf 1 setzen
	lsl.w	d4,d1		; d1 nach links shiften
	or.w	d0,d1		; Bits in d1 setzen
	add.w	d5,d4		; d4 = Anzahl zu Åbertragene Bytes
				; mîgliche Ergebnisse: 1,2,4,6,14
	subq.w	#1,d4		; wegen dbf-Schleife
	bra.s	put_bits

;----------------------------------------------------------------------
;	LÑnge des Strings	abgelegt		shiften   Åbertragen
;----------------------------------------------------------------------
;	2		%0		1	1
;	3		%10		1	2
;	4		%1100		2	4
;	5		%1101		2	4
;	6		%111000		3	6
;	7		%111001		3	6
;	8		%111010		3	6
;	9		%111011		3	6
;	10		%11110000000000 	10	14
;	11		%11110000000001 	10	14
;	..		..		..	..
;	$409		%11111111111111 	10	14
;----------------------------------------------------------------------

table2:	dc.b	$02,$03,$04,$06,$0a
	dc.b	$01,$01,$02,$03,$0a

put_bits: lsr.l	#1,d1		; legt die d4+1 rechten Bits von d1
	roxr.b	#1,d7		; als Informationsbits ab
	dbf	d6,nextbit	; BitzÑhler - 1, wenn <0, dann ...
	move.b	d7,(a1)+		; ... Informationsbyte speichern
	moveq	#0,d7		; neues Informationsbyte lîschen
	moveq	#7,d6		; BitzÑhler auf 7 (hîchstes Bit)
nextbit:	dbf	d4,put_bits	; mehr Bits speichern?
	rts			; zurÅck

langwort_ablegen:
	rol.l	#8,d7
	move.b	d7,(a1)+		; Informationsbyte speichern
	rol.l	#8,d7
	move.b	d7,(a1)+		; Informationsbyte speichern
	rol.l	#8,d7
	move.b	d7,(a1)+		; Informationsbyte speichern
	rol.l	#8,d7
	move.b	d7,(a1)+		; Informationsbyte speichern
	rts

cr_tab:
;a2: Zeiger auf Tabelle, a6: Zeiger auf Bytes, die in Tabelle kommen
	lea	2(a2),a4		;
	move.w	#$0fff,d0 	;HÑufigkeits-Tab lîschen
	moveq	#0,d1		;$20000 Bytes lîschen
temp22:	move.l	d1,(a4)+		;
	move.l	d1,(a4)+		;
	move.l	d1,(a4)+		;
	move.l	d1,(a4)+		;
	move.l	d1,(a4)+		;
	move.l	d1,(a4)+		;
	move.l	d1,(a4)+		;
	move.l	d1,(a4)+		;
	dbra	d0,temp22 	;
	lea	2(a2),a4		;HÑufigkeits-Tab erstellen
	lea	1(a6),a5		;niedrigste Zugriffsadresse fÅr
	move.w	#$7fff,d0 	; Stringoffset
	moveq	#0,d1		;
cr_tab1:	movep.w	-1(a5),d1		;Zwei Bytes lesen
	move.b	(a5)+,d1		;
	movea.l	d1,a3		;Diese als Pointer in HÑufigkeits-
	adda.l	a3,a3		; tabelle betrachten
	addq.w	#2,(a4,a3.l)	;An richtiger Stelle: Tabellenwert+2
	dbra	d0,cr_tab1	;ganze Tabelle abarbeiten
	clr.w	-(a4)		;Offset-Tab erstellen
	move.w	#$1fff,d0 	;
cr_tab2:	move.w	(a4)+,d1		;
	add.w	d1,(a4)		;
	move.w	(a4)+,d1		;
	add.w	d1,(a4)		;
	move.w	(a4)+,d1		;
	add.w	d1,(a4)		;
	move.w	(a4)+,d1		;
	add.w	d1,(a4)		;
	move.w	(a4)+,d1		;
	add.w	d1,(a4)		;
	move.w	(a4)+,d1		;
	add.w	d1,(a4)		;
	move.w	(a4)+,d1		;
	add.w	d1,(a4)		;
	move.w	(a4)+,d1		;
	add.w	d1,(a4)		;
	dbra	d0,cr_tab2	;
	rts			;




header:	bra.s	start
textlength:
	dc.l	ende_header-header-$1c
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	1
	dc.w	-1
start:	bra.s	realstart
	dc.b	'Pack-Ice'
realstart:
	lea	ende_header+120(pc),a4  ; a4 = Zielpuffer
	move.l	a4,a5		    ; a5 = Ende der gepackten Daten
reg1:	add.l	#$deadface,a5	; gepackte LÑnge - 120
	move.l	a4,a6		    ; a6 = Ende entpackte Daten
reg2:	add.l	#$fadedead,a6	; originale LÑnge

	bsr	decrunch_exec

	lea	start-$100(pc),a5
	lea	start(pc),a6
	lea	$1c(a4),a3

do_reloc1:
norel1a:
	movem.l	-$1c+2(a3),d0-d3
	lea	(a3,d0.l),a0	; adressiere reloziertabelle des
	add.l	d1,a0		; entpackten programms
	add.l	d3,a0		; + symboltabelle !!!!!!!

	move.l	a6,d0		; Anfang Textbereich
	move.l	(a0)+,d1		; reloziertabelle vorhanden?
	beq.s	reloc_ende	; nein, relozierung fertig
	adda.l	d1,a3		; bestimme erstes relozierbares lword
	moveq	#0,d1		; loesche distanzregister
reloziere:
	add.l	d0,(a3)		; reloziere langwort
nextrelobyte:
	move.b	(a0)+,d1		; hole distanzwert
	beq.s	reloc_ende	; beende bei distanzwert 0

	cmpi.b	#1,d1		; spezialkennung fuer abstand 254?
	bne.s	addrelo_offset	; nein, normaler offsetwert
	add.w	#254,a3		; addiere abstandwert
	bra.s	nextrelobyte	; und suche naechsten eintrag
addrelo_offset:
	adda.l	d1,a3		; addiere abstandswert
	bra.s	reloziere 	; und reloziere
norel1b:
reloc_ende:			; Basepage erstellen
	lea	2(a4),a3		; Anfang der relozierten daten
	addq.l	#8,a5		; Auf Textanfang in Basepage
	move.l	(a5)+,a2		; a2: Anfang Text
	move.l	(a3),(a5)+	; LÑnge Text
	move.l	(a3),d2
	add.l	(a3)+,a2		; a2: Anfang Data
	move.l	a2,(a5)+		; Anfang Data
	move.l	(a3),(a5)+	; LÑnge Data
	add.l	(a3),d2
	add.l	(a3)+,a2		; a2: Anfang Bss
	move.l	a2,(a5)+		; Anfang Bss
	move.l	(a3),(a5)+	; LÑnge Bss
	add.l	(a3)+,a2		; a2: erste freie Adresse
	add.l	(a3),d2
	add.l	(a3)+,a2
	add.w	#ende_header-header+200+$1c+40,a2
				; LÑnge Entpacker dazu
				; a2 zeigt jetzt auf leeren Speicher
	moveq	#(decrunch-copy_loop)/4,d0
	lea	(a2),a0
	lea	copy_loop(pc),a3	; rette programmteil, da dieses
movedata: move.l	(a3)+,(a2)+	; programm ueberkopiert wird
	dbra	d0,movedata	;
	lea	$1c(a4),a1	; ueberspringe header
	lsr.l	#3,d2		; teile durch 8
	movea.l 	a6,a3
	jmp	(a0)

copy_loop:
	move.l	(a1)+,(a3)+	; kopiere programm nach vorne
	move.l	(a1)+,(a3)+	;
	subq.l	#1,d2		;
	bpl.s	copy_loop		;
do_reloc2:
norel2a:
	move.l	-$100+24(a6),a3 	; Anfang bss
	move.l	-$100+28(a6),d1	; LÑnge bss
	lsr.l	#3,d1		; 2 wegen Longword und 2 Befehle
	sub.l	a0,a0

clearbss: move.l	a0,(a3)+		; Hier wird das BSS Segment
	move.l	a0,(a3)+		; gelîscht
	subq.l	#1,d2
	bpl.s	clearbss
norel2b:
	jmp	(a6)		; starte programm

decrunch_exec:
	incbin	d:\packer\ice_pack\ice_exec.img
ende_header:

	include	d:\packer\ice_pack\ice_unpa.s

		ds.w 4
dta_puffer:	ds.w 13
filesize: 	ds.w 2
filename: 	ds.w 9
		dc.b '0',$00,$00,$00,$00,$00,$00,$00
		ds.w 21
		dc.b $05,$00,$00,$00,$00,$00,$00,$00
crlf:		dc.b $0a,$0d,$00
ask_sourcefile:	dc.b $0d,$0a,$0a,'Enter '
		dc.b 'name of '
		dc.b 'source f'
		dc.b 'ile: ',$00,$0a,$0d
		dc.b 'Enter na'
		dc.b 'me of de'
		dc.b 'stinatio'
		dc.b 'n file: '
		dc.b $00
nomem_text:	dc.b 'Could not'
		dc.b ' allocat'
		dc.b 'e enough'
		dc.b ' memory',$0a
		dc.b $0d,$00
working:		dc.b $0a,$0d,'Workin'
		dc.b 'g: ',$00
picfound_text:	dc.b 'P>',$00
crunch_another:	dc.b $0a,$0d,'Crunch '
		dc.b 'another '
		dc.b 'file (Y/'
		dc.b 'n): ',$00
ask_relocation:	dc.b $0d,$0a,$0a,'Reloca'
		dc.b 'te progr'
		dc.b 'am after'
		dc.b ' depacki'
		dc.b 'ng (Y/n)'
		dc.b ': ',$00
ask_picture:	dc.b $0d,$0a,$0a,'Additi'
		dc.b 'onal pic'
		dc.b 'ture com'
		dc.b 'pression'
		dc.b ' (N/y): '
		dc.b $00
ask_offset:	dc.b $0d,$0a,'Please '
		dc.b 'enter th'
		dc.b 'e offset'
		dc.b ' (Defaul'
		dc.b 't: $0000'
ascii_offset:	dc.b '): $',$00
askexec:		dc.b $0a,$0d,'1) Crun'
		dc.b 'ch data '
		dc.b 'file (De'
		dc.b 'fault)',$0a,$0d
		dc.b '2) Crunc'
		dc.b 'h execut'
		dc.b 'able fil'
		dc.b 'e',$0a,$0d,'3) Se'
		dc.b 't Parame'
		dc.b 'ters',$0a,$0d,'Pl'
		dc.b 'ease sel'
		dc.b 'ect: ',$00
nomem_verify:	dc.b 'No mem!',$00
verifytext:	dc.b '    Ve'
		dc.b 'rify: ',$00
verify_ok:	dc.b 'OK! ',$00
verify_error:	dc.b 'Error!  '
		dc.b '      '
		dc.b '[cancell'
		dc.b 'ed]',$00
pack_error:	dc.b '    Da'
		dc.b 'ta canno'
		dc.b 't be pac'
		dc.b 'ked!  [c'
		dc.b 'ancelled'
		dc.b ']',$00
remaining:	dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'CRemain'
		dc.b 'ing: '
ascii_prozent:	dc.b '99%',$00,$00
nopicfound_text:	dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C-',$00
slow:		dc.b '~',0
timetext: 	dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'D',$1b,'D',$1b,'D'
		dc.b $1b,'D',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'C',$1b,'C',$1b,'C',$1b,'C'
		dc.b $1b,'CTime: '
		dc.b '	    '
		dc.b '  ',$08,$08,$08,$08,$08,$08
		dc.b $08,$08,$08,$08,$00,$00
packingrate:	dc.b '  Rate: '
		dc.b $00
saving:		dc.b '% [saving'
		dc.b ']',$00
alt_pressed:	dc.b '   <A'
		dc.b 'LT> was '
		dc.b 'pressed.'
		dc.b ' File is'
		dc.b ' skipped'
		dc.b '.',$00
uneffectiv:	dc.b '    La'
		dc.b 'rger tha'
		dc.b 'n origin'
		dc.b 'al!   [c'
		dc.b 'ancelled'
		dc.b ']',$00
copyright:
 dc.b $1b,'E',$1b,'f',$a
 dc.b ' ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~',$d,$a
 dc.b ' |   Version 2.30                                                            |',$d,$a
 dc.b ' {                           P A C K - I C E                                 }',$d,$a
 dc.b ' |                                                                           |',$d,$a
 dc.b ' {             This packer was written by Axe of Delight.                    }',$d,$a
 dc.b ' |       First version: 14.02.1990    Final changes: 29.11.1990              |',$d,$a
 dc.b ' {                                                                           }',$d,$a
 dc.b ' ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~',$d,$a,$a,0
 even
aes_askexec:	dc.b '[0][| Wh'
		dc.b 'at do yo'
		dc.b 'u wish t'
		dc.b 'o pack? '
		dc.b ' |  - Da'
		dc.b 'ta files'
		dc.b ' |  - Ex'
		dc.b 'ecutable'
		dc.b ' files |'
		dc.b '  - Set '
		dc.b 'Paramete'
		dc.b 'rs ][ DA'
		dc.b 'TA | EXE'
		dc.b 'C | SETU'
		dc.b 'P ]',$00
aes_askreloc:	dc.b '[0][| Re'
		dc.b 'locate a'
		dc.b 'fter dec'
		dc.b 'runch?  '
		dc.b '|  ][ YE'
		dc.b 'S | NO ]'
		dc.b $00
aes_askanother:	dc.b '[2][| Do '
		dc.b 'you wish'
		dc.b ' to pack'
		dc.b ' | anoth'
		dc.b 'er file?'
		dc.b '|][ YES '
		dc.b '| NO | ?'
		dc.b '?? ]',$00
aes_ask_picture:	dc.b '[0][| Do '
		dc.b 'you wish'
		dc.b ' to use '
		dc.b 'an | add'
		dc.b 'itional '
		dc.b 'algorith'
		dc.b 'm  | for'
		dc.b ' picture'
		dc.b ' compres'
		dc.b 'sion? |]'
		dc.b '[ NO | Y'
		dc.b 'ES ]',$00,$00


		BSS
	include	d:\sources\aes_bss.s
l00d5:		ds.b 6
mausflag: 	ds.w 1
picturerequest:	ds.w 1
pictureflag:	ds.w 1
mallocadr:	ds.l 1
table_start:	ds.l 1
tabptr:		ds.l 1
notab_request:	ds.w 1
faraway:		ds.l 1
save1:		ds.w 1
save2:		ds.w 1
readpuffer:	ds.l 1
verify:		ds.w 1
newtime:		ds.l 1
oldtime:		ds.l 1
newlength:	ds.l 1
handle:		ds.w 1
dataflag: 	ds.w 1
nullbytes:	ds.l 2
src_start:	ds.l 1
src_ende: 	ds.l 1
packed_data:	ds.l 1
maxgleich:	ds.l 1
maxlength:	ds.w 1
maxoffset:	ds.w 1
interrupt:	ds.w 1
error:		ds.w 1
l00eb:		ds.l 1
fselect_path:	ds.b 80
filename_input:	ds.w 1
fselect_name:	ds.b 20
