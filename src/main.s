    ;;  This program is free software: you can redistribute it and/or modify
    ;;  it under the terms of the GNU General Public License as published by
    ;;  the Free Software Foundation, either version 3 of the License, or
    ;;  (at your option) any later version.

    ;;  This program is distributed in the hope that it will be useful,
    ;;  but WITHOUT ANY WARRANTY; without even the implied warranty of
    ;;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    ;;  GNU General Public License for more details.

    ;;  You should have received a copy of the GNU General Public License
    ;;  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;; @vmu-it
  ;; @author  <jahan.addison@jacata.me>
  ;; @license GPL
  ;; @version 0.1
  ;;
  ;;
  ;; @changelog
  ;;
  ;;
  ;;

  .include "sfr.i"

;; VARIABLES

COUNTER = $30
STOP    = $31
STEP    = $32
X       = $33 ; horizontal position in frame buffer
Y       = $34 ; vertical position in frame buffer

;; Reset and Interrupt Vectors

 .org    0
 jmpf    start         

 .org    $3
 jmp     nop_irq

 .org    $b
 jmp     nop_irq

 .org    $13
 jmp     nop_irq

 .org    $1b
 jmp     t1int

 .org    $23
 jmp     nop_irq

 .org    $2b
 jmp     nop_irq

 .org    $33
 jmp     nop_irq

 .org    $3b
 jmp     nop_irq

 .org    $43
 jmp     nop_irq

 .org    $4b
 clr1    p3int,0
 clr1    p3int,1

nop_irq:
 reti

 .org    $130
t1int:
 push    ie
 clr1    ie,7
 not1    ext,0
 jmpf    t1int
 pop     ie
 reti

 .org    $1f0
goodbye:
 not1    ext,0
 jmpf    goodbye     


;;  Game Header

 .org    $200
 .byte   "VMU-it          "                     
 .byte   "written: jahan.addison@jacata.me"     

;;  Game Icon

  .org    $240
  .word   1               ; number of icons (max = 3)
  .word   10              ; animation speed
  .org    $260
  .word   $f000, $ff00, $f0f0, $f00f, $fff0, $faff, $ff1f, $ffff
  ; black, red, green, blue, yellow, cyan, purple, white
  .word   $ffff, $ffff, $ffff, $ffff, $ffff, $ffff, $ffff, $ffff
  ; white, white, white, white, white, white, white, white

  .org    $280

  ; A sample icon for now, just a yellow background

  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44
  .byte $44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44,$44


 ;
 ;; Start of program
 ;

start:
  ; For any game:
  mov #$a1,ocr
  mov #$09,mcr
  mov #$80,vccr
  clr1 p3int,0
  clr1 p1,7
  mov #$ff,p3

  ; clear screen

  call clrscr

  ; Move a pixel gracefully down the first LCD bank
  mov #0,STOP
  mov #$86,2
  mov #1,COUNTER
  mov #$10,@R2
  .loop:
  inc STOP
  ld STOP
  ; At 16, branch to next procedure in program
  be #$10,.continue
  call pause
  call moveup
  mov #0,@R2
  ld 2
  add #6
  st 2
  ld COUNTER
 ; be #2,.skip
  inc COUNTER
  .cskip:
  mov #$10,@R2
  br .loop
  .skip:
  mov #1,COUNTER
  ld 2
  add #4
  st 2
  br .cskip

  ; Move a pixel gracefully across the first LCD bank
  .continue:
  call clrscr
  call pause
  mov #$86,2
  mov #0,COUNTER
  ; 10000000: the beginning
  mov #$80,@R2
  .loopp:
  call pause  
  inc COUNTER
  ld @R2
  ror
  st @R2
  ld COUNTER
  be #8,.next
  br .loopp
  .next:
  mov #0,COUNTER
  mov #0,@R2
  ld 2
  add #1
  st 2
  mov #$80,@R2
  ; todo: divisible by 6
  be #$8C,.done
  br .loopp
  
  .done:
  call pause
  call clrscr
  jmp goodbye

  


.keypress:
  call getkeys
  bn acc,4,.keypress    
  bn acc,5,.keypress     
 
  br .keypress            


; Subroutines.

; todo: move the adjacent 4 bytes between 2 lines logic here

moveup:
  ld 2
  and #$f
  bne #$6,.up
  ld @R2
  push acc
  mov #0,@R2
  ld 2
  sub #12
  st 2
  pop acc
  st @R2
  .up:
  ret

; todo: move the adjacent 4 bytes between 2 lines logic here

movedown:
  ld 2
  and #$f
  bne #$6,.down
  ld @R2
  push acc
  mov #0,@R2
  ld 2
  add #6
  st 2
  pop acc
  st @R2
  .down:
  ret

pause:
  mov #0,b
  .start:
  mov #1,t1lr
  mov #$48,t1cnt 
  .run:
  ld t1lr
  bne #$ff,.run
  inc b
  ld b
  bne #3,.start
  clr1 t1cnt, 7
  ret  
           
    
clrscr:
  clr1 ocr,5
  push acc
  push xbnk
  push 2
  mov #0,xbnk
  .cbank:
  mov #$80,2
  .cloop:
  mov #0,@R2
  inc 2
  ld 2
  and #$f
  bne #$c,.cskip
  ld 2
  add #4
  st 2
  .cskip:
  ld 2
  bnz .cloop
  bp xbnk,0,.cexit
  mov #1,xbnk
  br .cbank
  .cexit:
  pop 2
  pop xbnk
  pop acc
  set1 ocr,5
  ret


setscr:
  clr1 ocr,5
  push acc
  push xbnk
  push c
  push 2
  mov #$80,2
  xor acc
  st xbnk
  st c
.sloop:
  ldc
  st @R2
  inc 2
  ld 2
  and #$f
  bne #$c,.sskip
  ld 2
  add #4
  st 2
  bnz .sskip
  inc xbnk
  mov #$80,2
.sskip:
  inc c
  ld c
  bne #$c0,.sloop
  pop 2
  pop c
  pop xbnk
  pop acc
  set1 ocr,5
  ret

getkeys:
  bp p7,0,quit           
  ld p3                  
  bn acc,6,quit         
  bn acc,7,sleep          
  ret                    
quit:
  jmp goodbye            
                      
sleep:
  set1 pcon,0            
  bn p3,7,sleep          
  mov #0,vccr            
sleepmore:
  set1 pcon,0             
  bp p7,0,quit            
  bp p3,7,sleepmore      
  mov #$80,vccr          
waitsleepup:
  set1 pcon,0            
  bn p3,7,waitsleepup
  br getkeys             

  .cnop   0,$200          ; pad to an even number of blocks
