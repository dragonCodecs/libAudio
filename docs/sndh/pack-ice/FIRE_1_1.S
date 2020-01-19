;**************************************************************************;
;           Fire Pack, a great packer written by Axe of Delight!           ;
;**************************************************************************;

; Fire Pack is the best packer that's available on the ST.
; I used especially much time to get the depacking-routine as small and
; fast (!!!) as possible.
; Assemble this source with Devpac 2 and set TAB-position to 10.

mouseoff:	macro 
	pea	moff(pc)
	clr.w	-(sp)
	move.w	#25,-(sp)
	trap	#14
	addq.l	#8,sp
	endm
mouseon:	macro
	pea	mon(pc)
	clr.w	-(sp)
	move.w	#25,-(sp)
	trap	#14
	addq.l	#8,sp
	endm

	opt	o+,ow-,a+

begin:	dc.w	$a00a		; schick die maus nach haus
	movea.l	sp,a5		; adressiere basepage
	movea.l	4(a5),a5		;
	move.l	12(a5),d0 	; bestimme zu reservierende
	add.l	20(a5),d0 	; programmlaenge
	add.l	28(a5),d0 	;
	addi.l	#$0200,d0 	; + basepagelaenge
	move.l	d0,-(sp)		; reserviere speicher ab der basepage
	move.l	a5,-(sp)		;
	move.w	#0,-(sp)		;
	move.w	#$4A,-(sp)	;
	trap	#1		; setblock
	adda.w	#$c,sp		;

	move.w	do_reloc1(pc),save1	; installiere orginalcode
	move.w	do_reloc2(pc),save2	;

	bsr.s	main		;Zum Hauptprogramm
	clr.w	-(sp)		;Gemdos $00 (0)
	trap	#1		;Pterm

main:
restart:
	lea	copyright(pc),a0	; gib copyright aus
	bsr	print		;
	lea	askexec(pc),a0	; frage, ob es sich um ein relozierbares
	bsr	print		; programm handelt
	move.w	#-1,relocflag
	moveq	#1,d0
	bra.s	set_packmode
waitkey:	bsr	cconin		; warte auf taste
	move.b	#1,dataflag	; vermerke, dass nur daten vorliegen
	cmpi.b	#'1',d0		; 1 = nur data
	beq.s	askverify 	;
	cmpi.b	#13,d0		; return = nur data
	beq.s	askverify 	;
	cmpi.b	#'2',d0		; 2 = ausfuehrbar
	beq.s	isexec
	ori.b	#$20,d0
	cmp.b	#'r',d0
	beq.s	changereloc
	swap	d0
	cmp.b	#58,d0
	bls.s	waitkey
	cmp.b	#68,d0
	bhi.s	waitkey
	sub.b	#58,d0
	ext.w	d0
set_packmode:
	moveq	#$10,d1
	lsl.l	#8,d1
	mulu	d0,d1
	move.l	d1,packmode
	bra.s	waitkey
changereloc:
	not.w	relocflag
	bra.s	waitkey

isexec:
	move.b	#0,dataflag	; ausfÅhrbares Programm

	move.w	save1(pc),do_reloc1	; installiere orginalcode
	move.w	save2(pc),do_reloc2	;

	tst.w	relocflag
	bne.s	askverify 	; relozieren

	move.w	#$6000+rel1b-rel1a,do_reloc1	; entferne reloziercode
	move.w	#$6000+rel2b-rel2a,do_reloc2

askverify:
	moveq	#-1,d3
;	lea	verifyask(pc),a0	; Frage: Verify ein/aus
;	bsr	print
;	bsr	cconin
;	ori.b	#$20,d0
;	cmp.b	#'n',d0
;	bne.s	verion
;	moveq	#0,d3
verion:	move.w	d3,verify

asksrc:
	lea	entersrc(pc),a0	; frage nach zu packender datei
	bsr	print		;
	pea	srcnam(pc)	; buffer fuer namen
	move.w	#$a,-(sp) 	;
	trap	#1		;
	addq.l	#6,sp		;
	lea	srcnamb(pc),a1	; setze nullbyte an eingabeende
	adda.w	d0,a1		;
	clr.b	(a1)		;
	lea	crlf(pc),a0	; weiteren zeilenvorschub
	bsr	print		;

	pea	dta(pc)		; setze dta zu suche
	move.w	#$1a,-(sp)	;
	trap	#1		;
	addq.l	#6,sp		;
	move.w	#0,-(sp)		; sfirst um die dateilaenge zu
	pea	srcnamb(pc)	; bekommen
	move.w	#$4e,-(sp)	;
	trap	#1		;
	addq.l	#8,sp		;
	tst.l	d0		; war die datei ueberhaupt da?
	bne.s	asksrc		; nein, frage erneut nach dem namen
fsnext:
	move.l	filesize(pc),d0	; Original- und Packspeicher nicht
	add.l	#1201,d0		; an gleiche Adresse legen
	bclr	#0,d0
	move.l	d0,-(sp)		; reserviere speicher fuer das
	move.w	#$48,-(sp)	; programm
	trap	#1		;
	addq.l	#6,sp		;
	tst.l	d0		; war genug frei?
	bmi	nomem		; nein, melde dass zuwenig speicher frei
	move.l	d0,packed_data	; Puffer fÅr gepackte Daten
	add.l	#1200,d0		; Originaldaten um 1000 Bytes hîher
	move.l	d0,src_start	; ... als gepackte Daten
	move.w	#2,-(sp)		; oeffne zum lesen
	pea	filename(pc)	;
	move.w	#$3d,-(sp)	;
	trap	#1		;
	addq.l	#8,sp		;
	move.w	d0,handle 	; merke handle
	tst.l	d0		; hat es geklappt?
	bmi	pterm0		; nein, dateifehler, beenden
	move.l	src_start(pc),-(sp)	; lies die datei ein
	move.l	filesize(pc),-(sp)	;
	move.w	handle(pc),-(sp)	;
	move.w	#$3f,-(sp)	;
	trap	#1		;
	lea	12(sp),sp 	;
	tst.l	d0		; ohne fehler?
	bmi	pterm0		; bei fehler beenden
	bsr	fclose		; datei nach gebrauch schliessen
	move.l	filesize(pc),d1	; bestimme endadresse der datei
	add.l	src_start(pc),d1	;
	move.l	d1,src_ende	; und speichere sie
	lea	crunchin(pc),a0	; melde start des packvorgangs
	bsr	print		;
	lea	filename(pc),a0
	bsr	print

	mouseoff
	clr.l	-(sp)		; schalte auf supervisor
	move.w	#$20,-(sp)	;
	trap	#1		;
	addq.l	#6,sp		
	move.l	d0,oldsp		; merke alten stack
	move.l	$04ba.w,zeit	; merke startzeit
	bsr	crunch 		; packe die daten
	move.l	d0,a3		; neue (gepackte) LÑnge
	bsr	printtime		; Wie lange hat's gedauert?
	move.l	oldsp(pc),-(sp)	; setze alten stack wieder
	move.w	#$20,-(sp)	;
	trap	#1		;
	addq.l	#6,sp		;
	mouseon

	lea	(a3),a1
	tst.b	dataflag		; nur Daten?
	bne.s	nurdata		; ja!
	add.w	#ende_header-header,a1	; + LÑnge des Programmheaders
