SUBDIRS:= `ls | egrep -v '^(CVS)$$'`
all:
	@$(MAKE) -C libctrshell
	@$(MAKE) -C CtrShellServer clean
	@$(MAKE) -C CtrShellServer

clean:
	@$(MAKE) -C libctrshell clean
	@$(MAKE) -C CtrShellServer clean

