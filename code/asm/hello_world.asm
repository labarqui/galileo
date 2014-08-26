global main

extern printf

section .data
	fmt: db 'Res: %s',0x0a, 0
	msg: db 'Hello world!',0

section .text
	main:
	mov esi, msg ; ABI is edi, esi
	mov edi, fmt
	mov eax, 0   ; rax is # of non-int args
	
	call printf

	ret