nurdata:	move.l	a1,newlength	; FÅr Packingrate merken
	move.l	filesize(pc),a2	; OriginallÑnge
	cmp.l	a2,a1		; neue LÑnge grîûer?
	bge	toolarge		; ja: Hau ab!

	tst.w	verify		; Verify durchfÅhren?
	beq	notest		; nein

	lea.l	verifytext(pc),a0	; Verifytext ausgeben
	bsr	print
	move.l	filesize(pc),d1	; Original LÑnge
	addi.l	#11000,d1 	; plus 10000 Bytes Verifypuffer
	move.l	d1,-(sp)
	move.w	#$48,-(sp)
	trap	#1		; Diesen Speicher reservieren
	addq.l	#6,sp
	tst.l	d0
	bpl.s	memo_ok		; alles klar
	lea	noverifymem(pc),a0	; tut mir leid
	bsr	print		; kein ausreichender Speicher
	bra	notest		; aber trotzdem sichern

memo_ok:	      
	move.l	d0,a4		; Freier Speicher
	move.l	d0,a2		; nach a2 und a4
	move.l	d0,mallocadr	; Adresse merken
	move.l	a3,d1		; LÑnge des gepackten Bereichs
	subq.l	#1,d1		; fÅr DBF eins abziehen
	move.l	packed_data(pc),a1	; a1 zeigt auf gepacktes File

copyblk:	move.b	(a1)+,(a2)+	; gepacktes File in freien
	dbra	d1,copyblk	; Bereich kopieren
	subi.l	#$010000,d1	; falls es mehr als 512K sind
	bpl.s	copyblk		; weitermachen

	move.l	a4,a0		; Anfang gepackte Daten
	move.l	a3,d0		; LÑnge gepackte Daten

	bsr	decrunch_normal	; entpacken

	add.w	#120,a4		; a4 = hier liegt entpacktes File
	lea.l	100(a4),a2	; Entpacktes File
	add.l	filesize(pc),a2	; + originale LÑnge
	move.l	a2,d0		; = d0
	bclr	#0,d0		; auf gerade Adresse setzen
	move.l	d0,readpuffer	; = freier Speicher

	bsr	fopen		; ôffne File
readon:	move.l	readpuffer(pc),a0	; Lade nach Adresse a0
	move.l	#10000,d0 	; Lade 10000 Bytes
	bsr	fread		; jetzt lesen
	move.l	d0,d4		; gelesene LÑnge merken
	beq.s	doclose		; Falls End of File
	cmpi.l	#10000,d0 	; Konnten auch 10000 Bytes gelesen
	bcc.s	noclose		; werden?
doclose:
	bsr	fclose		; Nein: File schlieûen, da dies
	move.l	d4,d0		; das letzte Datenpaket war
noclose:	      
	movea.l	readpuffer(pc),a2	; Hier stehen jetzt bis 10000 Bytes
	subq.w	#1,d0		; wegen dbf
	bmi.s	copyok
compare:	      
	move.b	(a2)+,d1		; Byte aus File holen
	move.b	(a4)+,d2		; Byte aus entpackten Daten holen
	cmp.b	d1,d2		; Sind beide gleich?
	bne.s	shitjek		; Wenn nein: Error!
	dbf	d0,compare	; Weiter vergleichen
copyok:	cmpi.l	#10000,d4 	; Wurden 10000 Bytes gelesen
	beq.s	readon		; ja: Weiter lesen
	lea.l	verifyok(pc),a0	; TEST BESTANDEN
	bsr	print		; Dies auch melden
	bsr	mfree		; reservierten Speicher frei
	bra.s	notest		; Weiter wie ohne verify
shitjek:	      
	lea.l	scheisse(pc),a0	; Pech gehabt
	bsr	print		; Dies auch melden
	bsr	mfree		; Speicher freigeben
	bra	done		; aber NICHT abspeichern

mfree:	move.l	mallocadr(pc),-(sp)
	move.w	#$49,-(sp)
	trap	#1
	addq.l	#6,sp
	rts	

notest:	bsr	packingrate		; Prozentzahl bestimmen

	cmpi.b	#1,dataflag		; nur daten?
	beq.s	onlydata			; ja, schreibe nur die gepackten daten
	subq.l	#4,a3			; ICE! nicht speichern
	movea.w	#ende_header-header,a4	; codelaenge des entpackers
	movea.l	a3,a5			; hole laenge der gepackten daten
	lea	-$1C(a5,a4.w),a5		; bestimme textsegmentlaenge
	move.l	a5,l0036			; speichere textsegmentlaenge
	bsr	fcreate			; lege datei neu an
	lea	header(pc),a5		; speichere header
	movea.w	#ende_header-header,a6	; hat soviele bytes
	bsr	fwrite			; schreibe daten
	movea.l	packed_data(pc),a5		; hole startadresse des gepackten codes
	movea.l	a3,a6			; sowie dessen laenge
	bsr	fwrite			; schreibe den code weg
	lea	nullbyts(pc),a5		; nuller als reloziertabelle
	movea.w	#4,a6			; mit speichern
	bsr	fwrite			;
	bsr	fclose			; datei schliessen
	bra.s	done			; frage nach erneutem packvorgang
onlydata:
	bsr	fcreate		; lege datei an
	movea.l	packed_data(pc),a5	; hole quelladresse gepackter daten
	movea.l	a3,a6		; sowie dessen laenge
	bsr	fwrite		; schreibe diese daten weg
	bsr	fclose		; schliesse die datei
	bra.s	done

toolarge:
	lea	 nopackerr(pc),a0
	bsr	print
	;bra.s	 done

