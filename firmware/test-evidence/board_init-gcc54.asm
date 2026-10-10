0000065c <board_init>:
     65c:	f8 94       	cli
     65e:	84 b7       	in	r24, 0x34	; 52
     660:	80 68       	ori	r24, 0x80	; 128
     662:	84 bf       	out	0x34, r24	; 52
     664:	84 bf       	out	0x34, r24	; 52
     666:	87 b3       	in	r24, 0x17	; 23
     668:	80 7f       	andi	r24, 0xF0	; 240
     66a:	87 bb       	out	0x17, r24	; 23
     66c:	88 b3       	in	r24, 0x18	; 24
     66e:	8f 60       	ori	r24, 0x0F	; 15
     670:	88 bb       	out	0x18, r24	; 24
     672:	1f bc       	out	0x2f, r1	; 47
     674:	82 e0       	ldi	r24, 0x02	; 2
     676:	8e bd       	out	0x2e, r24	; 46
     678:	1d bc       	out	0x2d, r1	; 45
     67a:	1c bc       	out	0x2c, r1	; 44
     67c:	20 e1       	ldi	r18, 0x10	; 16
     67e:	37 e2       	ldi	r19, 0x27	; 39
     680:	3b bd       	out	0x2b, r19	; 43
     682:	2a bd       	out	0x2a, r18	; 42
     684:	94 e1       	ldi	r25, 0x14	; 20
     686:	98 bf       	out	0x38, r25	; 56
     688:	99 bf       	out	0x39, r25	; 57
     68a:	85 bd       	out	0x25, r24	; 37
     68c:	14 bc       	out	0x24, r1	; 36
     68e:	8a 9a       	sbi	0x11, 2	; 17
     690:	92 98       	cbi	0x12, 2	; 18
     692:	8a e0       	ldi	r24, 0x0A	; 10
     694:	83 bf       	out	0x33, r24	; 51
     696:	89 ef       	ldi	r24, 0xF9	; 249
     698:	8c bf       	out	0x3c, r24	; 60
     69a:	78 94       	sei
     69c:	08 95       	ret

