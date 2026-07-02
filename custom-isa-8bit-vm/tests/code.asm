; ============================================================
; SNAKE GAME - 8-bit Custom ISA VM
; Grid: 64x64, Arrow keys: 1=UP 2=DOWN 3=LEFT 4=RIGHT
; Colors: 0=BLACK 2=RED(food) 3=GREEN(snake)
;
; Memory layout (offsets from program base):
;   0-1:   bootstrap (entry = 2)
;   2-5:   JMP code_start
;   6-69:  body_x[64] (array of X coords)
;   70-133: body_y[64] (array of Y coords)
;   134:   head_idx (starts at 2)
;   135:   tail_idx (starts at 0)
;   136:   direction
;   137:   new_dir
;   138:   snake_len
;   139:   food_x
;   140:   food_y
;   141:   frame_cnt
;   142:   move_int
;   143:   tmp_x
;   144:   tmp_y
;   145:   new_head_x (saved from tmp_x)
;   146:   new_head_y (saved from tmp_y)
;   147+:  CODE
; ============================================================

start:
    JMP code_start

; =================== PADDING (DATA AREA) ===================
.skip 140
; =================== CODE ===================
code_start:

; ===================== INIT =====================
init:
    ; head_idx = 2, tail_idx = 0
    ; body[0]=(30,32) body[1]=(31,32) body[2]=(32,32)
    MOV A, 2
    MOV [134], A
    MOV A, 0
    MOV [135], A

    MOV A, 4
    MOV [136], A
    MOV [137], A

    MOV A, 3
    MOV [138], A

    MOV A, 6
    MOV [142], A

    MOV A, 0
    MOV [141], A

    MOV A, 48
    MOV [139], A
    MOV A, 32
    MOV [140], A

    ; Init all 64 body entries
    MOV C, 0
init_body:
    MOV A, 30
    ADD A, C
    CMP C, 2
    JNG ib_keep
    MOV A, 32
ib_keep:
    MOV B, C
    ADD B, 6
    MOV [B], A

    MOV A, 32
    MOV B, C
    ADD B, 70
    MOV [B], A

    ADD C, 1
    CMP C, 64
    JNE init_body

    CALL full_redraw
    FBSYNC

; ===================== MAIN LOOP =====================
game_loop:

; ---------- INPUT ----------
    INPUT A
    CMP A, 0
    JE no_key

    CMP A, 1
    JNE ck_dn
    MOV B, [136]
    CMP B, 2
    JE no_key
    MOV A, 1
    MOV [137], A
    JMP no_key

ck_dn:
    CMP A, 2
    JNE ck_lt
    MOV B, [136]
    CMP B, 1
    JE no_key
    MOV A, 2
    MOV [137], A
    JMP no_key

ck_lt:
    CMP A, 3
    JNE ck_rt
    MOV B, [136]
    CMP B, 4
    JE no_key
    MOV A, 3
    MOV [137], A
    JMP no_key

ck_rt:
    CMP A, 4
    JNE no_key
    MOV B, [136]
    CMP B, 3
    JE no_key
    MOV A, 4
    MOV [137], A

no_key:

; ---------- FRAME COUNTER ----------
    MOV A, [141]
    ADD A, 1
    MOV [141], A
    MOV B, [142]
    CMP A, B
    JNE draw_skip

    MOV A, 0
    MOV [141], A

; ---------- COMMIT DIRECTION ----------
    MOV A, [137]
    MOV [136], A

; ---------- COMPUTE NEW HEAD ----------
    MOV D, [134]
    MOV A, D
    ADD A, 6
    MOV B, [A]
    MOV [143], B
    MOV A, D
    ADD A, 70
    MOV B, [A]
    MOV [144], B

    MOV A, [136]
    CMP A, 1
    JNE mv_dn
    MOV A, [144]
    SUB A, 1
    MOV [144], A
    JMP chk_wall

mv_dn:
    CMP A, 2
    JNE mv_lt
    MOV A, [144]
    ADD A, 1
    MOV [144], A
    JMP chk_wall