done:
	move.l	packed_data(pc),-(sp)	; loesche datenmodul wieder
	move.w	#$49,-(sp)		;
	trap	#1			;
	addq.l	#6,sp			; welcher bloedmann hatte dies vergessen ????
	tst.l	d0			; hat es geklappt?
	bne.s	pterm0			; nein, beende

	move.w	#$4f,-(sp)		;
	trap	#1			;
	addq.l	#2,sp			;
	tst.l	d0			; war die datei ueberhaupt da?
	bpl	fsnext			; nein, frage erneut nach dem namen

	lea	crlf(pc),a0		; einen zeilenvorschub bitte
	bsr	print			;
	lea	askanoth(pc),a0		; frage, ob noch etwas gepackt werden
	bsr	print			; soll
	bsr	cconin			; warte auf zeichen
	ori.b	#$20,d0			; unterdruecke grossbuchstaben
	cmpi.b	#'y',d0			; bei 'n' wird beendet
	bne.s	pterm0			; ende
	
	bra	restart			; und starte packer erneut
nomem:
	lea	nomemtxt(pc),a0		; melde speichermangel
	bsr	print			;
	bsr	crawcin			; warte auf eingabe
pterm0:
	rts


;---------------------------------------
fopen:	move.w	#0,-(sp)		; îffne Datei
	pea	filename(pc)	;
	move.w	#$3d,-(sp)	;
	trap	#1		;
	addq.l	#8,sp		;
	tst.l	d0		; hat es geklappt
	bmi.s	pterm0		; nein, fehler
	move.w	d0,handle 	; und merke handle
	rts			;
fread:	pea	(a0)		; a0 = Puffer
	move.l	d0,-(sp)		; d0 = Anzahl Bytes
	move.w	handle(pc),-(sp)
	move.w	#$3f,-(sp)
	trap	#1		; Jetzt lesen
	add.w	#$c,sp
	rts
fcreate:
	move.w	#0,-(sp)		; lege datei an
	pea	filename(pc)	;
	move.w	#$3c,-(sp)	;
	trap	#1		;
	addq.l	#8,sp		;
	tst.l	d0		; hat es geklappt
	bmi.s	pterm0		; nein, fehler
	move.w	d0,handle 	; und merke handle
	rts			;
fwrite:
	move.l	a5,-(sp)		; quelladresse
	move.l	a6,-(sp)		; datenlaenge
	move.w	handle(pc),-(sp)	; handle
	move.w	#$40,-(sp)	;
	trap	#1		;
	lea	12(sp),sp 	; schreibe daten
	tst.l	d0		; im fehlerfalle beenden
	bmi.s	pterm0		;
	rts			;
fclose:
	move.w	handle(pc),-(sp)	; hole handle
	move.w	#$3e,-(sp)	;
	trap	#1		; schliesse datei
	addq.l	#4,sp		;
	bmi.s	pterm0		;
	rts			;
cconin:
	move.w	#1,-(sp)		; warte auf tastatur
	trap	#1		;
	addq.l	#2,sp		;
	rts			;
crawcin:
	move.w	#7,-(sp)		; tastatureingabe mit echo
	trap	#1		;
	addq.l	#2,sp		;
	rts			;
print:
	move.l	a0,-(sp)		; textausgabe
	move.w	#9,-(sp)		;
	trap	#1		;
	addq.l	#6,sp		;
	rts			;

printtime:
	movem.l	d0-d7/a0-a6,-(sp)
	lea	timetext(pc),a0
	bsr.s	print
	move.l	$04ba.w,d0	; bestimme vergangene zeit
	sub.l	zeit(pc),d0
	divu	#200,d0
	swap	d0
	clr.w	d0
	swap	d0		; d0 = Anzahl Sekunden gesamt
	divu	#60,d0
	moveq	#0,d1
	move.w	d0,d1		; d1 = Minuten
	swap	d0
	moveq	#0,d2
	move.w	d0,d2		; d2 = Sekunden
	lea	digits(pc),a0
	clr.l	(a0)
	clr.l	2(a0)
minit:	divu	#10,d1
	swap	d1
	add.w	#'0',d1
	move.w	d1,(a0)+
	clr.w	d1
	swap	d1
	tst.w	d1
	bne.s	minit
	move.l	#$00200020,(a0)+
	lea	digits+6(pc),a0
	move.w	-(a0),d0
	bsr	conout
	move.w	-(a0),d0
	bsr	conout
	move.w	-(a0),d0
	bsr	conout
	move.b	#':',d0
	bsr	conout
	move.l	d2,d0
	divu	#10,d0
	add.w	#'0',d0
	bsr	conout
	swap	d0
	add.w	#'0',d0
	bsr	conout
	movem.l	(sp)+,d0-d7/a0-a6
	rts

digits:	dc.l	0,0,0,0

countdown_init:
	move.l	d0,-(sp)
	move.l	filesize(pc),d0		; gesamte FilelÑnge
	lsr.l	#7,d0			; in 128 StÅcke teilen
	move.l	d0,count_differenz		; Bei jedem StÅck werden
	move.l	src_start(pc),count_start	; die Prozent angezeigt!
	move.w	#-1,oldcountdown
	move.l	(sp)+,d0
	rts

countdown:
	cmp.l	count_start(pc),a4
	bcs.s	no_countdown
	movem.l	d0-d7/a0-a6,-(sp)
	move.l	count_differenz(pc),d0
	add.l	d0,count_start
	move.l	a4,d1
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
	move.l	#$00390039,d0
	sub.l	d1,d0
	lea	countd(pc),a0
	move.b	d0,(a0)+
	swap	d0
	move.b	d0,(a0)+
	lea	counttext(pc),a0
	bsr	print
endcountdown:
	movem.l	(sp)+,d0-d7/a0-a6
no_countdown:
	rts

count_start:	ds.l	1
count_differenz:	ds.l	1
oldcountdown:	ds.w	1


packingrate:
	lea	prozent1(pc),a0
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
	add.l	#$00300030,d0
	bsr	conout
	swap	d0
	bsr	conout
	lea	prozent2(pc),a0
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

conout:	movem.l d0-d3/a0-a3,-(sp)
	move.w	d0,-(sp)
	move.w	#2,-(sp)
	trap	#1
	addq.l	#4,sp
	movem.l (sp)+,d0-d3/a0-a3
	rts



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


crunch:	move.l	packmode(pc),d0	; mîglich: 0 bis $10000
	add.l	#$1110+$154,d0
	move.l	d0,optimize	; max. Offset + max. StringlÑnge
	bsr	countdown_init
	movea.l	src_start(pc),a4	; a4 = Anfang der zu packenden Daten
	move.l	filesize(pc),d0	; d0 = LÑnge der Daten
	movea.l	packed_data(pc),a5	; a5 = Zielpuffer fuer gepackte Daten
	lea.l	0(a4,d0.l),a6	; a6 = Ende der Daten
	moveq	#0,d5		; d5 = einzeln abgelegte Bytes
	moveq	#31,d6		; d6 = ZÑhler fÅr abgelegte Bits
	moveq	#0,d7		; d7 = Informationslangwort

