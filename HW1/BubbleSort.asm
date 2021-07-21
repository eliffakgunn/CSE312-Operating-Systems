.data 
	arr:		.word 100 #MAX_SIZE = 100
       	arrSize:	.asciiz "Please enter the size of the array: "
	arrElm:		.asciiz "Please enter the elements of array:\n"
	sortedArr:	.asciiz "Sorted array: "
	space:		.asciiz " "
       
.text
              
main2:
	la $s0, arr #arr[100]
	
	#get array size from user
	li $v0, 4
	la $a0, arrSize
	syscall
	
	li $v0, 5
	syscall
	move $a1, $v0
	
	#get elements of array from user
	li $v0, 4
	la $a0, arrElm
	syscall
		
	li $t0, 0 #i
	li $t1, 0 #dört artacak her seferinde
fillArr:
	beq  $t0, $a1, endFillArr
				
	li $v0, 5
	syscall
	move $t2, $v0
	
	sw $t2, arr($t1)

	addi $t0, $t0, 1	
	addi $t1, $t1, 4
	
	j fillArr			
endFillArr:

	li $t0, 0 #i
	addi $a2, $a1, -1 # size - 1
	li $t1, 0 #dört artacak her seferinde - i için
	
loop1:
	
	beq $t0, $a2, exit1
	
	sub $a3, $a1, $t0 #size - i
	addi $a3, $a3, -1 #(size - i) - 1
	
	li $t2, 0 #j
	li $t3, 0 #dört artacak her seferinde - j için	
	
	loop2:
		
		beq $t2, $a3, exit2
		
 		sll $t6, $t2, 2 #4j
 		add $t6, $s0, $t6 #arr[j] nin adresi
 		lw $t4, 0($t6) #t3 = arr[j]
 		
 		addi $t5, $t2, 1 #j+1
 		
 		sll $t7, $t5, 2 #4*(j+1)
 		add $t7, $s0, $t7 #arr[j+1]'in adresi
 		lw $t5, 0($t7) #t4 = arr[j+1]
 		
 		bgt $t4, $t5, swap
 		
 		j jump
 		 		
 		swap:
 			sw $t4, 0($t7)
 			sw $t5, 0($t6)
 			
 		jump:
			addi $t2, $t2, 1
	 		addi $t3, $t3, 4
	 		j loop2
	
	exit2:
		addi $t0, $t0, 1
	 	addi $t1, $t1, 4
	 	j loop1
	 	
exit1:
	#print elements
	li $t6, 0
 	li $t8, 0
 			
	li   $v0, 4			
	la   $a0, sortedArr
	syscall		
				 			
 	loopPrint:
 		beq $t6, $a1, endPrint
 		
 		lw $t9, arr($t8)
	 	
		li $v0, 1
		move $a0, $t9
		syscall	
				
		li   $v0, 4			
		la   $a0, space
		syscall	
				
		addi $t6, $t6, 1
		addi $t8, $t8, 4
							
		j loopPrint
				
endPrint:
		li $v0, 10
		syscall
		
