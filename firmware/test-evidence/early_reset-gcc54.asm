000004f0 <early_reset>:
     4f0:	84 b7       	in	r24, 0x34	; 52
     4f2:	80 93 5b 02 	sts	0x025B, r24	; 0x80025b <reset_cause>
     4f6:	14 be       	out	0x34, r1	; 52
     4f8:	0f b6       	in	r0, 0x3f	; 63
     4fa:	f8 94       	cli
     4fc:	a8 95       	wdr
     4fe:	81 b5       	in	r24, 0x21	; 33
     500:	88 61       	ori	r24, 0x18	; 24
     502:	81 bd       	out	0x21, r24	; 33
     504:	11 bc       	out	0x21, r1	; 33
     506:	0f be       	out	0x3f, r0	; 63