mainloop:
	lea	-1(a6),a0		; 1 Byte vor Ende
	cmp.l	a0,a4		; hier nicht mehr packen
	bcc	ein_byte_ablegen	; also: Suche abbrechen

	move.l	a4,a3		; a3 = Anfang des Suchbereichs ...
	adda.l	#$ffff+$1110,a3	; + LÑnge = Ende des Suchbereichs
	cmpa.l	a6,a3		; Dateiende Åberschritten?
	ble.s	gleich_ok		; nein!
	movea.l	a6,a3		; Dateiende
gleich_ok:
	move.l	a4,a1
	move.b	(a1)+,d0		; aktuelles Byte
gleich_compare:
	cmp.b	(a1)+,d0		; Byte mit akt. Byte vergleichen
	bne.s	gleich_ende	; nicht mehr gleich
	cmpa.l	a1,a3		; Ende des Suchbereichs erreicht?
	bgt.s	gleich_compare	; nein, suche noch weiter
gleich_ende:
	move.l	a1,d1		; a1 = Ende des Strings
	sub.l	a4,d1		; a4 = Anfang
	subq.l	#2,d1		; => d1 = gleiche Bytes - 1
	move.l	d1,maxgleich	; bisheriger lÑngste Bytefolge
	cmp.l	#$ff+$40+$10+5,d1	; lÑnger als lÑngster String?
	bge	select_gleich	; ja: String gar nicht testen

	move.l	a4,a3		; a3 = Anfang des Suchbereichs ...
	adda.l	optimize(pc),a3	; + LÑnge = Ende des Suchbereichs
	cmpa.l	a6,a3		; Dateiende Åberschritten?
	ble.s	offset_ende	; nein!
	movea.l	a6,a3		; Dateiende
offset_ende:
	moveq	#1,d4		; d4 = maximale StringlÑnge
	lea	1(a4),a2		; a2 = a4 + 1

weiter_mit_string:
	move.b	(a4),d0		; current nach d0
	move.b	1(a4),d1		; current + 1 nach d1
search_string:
	cmp.b	(a2)+,d0		; nach gleichem Byte suchen
	beq.s	ein_byte_stimmt
no_string:
	cmpa.l	a2,a3		; sucht stringanfang innerhalb
	bgt.s	search_string	; current + offset
	bra	string_suche_fertig
ein_byte_stimmt:
	cmp.b	(a2),d1		; current + n+1 mit current + 1
	bne.s	no_string 	; string stimmt nicht mehr
string_start_found:
	move.l	a2,a1		; a1: String an dieser Adresse
	subq.l	#1,a1		;     genauer untersuchen
	move.l	a1,d0		; d0 = auch Start des Strings
	movea.l	a4,a0		; a0 = current Pointer
string_compare:
	cmp.b	(a0)+,(a1)+	; vergleiche soweit es geht
	bne.s	string_zu_ende	; nicht mehr gleich
	cmp.l	d0,a0		; sich selbst gefunden?
	bhi.s	string_zu_ende	; ja: dann Ende des Strings
	cmpa.l	a1,a3		; Ende des Suchbereichs erreicht?
	bge.s	string_compare	; nein, suche noch weiter
string_zu_ende:
	move.l	a0,d1		; a0 = Ende des Strings
	sub.l	a4,d1		; a4 = Anfang
	subq.l	#1,d1		; => d1 = LÑnge
	cmp.l	d1,d4		; lÑnger als bisheriger lÑngster
	bge.s	string_lohnt_nicht	; nein, dann lohnt es noch nicht
	move.l	a1,d2		; Ende der Kopie
	sub.l	a0,d2		; Ende Kopie minus Ende Original
	sub.l	d1,d2		; => d2 = Bytes zwischen Strings

	move.l	#$ff+$40+$10+5,d0	; maximale StringlÑnge
	cmp.l	d0,d1		; wird diese Åberschritten?
	ble.s	length_ok		; nein!
	sub.l	d0,d1		; um wieviel genau?
	add.l	d1,d2		; den öbertrag zum Offset addieren
	move.l	d0,d1		; und LÑnge auf Maximalwert
	cmp.l	d4,d1		; Der String ist kÅrzer geworden.
	ble.s	string_lohnt_nicht	; Lohnt er sich trotzdem noch?
length_ok:

	cmp.w	#2,d1		; wenn String > 2:
	beq.s	offset_ok		; Offset testen
	cmp.l	#$111f,d2		; maximaler Offset
	bgt.s	string_lohnt_nicht	; Offset zu groû
offset_ok:

	move.l	d1,d4		; neue maximale StringlÑnge ...
	move.l	d1,maxlength	; in d4 und maxlength und ...
	move.l	d2,maxoffset	; dann zugehîrigen Offset merken

string_lohnt_nicht:
	cmpa.l	a2,a3		; Schon Åber die Grenze hinaus?
	bgt	weiter_mit_string	; nein!

string_suche_fertig:
	cmp.l	maxgleich,d4	; besser String oder gleiche Bytes?
	ble	select_gleich	; gleiche Bytes!
	cmp.w	#1,d4		; gar kein String gefunden?
	beq	ein_byte_ablegen	; dann Byte normal ablegen
	cmp.w	#2,d4		; 2 Byte String gefunden
	beq	string_2		; ja
	bra	string_long	; sonst: langer String

;**************************************************************************
ein_byte_ablegen:
	bsr	test_specialbyte	; -1, 0, 1, 2, 4, 8, 16, ...
	tst.w	d0
	bpl.s	special_byte
	move.b	(a4)+,(a5)+	; ein Byte unverÑndert ablegen
	addq.l	#1,d5		; d5 = BytezÑhler
	bra	kein_byte_ablegen	; Byte wurde abgelegt

special_byte:
	addq.l	#1,a4
	bra	kein_byte_ablegen

select_gleich:
	bsr	make_normal_bytes	; Information der vorherigen Bytes
	bsr	make_gleich	; gleiche Bytes ablegen
	add.l	maxgleich(pc),a4	; Originalpointer erhîhen
	bra	kein_byte_ablegen	; Originalbyte ablegen

string_2:	bsr	make_normal_bytes	; Information der vorherigen Bytes
	bsr	make_offset_2
	bsr	make_length
	addq.l	#2,a4
	bra.s	kein_byte_ablegen

