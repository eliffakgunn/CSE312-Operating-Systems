.data 
       prime: .asciiz " prime" 
       newLine:	.asciiz "\n"
       
.text
              
main:
	li $a1, 0 #i
	
	#find prime numbers from 0 to 1000
	findPrimes:
		beq  $a1, 1001, endFindPrimes
		
		jal isPrime
		
		move $t0, $v0 	
	
		beq $t0, 1, printPrime

		li $v0, 1
		move $a0, $a1
		syscall
			
		li $v0, 4
		la $a0, newLine
		syscall
	
		addi $a1, $a1, 1
		j findPrimes
	
		printPrime:
			li $v0, 1
			move $a0, $a1
			syscall
		
			li $v0, 4
			la $a0, prime
			syscall
			
			li $v0, 4
			la $a0, newLine
			syscall
			
		addi $a1, $a1, 1
		j findPrimes
	
	endFindPrimes:
		li $v0, 10
		syscall
		
	
isPrime:
	ble $a1, 1, retFalse #num <= 1 ise false
	
	li $a2, 2 #i
	loop:
		beq $a2, $a1, retTrue
				
		#num / divisor
		div $t0, $a1, $a2
		#divisor*(num/divisor)
		mul $t0, $a2, $t0
		#mod = num - (divisor*(num/divisor))
		sub $t0, $a1, $t0
		
		beq $t0, 0, retFalse
		
		addi $a2, $a2, 1
		j loop
	
	retTrue:
		li $v0, 1
	 	jr $ra
	 	
	 retFalse:
		li $v0, 0
	 	jr $ra
		


