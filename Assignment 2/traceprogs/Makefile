SRCS = simpleloop.c matmul.c blocked.c simpleloop2.c
PROGS = simpleloop matmul blocked simpleloop2

all : $(PROGS)

$(PROGS) : % : %.c
	gcc -Wall -g -o $@ $<


traces: $(PROGS)
	./runit simpleloop
	./runit matmul 100
	./runit blocked 100 25
	./runit simpleloop2 

.PHONY: clean
clean : 
	rm -f simpleloop matmul blocked simpleloop2 tr-*.ref *.marker *~
