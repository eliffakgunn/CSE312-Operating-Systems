.data
	str:		.asciiz "\n\nPlease enter the filename (ShowPrimes.asm, Factorize.asm, or BubbleSort.asm)\n"
	buffer:		.space 64
	newline:	.asciiz "\n"
.text

main:
	loop: 					
		#prints message to screen to get filename 
		la $a0, str	
		li $v0, 4
		syscall
		
		#takes filename
		la $a0, buffer
		li $a1, 64
		li $v0, 8				
		syscall

		#call CreateProcess syscall
		li $v0,18				
		syscall
		
		j loop					