mv_lt:
    CMP A, 3
    JNE mv_rt
    MOV A, [143]
    SUB A, 1
    MOV [143], A
    JMP chk_wall

mv_rt:
    MOV A, [143]
    ADD A, 1
    MOV [143], A

; ---------- SAVE NEW HEAD + WALL COLLISION ----------
chk_wall:
    ; Save new head position before anything overwrites [143,144]
    MOV A, [143]
    MOV [145], A
    MOV A, [144]
    MOV [146], A

    MOV A, [143]
    CMP A, 63
    JG gameover
    MOV A, [144]
    CMP A, 63
    JG gameover

; ---------- SELF COLLISION ----------
    MOV D, [135]
    ADD D, 1
    CMP D, 64
    JNE sl_start
    MOV D, 0

sl_start:
    MOV A, [134]
    CMP D, A
    JE sl_done

    MOV A, D
    ADD A, 6
    MOV B, [A]
    CMP B, [143]
    JNE sl_next
    MOV A, D
    ADD A, 70
    MOV B, [A]
    CMP B, [144]
    JE gameover

sl_next:
    MOV A, D
    ADD A, 1
    MOV D, A
    CMP D, 64
    JNE sl_start
    MOV D, 0
    JMP sl_start

sl_done:

; ---------- FOOD CHECK ----------
    MOV A, [143]
    CMP A, [139]
    JNE no_eat
    MOV A, [144]
    CMP A, [140]
    JNE no_eat

; ---------- EAT ----------
    MOV A, [138]
    ADD A, 1
    MOV [138], A

    MOV A, [139]
    ADD A, 7
    CMP A, 60
    JNG food_ok
    MOV A, 5
food_ok:
    MOV [139], A
    MOV A, [140]
    ADD A, 3
    CMP A, 60
    JNG food_y_ok
    MOV A, 10
food_y_ok:
    MOV [140], A

    JMP store_hd

; ---------- ERASE TAIL ----------
no_eat:
    MOV D, [135]
    MOV A, D
    ADD A, 6
    MOV B, [A]
    MOV [143], B
    MOV A, D
    ADD A, 70
    MOV B, [A]
    MOV [144], B

    MOV B, [144]
    MUL B, 64
    ADD B, [143]
    MOV A, [144]
    DIV A, 4
    FBSET A, 0

    MOV A, [135]
    ADD A, 1
    CMP A, 64
    JNE tw_nw
    MOV A, 0
tw_nw:
    MOV [135], A

; ---------- STORE NEW HEAD ----------
store_hd:
    MOV A, [134]
    ADD A, 1
    CMP A, 64
    JNE hw_nw
    MOV A, 0
hw_nw:
    MOV [134], A

    MOV D, [134]
    MOV A, D
    ADD A, 6
    MOV B, [145]
    MOV [A], B
    MOV A, D
    ADD A, 70
    MOV B, [146]
    MOV [A], B

; ---------- DRAW NEW HEAD ----------
    MOV B, [146]
    MUL B, 64
    ADD B, [145]
    MOV A, [146]
    DIV A, 4
    FBSET A, 3

; ---------- DRAW FOOD ----------
draw_skip:
    MOV B, [140]
    MUL B, 64
    ADD B, [139]
    MOV A, [140]
    DIV A, 4
    FBSET A, 2

    FBSYNC
    JMP game_loop

; ===================== GAME OVER =====================
gameover:
    FBSYNC
    HALT

; ===================== FULL REDRAW =====================
full_redraw:
    MOV D, [135]
fr_loop:
    MOV A, D
    ADD A, 6
    MOV B, [A]
    MOV [143], B
    MOV A, D
    ADD A, 70
    MOV B, [A]
    MOV [144], B

    MOV B, [144]
    MUL B, 64
    ADD B, [143]
    MOV A, [144]
    DIV A, 4
    FBSET A, 3

    MOV A, [134]
    CMP D, A
    JE fr_done

    MOV A, D
    ADD A, 1
    MOV D, A
    CMP D, 64
    JNE fr_loop
    MOV D, 0
    JMP fr_loop

fr_done:
    RET
