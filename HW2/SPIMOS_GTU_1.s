#SPIMOS_GTU_1

.text
main:
	#init_program syscall 
	li $v0 ,23
	li $a0, 1 #to indicate kernel type
	syscall

	li $v0, 10
	syscall    

