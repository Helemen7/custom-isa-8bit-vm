start:
    MOV A, 0
    MOV B, 0
    MOV C, 0
    MOV D, 0
    HALLOC A, 10
    HALLOC B, 5
    HALLOC C, 10
    MOV [B], 99
    HFREE A
    HFREE B
    HALLOC D, 15
    MOV [D], 55
    MOV A, [C]
    MOV B, [D]
    HALT
