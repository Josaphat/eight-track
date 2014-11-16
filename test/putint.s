format_str:
	.asciz "%d\n"

	.globl putint
putint:
	movl %edi, %esi
	movl $format_str, %edi
	call printf
	ret