string_long:
	bsr	make_normal_bytes	; Information der vorherigen Bytes
	bsr	make_long_offset	; Offset zu String in Bits ablegen
	bsr	make_length	; LÑnge zu String in Bits ablegen
	moveq	#%00,d1		; Kennbits
	moveq	#2,d0		; Anzahl Bits
	bsr	putbits		; Bits ablegen
	add.l	maxlength,a4
	bra.s	kein_byte_ablegen
	nop

kein_byte_ablegen:
	bsr	countdown		; Wie lange denn noch?
	cmpa.l	a6,a4		; letztes Byte der Daten erreicht?
	bcs	mainloop		; nein, noch davor

	bsr	make_normal_bytes	; Informationsbyte ablegen
	bset	d6,d7		; letztes Info-Langwort erzeugen
	bsr	ablegen_d7	; und ablegen
	move.l	filesize(pc),d7	; ## originale LÑnge
	bsr	ablegen_d7	; speichern
	move.l	#'Fire',d7	; ## Kennung speichern
	bsr	ablegen_d7	; speichern
	sub.l	packed_data(pc),a5	; minus Anfang des Puffers
	move.l	a5,d0		; nach d0
	rts	


test_specialbyte:
	moveq	#-1,d0
	move.b	(a4),d2
	lea	special_tab(pc),a1
special_next:
	move.w	(a1),d1
	bmi.s	no_specialbyte
	cmp.b	d1,d2
	beq.s	special_found
	addq.l	#6,a1
	bra.s	special_next
special_found:
	pea	2(a1)
	bsr	make_normal_bytes	; vorherige normale Bytes markieren
	move.l	(sp)+,a1
	move.w	(a1)+,d1		; Bitkombination
	move.w	(a1)+,d0		; Anzahl der Bits
	bsr	putbits		; Bits ablegen
	moveq	#1,d0		; Kennung: Spezialbyte gefunden
no_specialbyte:
	rts

special_tab:
 dc.w	  1, %110000, 6
 dc.w	  2, %110001, 6
 dc.w	  4, %110010, 6
 dc.w	  8, %110011, 6
 dc.w	 16, %110100, 6
 dc.w	 32, %110101, 6
 dc.w	 64, %110110, 6
 dc.w	128, %110111, 6
 dc.w	  0, %1000,   4
 dc.w	 -1

make_gleich:
	lea	gleich_tab(pc),a0
	move.l	maxgleich(pc),d1	; LÑnge der gleichen Bytes
	subq.l	#1,d1
	bne.s	standard_gleich	; nein: unten weitermachen
	moveq	#%111000,d1	; Kennung: VorgÑngerbyte kopieren
	moveq	#6,d0		; Anzahl Bits
	bra	putbits		; Bits ablegen

standard_gleich:
	move.l	#$1110f,d0	; d0: Maximalwert
	cmp.l	d0,d1		; wenn d1 grîûer als Maximalwert
	ble.s	read_gleich	; ...
	move.l	d0,d1		; wird d1 auf Maximalwert gesetzt
read_gleich:
	move.l	(a0),d0		; Wert der LÑnge auslesen
	bmi	error		; -1: Tabelle zu Ende
	sub.l	d0,d1		; Standardwert abziehen
	bmi.s	found_gleich	; öbertrag vorhanden?
	add.w	#12,a0		; nÑchste Tabellenposition
	bra.s	read_gleich
found_gleich:
	add.l	d0,d1		; nein: vorherigen Wert verwenden
	move.l	4(a0),d0		; so viele Bits ablegen
	bsr	putbits
	move.l	8(a0),d1		; d1:  0,  1,  2,  3
	moveq	#2,d0		; 2 Bits von d1 abspeichern
	bsr	putbits
	moveq	#%0000,d1
	moveq	#4,d0
	bra	putbits

gleich_tab:
 dc.l	   $10,  4,  0
 dc.l	  $100,  8,  1
 dc.l	 $1000, 12,  2
 dc.l	$10000, 16,  3
 dc.w	-1

make_offset:
	move.l	table_position(pc),a0
	move.l	12(a0),d0		; d0 Bits fÅr Offset verwenden
	move.l	d0,d2		; d2:   4,   8,  12,  16
	and.w	#15,d2		; d2:   4,   8,  12,   0
	move.l	maxoffset(pc),d1	; Offset nach d1 holen
	bsr	putbits		; und d0 Bits davon schreiben
	lsr.w	#2,d2		; d2:   1,   2,   3,   0
	move.w	d2,d1		; Diese Bitkennung ablegen
	moveq	#2,d0		; 2 Bits werden abgelegt
	bra	putbits		; "bra" entspricht "bsr" und "rts"

make_long_offset:
	move.l	table_position(pc),a0
	move.l	12(a0),d0		; d0 Bits fÅr Offset verwenden
	move.l	d0,d2		; d2:   4,   8,  12,  16
	move.l	maxoffset(pc),d1	; Offset nach d1 holen
	bsr	putbits		; und d0 Bits davon schreiben
	lsr.w	#2,d2		; d2:   1,   2,   3,   4
	subq.w	#1,d2		; d2: %00, %01, %10, %11
	move.w	d2,d1		; Diese Bitkennung ablegen
	moveq	#2,d0		; 2 Bits werden abgelegt
	bra	putbits		; "bra" entspricht "bsr" und "rts"

make_length:
	move.l	maxlength(pc),d1
	subq.l	#5,d1		; LÑnge 5 ist ausgeschlossen
	lea	length_tab(pc),a0
find_length:
	tst.l	(a0)		; Tabelle zu Ende?
	bmi	error		; normal nicht mîglich
	sub.l	(a0),d1		; Standardoffset abziehen
	bmi.s	length_found	; Standardoffset grîûer
	addq.l	#8,a0		; nÑchste Tabellenposition
	bra.s	find_length	; weitersuchen
length_found:
	add.l	(a0),d1		; öberschuû von der StandardlÑnge
	move.l	4(a0),d0		; Anzahl Bits des öberschusses
	bsr	putbits
	move.l	4(a0),d1		; d1: 4, 6, 8
	subq.w	#2,d1		; d1: 2, 4, 6
	lsr.w	#1,d1		; d1: 1, 2, 3
	moveq	#2,d0		; 2 Bits von d1 abspeichern
	bra	putbits

length_tab:
 dc.l	 $10, 4
 dc.l	 $40, 6
 dc.l	$100, 8
 dc.l	-1


make_normal_bytes:
	move.l	#1038,d0
	sub.l	d0,d5		; zu viele einzelne Bytes abgelegt?
	bmi.s	less_1038		; nein: weniger als 1038!
	beq.s	less_1038		; nein: genau 1038!
	moveq	#-1,d1		; Information fÅr 1038 normale Bytes
	moveq	#19,d0		; 19 1-Bits bedeutet: 1038 Bytes
	pea	make_normal_bytes(pc)
	bsr.s	putbits
	moveq	#0,d1		; Information fÅr 0 gleiche Bytes
	moveq	#10,d0		; insgesamt 10 0-Bits
	bra.s	putbits

