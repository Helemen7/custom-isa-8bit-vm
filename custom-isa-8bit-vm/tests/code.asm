; -------------------------------------------------------
; PROGRAMMA DI TEST GRAFICO: STELLA GIALLA 4 PUNTE (8x8)
; -------------------------------------------------------

; 1. Attiva la modalità GUI scrivendo 1 nel registro di controllo
MOV A, 1
MOV [0x2FFA], A

; 2. Prepariamo il codice colore Giallo (Indice 5) nel registro B
MOV B, 5

; 3. DISEGNO DELLA STELLA (Pixel calcolati con: Y * 64 + X)

; --- Riga 0 (Punta superiore) ---
FBSET 3, B          ; Pixel (3, 0) -> NUMBER

; --- Riga 1 ---
FBSET 66, B         ; Pixel (2, 1) -> NUMBER
FBSET 67, B         ; Pixel (3, 1) -> NUMBER
FBSET 68, B         ; Pixel (4, 1) -> NUMBER

; --- Riga 2 ---
FBSET 129, B        ; Pixel (1, 2) -> NUMBER
FBSET 130, B        ; Pixel (2, 2) -> NUMBER
FBSET 131, B        ; Pixel (3, 2) -> NUMBER
FBSET 132, B        ; Pixel (4, 2) -> NUMBER
FBSET 133, B        ; Pixel (5, 2) -> NUMBER

; --- Riga 3 (Centro perfetto - Qui scatta NUMBER_16!) ---
FBSET 192, B        ; Pixel (0, 3) -> NUMBER
FBSET 193, B        ; Pixel (1, 3) -> NUMBER
FBSET 194, B        ; Pixel (2, 3) -> NUMBER
FBSET 195, B        ; Pixel (3, 3) -> NUMBER
FBSET 196, B        ; Pixel (4, 3) -> NUMBER
FBSET 197, B        ; Pixel (5, 3) -> NUMBER
FBSET 198, B        ; Pixel (6, 3) -> NUMBER

; --- Riga 4 (Inizia la discesa speculare) ---
FBSET 257, B        ; Pixel (1, 4) -> NUMBER_16 (>= 256)
FBSET 258, B        ; Pixel (2, 4) -> NUMBER_16
FBSET 259, B        ; Pixel (3, 4) -> NUMBER_16
FBSET 260, B        ; Pixel (4, 4) -> NUMBER_16
FBSET 261, B        ; Pixel (5, 4) -> NUMBER_16

; --- Riga 5 ---
FBSET 322, B        ; Pixel (2, 5) -> NUMBER_16
FBSET 323, B        ; Pixel (3, 5) -> NUMBER_16
FBSET 324, B        ; Pixel (4, 5) -> NUMBER_16

; --- Riga 6 (Punta inferiore) ---
FBSET 387, B        ; Pixel (3, 6) -> NUMBER_16

; 4. Sincronizza il Framebuffer con MiniFB per mostrare il disegno
FBSYNC

; 5. Fine del programma
HALT
