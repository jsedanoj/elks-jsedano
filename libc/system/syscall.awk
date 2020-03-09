#!/usr/bin/awk -f

function print_syscall(file, p3, p4)
{
	printf "\t_syscall_" > sf;
	if(p4 != "!" || p3 < 1)
		printf "%d", p3 > sf;
	else
		printf "%dp", p3 - 1 > sf;
	print "" > sf;
}

BEGIN {
	system("rm -f syscall");
	system("mkdir syscall");
	print "OBJS += \\";
}

/^[	 ]*#/ { next; }
/^[	 ]*$/ { next; }
{
	callno = $2 + 0;
	if (!(callno in calltab))
		callwas [callno] = $1;

	if ($3 == "x" || $3 == "") next;
	else if ($4 == "@" || $4 == "-") next;
	else if ($4 == "*") funcname = "_" $1;
	else funcname = $1;

	if (callno > max_call) max_call = callno;
	calltab [callno] = $1;

	sf = "syscall/" funcname ".s";

	print "// This file is automatically generated from syscall.dat" > sf;
	print "// See syscall.awk for details" > sf;
	print "\t.code16" > sf;
	print "\t.text" > sf;
	printf "\t.extern" > sf;
	print_syscall(sf, $3, $4)
	printf "\t.global %s\n", funcname > sf;
	printf "%s:\n", funcname > sf;
	printf "\tmov\t$%d,%%ax\n", callno > sf;
	printf "\tjmp" > sf;
	print_syscall(sf, $3, $4);
	close(sf);

	printf "\tsyscall/%s.o \\\n", funcname;
}
END {
	for (i = 0; i <= max_call; i++)
		if (i in calltab) {
			printf ("#ifndef sys_%s\n", calltab [i]) > "defn_tab.v";
			printf ("#define sys_%s sys_enosys\n", calltab [i]) > "defn_tab.v";
			printf ("#endif\n\n") > "defn_tab.v";

			printf ("/* %3d */  sys_%s,\n", i, calltab [i]) > "call_tab.v";
		}
		else
			printf("/* %3d */  sys_enosys, /* %s */\n", i, callwas [i]) > "call_tab.v";
}