less_1038:add.l	d0,d5		; alten Wert wieder ausrechnen
	movem.l	normal_register(pc),d0-d2/a0-a2
				; d0 = 5: LÑnge der normal_table - 1
				; d1 = 0
				; d2 = 0
				; a0 = Byte-Tabelle
				; a1 = Shift-Tabelle
				; a2 = Anzahl Bits Åbertragen Tab.

kleiner:	move.b	(a0)+,d1		; d1 = StandardlÑnge
	cmp.w	d1,d5		; Anzahl der bereits abgelegten
	dbge	d0,kleiner	; Bytes in Tabelle suchen
	add.l	d0,a1		; a1 und a2 auf den zur LÑnge ...
	add.l	d0,a2		; passenden Wert setzen
	sub.w	d1,d5		; d5 = öberschuû von Tabelle
	move.b	(a1),d2		; Shift-Byte holen
	moveq	#-1,d1		; alle Bits setzen
	lsl.l	d2,d1		; Rechts Nullen nachschieben
	or.w	d5,d1		; in d1 steht jetzt: 
				; %11111..111 d5 (als Bits)
	move.b	(a2),d0		; in d0 jetzt das Bit_Menge-Byte
	moveq	#0,d5		; keine normalen Bytes abgelegt
	bra.s	putbits		; d0 Bits ins Informationsbyte

;----------------------------------------------------------------------
;	Anzahl Bytes	abgelegt		shiften	Åbertragen
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
;	15		%1111111110000000000  10	  19
;	16		%1111111110000000001
;         ..                  ..
;	1038 (=$40e)	%1111111111111111111
;----------------------------------------------------------------------

normal_register:
	dc.l	5, 0, 0, normal_table, normal_shift, normal_bits
normal_table:
	dc.b	15, 08, 05, 02, 01, 00
normal_shift:
	dc.b	01, 01, 02, 02, 03, 10
normal_bits:
	dc.b	01, 02, 04, 06, 09, 19


next_bit:	lsr.l	#1,d1		; legt die d0 rechten Bits von d1
	roxr.l	#1,d7		; als Informationsbits ab
	dbf	d6,putbits	; BitzÑhler - 1, wenn <0, dann ...
	bsr.s	ablegen_d7	; ... Langwort speichern
	moveq	#0,d7		; neues Informationsbyte lîschen
	moveq	#31,d6		; BitzÑhler auf 31 (hîchstes Bit)
putbits:	dbf	d0,next_bit	; mehr Bits speichern?
	rts			; zurÅck

ablegen_d7:
	rol.l	#8,d7
	move.b	d7,(a5)+		; Informationsbyte speichern
	rol.l	#8,d7
	move.b	d7,(a5)+		; Informationsbyte speichern
	rol.l	#8,d7
	move.b	d7,(a5)+		; Informationsbyte speichern
	rol.l	#8,d7
	move.b	d7,(a5)+		; Informationsbyte speichern
	rts

error:	lea	scheisse2(pc),a0
	bsr	print
hÑng_dich_auf:
	bra.s	hÑng_dich_auf

maxgleich:	ds.l	1
maxlength:	ds.l	1
maxoffset:	ds.l	1
table_position:	ds.l	1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


header:
	bra.s	start
l0036:	dc.l ende_header-header-$1c	; laenge textsegment
	dc.l 0			; laenge datasegment
	dc.l 0			; laenge bss-segment
	dc.l 0			; laenge symboltabelle
	dc.l 0			; reserviert
	dc.l 1			; fastload
	dc.w -1			; keine reloziertabelle
start:
	bra.s	realstart			; nur (C)opyright!
	dc.b	'Pack-Ice by Axe/Delight'	; Diese Zeilen
	even				; kînnen gelîscht
realstart:				; werden.
	lea	start(pc),a4	; textsegmentadresse holen
	move.l	start-$100+16(pc),a5; hole Anfang des Datensegments
	lea	ende_header-header+200(a4),a6
				; a6 = Anfang der entpackten Daten
	bsr	decrunch		; entpacke daten

	move.l	a6,a3		; Entpackte Daten
	lea	-$0100(a4),a5	; adressiere basepage
do_reloc1:
rel1a equ do_reloc1+2
	move.l	2(a3),a0		; adressiere reloziertabelle des
	add.l	6(a3),a0		; entpackten programms
	add.l	14(a3),a0 	; + symboltabelle !!!!!!!
	lea	$1c(a0,a3.l),a0	;

	move.l	8(a5),d0		; hole adresse textsegment
	add.w	#$1c,a3		; pointer auf entpacktes programm
	tst.l	(a0)		; reloziertabelle vorhanden?
	beq.s	reloc_ende	; nein, relozierung fertig
	adda.l	(a0)+,a3		; bestimme erstes relozierbares lword
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

rel1b:
reloc_ende:			; Basepage erstellen
	lea	2(a6),a3		; anfang der relozierten daten
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
	add.w	#ende_header-header+200+$1c+40,a2
				; LÑnge Entpacker dazu
				; a2 zeigt jetzt auf leeren Speicher
	moveq	#(decrunch-copy_loop)/4,d0
	lea	(a2),a0
	lea	copy_loop(pc),a1	; rette programmteil, da dieses
l003e:	move.l	(a1)+,(a2)+	; programm Åberkopiert wird
	dbra	d0,l003e		;
	lea	$1c(a6),a1	; Åberspringe header
	lsr.l	#3,d2		; teile durch 8
	movea.l	a4,a3
	jmp	(a0)

copy_loop:
	move.l	(a1)+,(a3)+	; kopiere programm nach vorne
	move.l	(a1)+,(a3)+	;
	dbf	d2,copy_loop	;
	moveq	#1,d3		; d3 = $0000 0001
	swap	d3		; d3 = $0001 0000 = $10000
	sub.l	d3,d2		; falls es mehr als 512K sind
	bpl.s	copy_loop 	;
do_reloc2:
rel2a equ do_reloc2+2
	move.l	-$100+24(a4),a3	; Anfang bss
	move.l	-$100+28(a4),d1 	; LÑnge bss
	lsr.l	#3,d1		; 2 wegen Longword und 2 Befehle
	moveq	#0,d0

clearbss:	move.l	d0,(a3)+		; Hier
	move.l	d0,(a3)+		; wird
	dbf	d1,clearbss	;
	sub.l	d3,d2		; falls es mehr als 512K sind
	bpl.s	clearbss		;
