ext2: ext2_ls.o
	gcc -Wall -g -o % $^

test_helper :  helper.o test_helper.o
	gcc -Wall -g -o test_helper $^

%.o : %.c helper.h ext2.h
	gcc -Wall -g -c $<

clean : 
	rm -f *.o *~
