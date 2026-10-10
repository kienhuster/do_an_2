0000080a <diagnostic_tick>:
     80a:	cf 92       	push	r12
     80c:	df 92       	push	r13
     80e:	ef 92       	push	r14
     810:	ff 92       	push	r15
     812:	41 11       	cpse	r20, r1
     814:	0a c0       	rjmp	.+20     	; 0x82a <diagnostic_tick+0x20>
     816:	9f b7       	in	r25, 0x3f	; 63
     818:	f8 94       	cli
     81a:	81 b3       	in	r24, 0x11	; 17
     81c:	87 7e       	andi	r24, 0xE7	; 231
     81e:	81 bb       	out	0x11, r24	; 17
     820:	82 b3       	in	r24, 0x12	; 18
     822:	87 7e       	andi	r24, 0xE7	; 231
     824:	82 bb       	out	0x12, r24	; 18
     826:	9f bf       	out	0x3f, r25	; 63
     828:	3e c0       	rjmp	.+124    	; 0x8a6 <diagnostic_tick+0x9c>
     82a:	6b 01       	movw	r12, r22
     82c:	7c 01       	movw	r14, r24
     82e:	81 b3       	in	r24, 0x11	; 17
     830:	88 61       	ori	r24, 0x18	; 24
     832:	81 bb       	out	0x11, r24	; 17
     834:	20 91 db 00 	lds	r18, 0x00DB	; 0x8000db <next.2168>
     838:	30 91 dc 00 	lds	r19, 0x00DC	; 0x8000dc <next.2168+0x1>
     83c:	40 91 dd 00 	lds	r20, 0x00DD	; 0x8000dd <next.2168+0x2>
     840:	50 91 de 00 	lds	r21, 0x00DE	; 0x8000de <next.2168+0x3>
     844:	c7 01       	movw	r24, r14
     846:	b6 01       	movw	r22, r12
     848:	0e 94 86 0c 	call	0x190c	; 0x190c <due>
     84c:	88 23       	and	r24, r24
     84e:	59 f1       	breq	.+86     	; 0x8a6 <diagnostic_tick+0x9c>
     850:	8c e2       	ldi	r24, 0x2C	; 44
     852:	c8 0e       	add	r12, r24
     854:	81 e0       	ldi	r24, 0x01	; 1
     856:	d8 1e       	adc	r13, r24
     858:	e1 1c       	adc	r14, r1
     85a:	f1 1c       	adc	r15, r1
     85c:	c0 92 db 00 	sts	0x00DB, r12	; 0x8000db <next.2168>
     860:	d0 92 dc 00 	sts	0x00DC, r13	; 0x8000dc <next.2168+0x1>
     864:	e0 92 dd 00 	sts	0x00DD, r14	; 0x8000dd <next.2168+0x2>
     868:	f0 92 de 00 	sts	0x00DE, r15	; 0x8000de <next.2168+0x3>
     86c:	80 91 da 00 	lds	r24, 0x00DA	; 0x8000da <step.2167>
     870:	90 e0       	ldi	r25, 0x00	; 0
     872:	01 96       	adiw	r24, 0x01	; 1
     874:	6a e0       	ldi	r22, 0x0A	; 10
     876:	70 e0       	ldi	r23, 0x00	; 0
     878:	0e 94 39 19 	call	0x3272	; 0x3272 <__divmodhi4>
     87c:	80 93 da 00 	sts	0x00DA, r24	; 0x8000da <step.2167>
     880:	4f b7       	in	r20, 0x3f	; 63
     882:	f8 94       	cli
     884:	22 b3       	in	r18, 0x12	; 18
     886:	30 91 da 00 	lds	r19, 0x00DA	; 0x8000da <step.2167>
     88a:	37 70       	andi	r19, 0x07	; 7
     88c:	81 e0       	ldi	r24, 0x01	; 1
     88e:	90 e0       	ldi	r25, 0x00	; 0
     890:	01 c0       	rjmp	.+2      	; 0x894 <diagnostic_tick+0x8a>
     892:	88 0f       	add	r24, r24
     894:	3a 95       	dec	r19
     896:	ea f7       	brpl	.-6      	; 0x892 <diagnostic_tick+0x88>
     898:	80 95       	com	r24
     89a:	88 71       	andi	r24, 0x18	; 24
     89c:	92 2f       	mov	r25, r18
     89e:	97 7e       	andi	r25, 0xE7	; 231
     8a0:	89 2b       	or	r24, r25
     8a2:	82 bb       	out	0x12, r24	; 18
     8a4:	4f bf       	out	0x3f, r20	; 63
     8a6:	ff 90       	pop	r15
     8a8:	ef 90       	pop	r14
     8aa:	df 90       	pop	r13
     8ac:	cf 90       	pop	r12
     8ae:	08 95       	ret

