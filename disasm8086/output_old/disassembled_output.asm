bits 16
add bx, [bx + si]
add bx, [bp + 0]
add si, 2
add bp, 2
add cx, 8
add bx, [bp + 0]
add cx, [bx + 2]
add bh, [bp + si + 4]
add di, [bp + di + 6]
add [bx + si], bx
add [bp + 0], bx
add [bp + 0], bx
add [bx + 2], cx
add [bp + si + 4], bh
add [bp + di + 6], di
add [bx], byte 34
add [bp + si + 1000], word 29
add ax, [bp + 0]
add al, [bx + si]
add ax, bx
add al, ah
add ax, 1000
add al, -30
add al, 9
sub bx, [bx + si]
sub bx, [bp + 0]
sub si, 2
sub bp, 2
sub cx, 8
sub bx, [bp + 0]
sub cx, [bx + 2]
sub bh, [bp + si + 4]
sub di, [bp + di + 6]
sub [bx + si], bx
sub [bp + 0], bx
sub [bp + 0], bx
sub [bx + 2], cx
sub [bp + si + 4], bh
sub [bp + di + 6], di
sub [bx], byte 34
sub [bx + di], word 29
sub ax, [bp + 0]
sub al, [bx + si]
sub ax, bx
sub al, ah
sub ax, 1000
sub al, -30
sub al, 9
cmp bx, [bx + si]
cmp bx, [bp + 0]
cmp si, 2
cmp bp, 2
cmp cx, 8
cmp bx, [bp + 0]
cmp cx, [bx + 2]
cmp bh, [bp + si + 4]
cmp di, [bp + di + 6]
cmp [bx + si], bx
cmp [bp + 0], bx
cmp [bp + 0], bx
cmp [bx + 2], cx
cmp [bp + si + 4], bh
cmp [bp + di + 6], di
cmp [bx], byte 34
cmp [4834], word 29
cmp ax, [bp + 0]
cmp al, [bx + si]
cmp ax, bx
cmp al, ah
cmp ax, 1000
cmp al, -30
cmp al, 9
label199:
jnz label203
jnz label199
label203:
jnz label199
jnz label203
label207:
je label207
jl label207
jle label207
jb label207
jbe label207
jp label207
jo label207
js label207
jnz label207
jnl label207
jg label207
jnb label207
ja label207
jnp label207
jno label207
jns label207
loop label207
loopz label207
loopnz label207
jcxz label207