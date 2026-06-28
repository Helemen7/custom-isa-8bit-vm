start:
	MOV A, 5
	MOV B, 3
	MOV C, 0
loop:
	CMP B, 0
	JE end
	ADD C, A
	SUB B, 1
	JMP loop
end:
	MOV A, C
	HALT

