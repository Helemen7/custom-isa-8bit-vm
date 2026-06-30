start:
    MOV A, 4                    ; Passiamo il parametro dentro A
    CALL calcola_quadrato_piu_cinque
    MOV B, D                    ; Il risultato finale sarà in D, lo spostiamo in B
    HALT

calcola_quadrato_piu_cinque:
    ; A contiene ancora 4. Dobbiamo passarlo a 'moltiplica' duplicato.
    MOV B, A                    ; Primo parametro per moltiplica
    MOV C, A                    ; Secondo parametro per moltiplica
    CALL moltiplica             ; Moltiplica farà B * C, risultato in B
    
    MOV D, B                    ; Prendiamo il risultato (16)
    ADD D, 5                    ; Aggiungiamo 5 (21)
    RET

moltiplica:
    MUL B, C                    ; B = B * C (4 * 4 = 16)
    RET