rel2b:
	jmp	(a4)		; starte programm

;********************************************* Unpackroutine von FIRE PACK
; a6 = Anfang der entpackten Daten, a5 = Ende der gepackten Daten
decrunch: movem.l	d0-d7/a0-a6,-(sp)
	move.l	a6,a0		; in a0 merken
	bsr.s	getlongword	; LÑnge holen
	add.l	d7,a6		; a6 = Ende der entpackten Daten
				; a5 = Ende von Byte-Daten
	bsr.s	getlongword	; erstes Informationslangwort
	lea	tabellen(pc),a4	; a4 = Zeiger auf Datenbereich
	moveq	#1,d6
	swap	d6		; d6 = $10000
	moveq	#0,d5		; d5 = 0 (oberes Wort: immer 0!)

normal_bytes:
	bsr.s	get_1_bit
	bcc.s	test_if_end	; Bit %0: keine Daten
	moveq	#0,d1		; falls zu copy_direkt
	bsr.s	get_1_bit
	bcc.s	copy_direkt	; Bitfolge: %10: 1 Byte direkt kop.
;	lea.l	direkt_tab+16-tabellen(a4),a1	; ... siehe nÑchste Zeile
	move.l	a4,a1
	moveq	#3,d3
nextgb:	move.l	-(a1),d0		; d0.w Bytes lesen
	bsr.s	get_d0_bits
	swap	d0
	cmp.w	d0,d1		; alle gelesenen Bits gesetzt?
	dbne	d3,nextgb		; ja: dann weiter Bits lesen
no_more:	add.l	16(a1),d1		; Anzahl der zu Åbertragenen Bytes
copy_direkt:
	move.b	-(a5),-(a6)	; Daten direkt kopieren
	dbf	d1,copy_direkt	; noch ein Byte
test_if_end:
	cmp.l	a0,a6		; Fertig?
	bgt.s	strings		; Weiter wenn Ende nicht erreicht
not_packed:
	movem.l	(sp)+,d0-d7/a0-a6
	rts

;************************** Unterroutinen: wegen Optimierung nicht am Schluû
getlongword:
	moveq	#3,d0
glw:	move.b	-(a5),d7
	ror.l	#8,d7
	dbf	d0,glw
	rts

; get_d0_bits: Siehe weiter unten
not_found:move.w	a5,d7
	btst	#0,d7
	bne.s	noteven
	move.l	-(a5),d7		; hole sonst ein weiters longword
	addx.l	d7,d7		; hole ein bit
;	addx.w	d1,d1		; und Åbernimm es
;	dbf	d0,hole_bit_loop	; bis alle bits geholt wurden
;	rts
	bra.s	on_d0

noteven:	move.l	-5(a5),d7
	lsl.l	#8,d7
	move.b	-(a5),d7
	subq.l	#3,a5
	add.l	d7,d7
	bset	#0,d7
;	addx.w	d1,d1
;	dbf	d0,hole_bit_loop
;	rts
	bra.s	on_d0

get_1_bit:
	add.l	d7,d7		; hole ein bit
	beq.s	not_found2	; quellfeld leer
	rts
not_found2:
	move.w	a5,d7
	btst	#0,d7
	bne.s	noteven2
	move.l	-(a5),d7		; hole sonst ein weiters longword
	addx.l	d7,d7		; hole ein bit
	rts
noteven2:	move.l	-5(a5),d7
	lsl.l	#8,d7
	move.b	-(a5),d7
	subq.l	#3,a5
	add.l	d7,d7
	bset	#0,d7
	rts

get_d0_bits:
	moveq	#0,d1		; ergebnisfeld vorbereiten
hole_bit_loop:
	add.l	d7,d7		; hole ein bit
	beq.s	not_found		; quellfeld leer
on_d0:	addx.w	d1,d1		; und Åbernimm es
	dbf	d0,hole_bit_loop	; bis alle bits geholt wurden
	rts
;************************************ Ende der Unterroutinen


strings:
	moveq	#1,d0		; 2 Bits lesen
	bsr.s	get_d0_bits
	subq.w	#1,d1
	bmi.s	gleich_morestring	; %00
	beq.s	length_2		; %01
	subq.w	#1,d1
	beq.s	length_3		; %10
	bsr.s	get_1_bit
	bcc.s	bitset		; %110
	bsr.s	get_1_bit
	bcc.s	length_4		; %1110
	bra.s	length_5		; %1111

get_short_offset:
	moveq	#1,d0
	bsr.s	get_d0_bits	; d1:  0,  1,  2,  3
	subq.w	#1,d1
	bpl.s	contoffs
	moveq	#0,d0		; Sonderfall
	rts

get_long_offset:
	moveq	#1,d0		; 2 Bits lesen
	bsr.s	get_d0_bits	; d1:  0,  1,  2,  3
contoffs:	add.w	d1,d1		; d1:  0,  2,  4,  6
	add.w	d1,d1		; d1:  0,  4,  8, 12
	movem.w	offset_table-tabellen(a4,d1),d0/d5
	bsr.s	get_d0_bits	; 4, 8, 12 oder 16 Bits lesen
	add.l	d5,d1
	rts


gleich_morestring:			; %00
	moveq	#1,d0		; 2 Bits lesen
	bsr.s	get_d0_bits	; d1:  0,  1,  2,  3
	subq.w	#1,d1
	bmi.s	gleich_string	; %0000

	add.w	d1,d1		; d1:      0,  2,  4
	add.w	d1,d1		; d1:      0,  4,  8
	movem.w	more_table-tabellen(a4,d1),d0/d2
	bsr.s	get_d0_bits
	add.w	d1,d2		; d2 = StringlÑnge
	bsr.s	get_long_offset
	move.w	d2,d0		; d0 = StringlÑnge
	bra.s	copy_longstring

bitset:	moveq	#2,d0		; %110
	bsr.s	get_d0_bits
	moveq	#0,d0
	bset	d1,d0
	bra.s	put_d0

length_2:	moveq	#7,d0		; %01
	bsr.s	get_d0_bits
	moveq	#2-2,d0
	bra.s	copy_string

length_3:	bsr.s	get_short_offset	; %10
	tst.w	d0
	beq	put_d0		; 0 ablegen
	moveq	#3-2,d0
	bra.s	copy_string

length_4:	bsr.s	get_short_offset	; %1110
	tst.w	d0
	beq.s	vorgÑnger_kopieren
	moveq	#4-2,d0
	bra.s	copy_string

length_5:	bsr.s	get_short_offset	; %1111
	tst.w	d0
	beq.s	put_ff
	moveq	#5-2,d0
	bra.s	copy_string


