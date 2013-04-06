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

  mov #0,Y
  mov #$80,2
  mov #$10,@R2

  ; Move a pixel gracefully down the LCD frame buffer
  .downn:
  call pause
  call movedown
  ld Y
  be #31,.upp
  br .downn
  
  ; Move a pixel gracefully up the LCD frame buffer
  .upp:
  call moveup
  call pause
  ld Y
  bz .done
  br .upp

  ; We're done here
  .done:
  call clrscr
  jmp goodbye

  


.keypress:
  call getkeys
  bn acc,4,.keypress    
  bn acc,5,.keypress     
 
  br .keypress            


; Subroutines.

moveup:
  ld Y
  ; check if top of buffer
  be #0,.up
  ; branch to bank 0 coroutine at 16
  be #$10,.ubank
  ; check the lowest-order bit (big-endian) for even to skip
  bn Y,0,.upskip
  ; move down
  .upcont:
  dec Y
  ld @R2
  push acc
  mov #0,@R2
  .upcontt:
  ld 2
  sub #6
  st 2
  pop acc
  st @R2
  br .up
  .ubank:
  ; save our current state for bank 0
  ld @R2
  push acc
  mov #0,@R2
  mov #0,xbnk
  ld 2
  ; add by 118 to get the last row position on the new bank
  add #$76
  st 2
  dec Y
  pop acc
  st @R2
  ; we've moved up, we're done here
  br .up
  .upskip:
  dec Y
  ld @R2
  mov #0,@R2
  push acc
  ld 2
  sub #4
  st 2
  br .upcontt
  .up:
  ret


movedown:
  ld Y
  ; check if bottom of buffer
  be #$1F,.down
  ; branch to bank 1 coroutine at 15
  be #$f,.dbank
  ; check the lowest-order bit (big-endian) for odd to skip
  bp Y,0,.downskip
  ; move down
  .downcont:
  inc Y
  ld @R2
  push acc
  mov #0,@R2
  .downcontt:
  ld 2
  add #6
  st 2
  pop acc
  st @R2
  br .down
  .dbank:
  ; save our current state for bank 1
  ld @R2
  push acc
  mov #0,@R2
  mov #1,xbnk
  ld 2
  ; subtract by 118 to get the first row position on the new bank
  sub #$76
  st 2
  inc Y
  pop acc
  st @R2
  ; we've moved down, we're done here
  br .down
  .downskip:
  inc Y
  ld @R2
  mov #0,@R2
  push acc
  ld 2
  add #4
  st 2
  br .downcontt
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
