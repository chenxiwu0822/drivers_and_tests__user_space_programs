//基于S3C2440利用ARM汇编语言实现跑马灯程序 
 
/* asm.s */ 
GPFCON   EQU    0x56000050    //0x56000050    is the address
GPFDAT   EQU    0x56000054 
GPFUP    EQU    0x56000058   

 



port means IO port。
  EXPORT LEDTEST   //define the global variable
   AREA  LEDTESTASM,CODE,READONLY   ;//该伪指令定义了一个代码段，段名为LEDTESTASM，属性只读 
 
LEDTEST 
  ;设置GPF4－GPF7为output 
    ldr  r0,=GPFCON        //here ldr is pseudo instruction,not same as ldr instruction,address,that is  0x56000050,WHEN OPERATE R0,EQUAL TO SET 0x56000050.
    ldr  r1,[r0] 
    bic  r1,r1,#0xff00 
    orr  r1,r1,#0x5500 
    str  r1,[r0] 
     
    ;禁止GPF4－GPF7端口的上拉电阻 
    ldr  r0,=GPFUP 
    ldr  r1,[r0] 

 
    orr  r1,r1,#0xf0 
    str  r1,[r0] 
 
looptest 
    ;将数据端口F的数据寄存器的地址附给寄存器r2 
    ldr  r2,=GPFDAT 
     
    ldr  r3,[r2] 
    bic  r3,r3,#0xf0 
    orr  r3,r3,#0xb0 
    str  r3,[r2]              ;GPF6 output 0 
    ldr r0,=0x2ffff 
    bl   delay                ;调用延迟子程序 
     
    ldr  r3,[r2] 
    bic  r3,r3,#0xf0 
    orr  r3,r3,#0x70 
    str  r3,[r2]              ;GPF7 output 0 
    ldr r0,=0x2ffff 
    bl   delay                ;调用延迟子程序 
  
    ldr  r3,[r2] 
    bic  r3,r3,#0xf0 
    orr  r3,r3,#0xd0 
    str  r3,[r2]              ;GPF5 output 0 
    ldr r0,=0x2ffff 
    bl   delay                ;调用延迟子程序 
     
    ldr  r3,[r2] 
    bic  r3,r3,#0xf0 
    orr  r3,r3,#0xe0 
    str  r3,[r2]              ;GPF4 output 0 
    ldr r0,=0x2ffff 
    bl   delay                ;调用延迟子程序 
 
    b looptest 

//跑马灯延时 
delay 
  sub r0,r0,#1              ;r0=r0-1             
  cmp r0,#0x0               ;将r0的值与0相比较 
  bne delay                 ;比较的结果不为0（r0不为0），继续调用delay,否则执行下一条语句 
  mov pc,lr                 ;返回 //return to the caller
     
END                         ;程序结束符 
