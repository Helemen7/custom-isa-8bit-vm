start:
    MOV A, 0
    MOV B, 100
    MOV C, 0
    MOV D, 0
    MOV [100], 110
    MOV [110], 120
    MOV [120], 85
    MOV C, [B]
    SPUSH [B]
    MOV B, 0
    MOV D, [C]
    SPUSH [D]
    MOV D, 0
    SPOP A
    SPOP B
    CMP A, 85
    JE calcolo_finale
    HALT
calcolo_finale:
    MOV D, 15
    ADD A, D
    ADD A, B
    HALT
