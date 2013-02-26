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
  mov #$a1,ocr
  mov #$09,mcr
  mov #$80,vccr
  clr1 p3int,0
  clr1 p1,7
  mov #$ff,p3

  call clrscr
  mov #$80,2
  mov #$ff,@R2
  .loop:
  call pause
  ld 2
  add #1
  st 2
  inc c
  mov #$ff,@R2
  ld c
  bne #20,.loop
  


.keypress:
  call getkeys
  bn acc,4,.keypress    
  bn acc,5,.keypress     
 
  br .keypress            


; Subroutines.

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
  ;
  push acc
  push xbnk
  push 2
  ;
  mov #0,xbnk
  .cbank:
  mov #$80,2
  .cloop:
  mov #0,@R2
  ;
  inc 2
  ;
  ld 2
  and #$f
  bne #$c,.cskip
  ;
  ld 2
  add #4
  st 2
  .cskip:
  ld 2
  bnz .cloop
  ;
  bp xbnk,0,.cexit
  ;
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
  ;
  push c
  push 2
  mov #$80,2
  ;
  xor acc
  st xbnk
  st c
.sloop:
  ldc
  st @R2
  ;
  inc 2
  ;
  ld 2
  and #$f
  bne #$c,.sskip
  ;
  ld 2
  add #4
  st 2
  ;
  bnz .sskip
  ;
  inc xbnk
  mov #$80,2
.sskip:
  inc c
  ld c
  bne #$c0,.sloop
  ;
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
  ;
sleepmore:
  set1 pcon,0             
  bp p7,0,quit            
  bp p3,7,sleepmore      
  mov #$80,vccr          
  ;
waitsleepup:
  set1 pcon,0            
  bn p3,7,waitsleepup
  br getkeys             

  .cnop   0,$200          ; pad to an even number of blocks
