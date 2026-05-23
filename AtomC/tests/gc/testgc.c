// implementare recursiva pentru factorial
int fact(int n) // stack frame: n[-2] ret[-1] oldFP[0]
{
	if (n < 3)
		return n;
	return n * fact(n - 1);
}

void main() // stack frame: ret[-1] oldFP[0] r[1] i[2]
{
	put_i(4.9); // se afiseaza 4

	put_i(fact(3)); // se afiseaza 6

	// implementare nerecursiva pentru factorial
	int r;
	r = 1;
	int i;
	i = 2;
	while (i < 5)
	{
		r = r * i;
		i = i + 1;
	}
	put_i(r); // se afiseaza 24
}

/*
			CALL main
			HLT

main:		ENTER 2
			PUSH.f 4.9
			CONV.f.i
			CALL_EXT put_i

			PUSH.i 3
			CALL fact
			CALL_EXT put_i

			PUSH.i 1
			FPSTORE.i 1
			PUSH.i 2
			FPSTORE.i 2
while_start:FPLOAD.i 2
			PUSH.i 5
			LESS.i
			JF while_end
			FPLOAD.i 1
			FPLOAD.i 2
			MUL.i
			FPSTORE.i 1
			FPLOAD.i 2
			PUSH.i 1
			ADD.i
			FPSTORE.i 2
			JMP while_start
while_end:	FPLOAD.i 1
			CALL_EXT put_i
			RET_VOID 0

fact:		ENTER 0
			FPLOAD.i -2
			PUSH.i 3
			LESS.i
			JF else
			FPLOAD.i -2
			RET 1
else:		FPLOAD.i -2
			FPLOAD.i -2
			PUSH.i 1
			SUB.i
			CALL fact
			MUL.i
			RET 1
*/