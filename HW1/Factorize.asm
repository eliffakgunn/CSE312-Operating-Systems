.data 
       number:	.asciiz "Please enter the number: " 
       factors:	.asciiz "Factors: " 
       newLine:	.asciiz "\n"
       space:	.asciiz " "
       
.text
              
main:

	#get number user
	li $v0, 4
	la $a0, number
	syscall
	
	li $v0, 5
	syscall
	move $a1, $v0
	
	blt $a1, 0, makePositive
	#else continue
	j continue

makePositive:
	mul $a1, $a1, -1
	
continue:
	
	li $a2, 1 #i = 1
	addi $t1, $a1, 1 #t0 = num + 1
	
	li $v0, 4
	la $a0, factors
	syscall
	
	loop:
		beq  $a2, $t1, endLoop
		
		#num % i
		div $t0, $a1, $a2
		mul $t0, $a2, $t0
		sub $t0, $a1, $t0
		
		beq $t0, 0, print
		
		addi $a2, $a2, 1
		j loop
		
		print:
			li $v0, 1
			move $a0, $a2
			syscall
			
			li $v0, 4
			la $a0, space
			syscall
			
			addi $a2, $a2, 1
			j loop

	endLoop:
		li $v0, 10
		syscall
		

