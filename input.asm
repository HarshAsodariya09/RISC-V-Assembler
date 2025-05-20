.text:
addi x1, x0, 0     # x1 = 0
jal x2, jump       # jump to label
addi x1, x0, 99    # this should be skipped
jump:
addi x3, x0, 42    # x3 = 42