put_ff:	moveq	#-1,d0
	bra.s	put_d0

vorgÑnger_kopieren:
	move.b	(a6),d0
;	bra.s	put_d0

put_d0:	move.b	d0,-(a6)
	bra.s	backmain


gleich_string:
	bsr.s	get_long_offset	; Anzahl gleicher Bytes lesen
	beq.s	backmain		; 0: zurÅck
	move.b	(a6),d0
copy_gl:	move.b	d0,-(a6)
	dbf	d1,copy_gl
	sub.l	d6,d1
	bmi.s	backmain
	bra.s	copy_gl

copy_longstring:
	subq.w	#2,d0		; StringlÑnge - 2 (wegen dbf)
copy_string:			; d1 = Offset, d0 = Anzahl Bytes -2
	lea.l	2(a6,d1.l),a1	; Hier stehen die Originaldaten
	add.w	d0,a1		; dazu die StringlÑnge-2
	move.b	-(a1),-(a6)	; ein Byte auf jeden Fall kopieren
dep_b:	move.b	-(a1),-(a6)	; mehr Bytes kopieren
	dbf	d0,dep_b		; und noch ein Mal
backmain:	bra	normal_bytes	; Jetzt kommen wieder normale Bytes


direkt_tab:
	dc.l	$03ff0009,$00070002,$00030001,$00030001 ; Anzahl 1-Bits
tabellen:	dc.l	    15-1,      8-1,      5-1,      2-1  ; Anz. Bytes

offset_table:
	dc.w	 3,             0
	dc.w	 7,          16+0
	dc.w	11,      256+16+0
	dc.w	15, 4096+256+16+0
more_table:
	dc.w	3,       5
	dc.w	5,    16+5
	dc.w	7, 64+16+5
;*************************************************** Ende der Unpackroutine
ende_header:

;********************************************* Unpackroutine von ICE-PACK
; a0 = Pack-Adresse, d0 = LÑnge
decrunch_normal:
	movem.l	d0-d7/a0-a6,-(sp)
	lea	(a0,d0.l),a5	; a5 = Ende aller Daten
	bsr	getlongword	; Kenn-Langwort holen
	cmp.l	#'Fire',d7	; Kennung gefunden?
	bne	not_packed	; nein: nicht gepackt
	add.w	#120,a0		; Anfang entpackte Daten: hîher
	bsr	getlongword	; LÑnge holen
	move.l	d7,(sp)		; OriginallÑnge: spÑter nach d0
	lea	(a0,d7.l),a6	; a6 = neues Ende
				; a5 = Ende von Byte-Daten
	bsr	getlongword	; erstes Informationslangwort
	lea	tabellen(pc),a4	; a4 = Zeiger auf Datenbereich
	moveq	#1,d6
	swap	d6		; d6 = $10000
	moveq	#0,d5		; d5 = 0 (oberes Wort: immer 0!)
	bra	normal_bytes
;********************************************* weiter mit anderer Routine

		ds.l 2
dta:		ds.w 13
filesize:		ds.l 1
filename:		ds.w 9
srcnam:		dc.b '0',$00
srcnamb:		ds.w 24
dstnam:		dc.b '0',$00
dstnamb:		ds.w 24
inoffset:		dc.b $05,$00
inoffsetb:	ds.w 3
crlf:		dc.b $0A,$0D,$00
entersrc: dc.b 13,10,10,'Enter name of source file: ',0
enterdst: dc.b $0A,$0D,'Enter name of destination file: ',0
nomemtxt: dc.b 'Could not allocate enough memory',$0A,$0D,$00
crunchin: dc.b $0A,$0D,'Working: ',0
askanoth: dc.b $0A,$0D,'Crunch another file (y/N): ',$00
askexec:
	dc.b $0A,$0D,'1) Crunch data file (Default)'
	dc.b $0A,$0D,'2) Crunch executable file'
	dc.b $0A,$0D,'Choose:',$00
verifyask:
	dc.b	13,10,10,'Pack files with verify (Y/n)? ',0
noverifymem:
	dc.b	'No mem!',0
verifytext:
	dc.b	'      Verify: ',0
verifyok:
	dc.b	'OK! ',0
scheisse:
	dc.b	'Error!          [cancelled]',0
scheisse2:
	dc.b	'       ***** Fatal Error during packing!!!',0
	even
counttext:
	rept 45
	dc.b $1b,'D'			; Cursor nach links
	endr
	rept 24
	dc.b $1b,'C'			; Cursor nach rechts
	endr
	dc.b	'Remaining: '
countd:	dc.b	'99%',0

	even
timetext:
	rept 45
	dc.b $1b,'D'			; Cursor nach links
	endr
	rept 24
	dc.b $1b,'C'			; Cursor nach rechts
	endr
	dc.b	'Time:           ',8,8,8,8,8,8,8,8,8,8,0
	even

prozent1:
	dc.b	'  Rate: ',0
prozent2:
	dc.b	'% [saving]',0
nopackerr:
	dc.b	'      Larger than original!   [cancelled]',0
copyright:
	dc.b $1b,'E',10

 dc.b	' ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~',13,10
 dc.b	' |   Version 1.0                                                             |',13,10
 dc.b	' {                             F I R E   P A C K                             }',13,10
 dc.b	' |                                                                           |',13,10
 dc.b	' {                 This packer was written by Axe of Delight.                }',13,10
 dc.b	' |                   First version finished on: 18.04.1990                   |',13,10
 dc.b	' {                   Final changes on:          18.04.1990                   }',13,10
 dc.b	' |                                                                           |',13,10
 dc.b	' ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~',13,10
 dc.b	10,0

	even
moff:	dc.b	$12,0
mon:	dc.b	$8,0
	even

	section bss

mallocadr:	ds.l 1
save1:		ds.w 1
save2:		ds.w 1
relocflag:	ds.w 1
packmode:		ds.l 1
readpuffer:	ds.l 1
verify:		ds.w 1		; Verify durchfÅhren
zeit:		ds.l 1		; vergangene zeit wÑhrend packen
newlength:	ds.l 1		; neue LÑnge des Files
handle:		ds.w 1
dataflag:		ds.w 1
oldsp:		ds.w 2
nullbyts: 	ds.w 4
src_start:	ds.l 1		; ungepackte daten
packed_data:	ds.l 1		; gepackte daten
src_ende: 	ds.l 1		; ende ungepackter daten
string_dist:	ds.w 2
string_tabwert:	ds.w 1
long_string:	ds.w 1
offset_long_string:	ds.w 1
optimize:		ds.l 1

;***************************************************************


