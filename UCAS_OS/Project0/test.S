global main
main:
        li s0,1
	li s1,101
	li s2,0
loop:
        add  s2,s2,s0
	addi s0,s0,1
	blt s0,s1,loop
end:
	j end
